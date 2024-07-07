// DBG-Timer1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <strsafe.h>

// App name in ANSI and Wide
char szAppName[] = "InputTextSpell";
wchar_t wcAppName[] = L"InputTextSpell";

//Quick Messagebox macros
#define msgba(h,x) MessageBoxA(h,x,szAppName,MB_OK)
#define msgbw(h,x) MessageBoxW(h,x,wcAppName,MB_OK)
// Quick Messagebox ANCI
#define msga(x) msgba(ghMain,x)
// Quick Messagebox WIDE String
#define msgw(x) msgbw(ghMain,x)

// Function Prototypes
BOOL InitPaltalkWindows(void);
BOOL CopyPasteToPaltalk(char* szText);

// Paltalk Windows Handles
HWND ghPtMain = NULL;
// App main window Handle
HWND	ghMain = 0;

// Paltalk Pocess Id
DWORD gdwPtProcId = 0;
// Paltalk path
const char* szPaltalkPath = "C:\\Program Files (x86)\\Paltalk\\Paltalk.exe";
char gszPtMsg[MAX_PATH] = { 0 };


int main()
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    int iUserInput = 0;
    // Try to Start Paltalk

    CreateProcessA(szPaltalkPath, NULL, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
    // Save the Paltalk Process id in the clobal gdwPtProcId
    gdwPtProcId = pi.dwProcessId;

    std::cout << "Paltalk ProcH: " << pi.hProcess << " threadH: " << pi.hThread << " dwProcId: " << pi.dwProcessId << "\n";

    std::cout << "When Paltalk is Running, Open the Room for Timming, then enter 1 :";

    std::cin >> iUserInput;
    if (iUserInput != 1)
    {
        std::cout << "Start Paltalk and Try again!";
        return 2;
    }

    // Get the Room window handle
    InitPaltalkWindows();

    std::cout << "This should be Paltalk window handle: " << ghPtMain << "\n";
    std::cout << "input 1 to cont. :";
    std::cin >> iUserInput;

    // Try to attach to Paltalk in DEBUG
    DebugActiveProcess(gdwPtProcId);

    // This is the debug loop
    DEBUG_EVENT debug_event = { 0 };
    for (;;)
    {
        if (!WaitForDebugEvent(&debug_event, INFINITE))
            return  3;


        switch (debug_event.dwDebugEventCode)
        {
        case EXCEPTION_DEBUG_EVENT:
        {
            ContinueDebugEvent(debug_event.dwProcessId,
                debug_event.dwThreadId,
                DBG_EXCEPTION_NOT_HANDLED);
        }
        break;
        case CREATE_PROCESS_DEBUG_EVENT:
        {
            std::cout << "Process Strated Event\n";

        }
        break;
        case CREATE_THREAD_DEBUG_EVENT:
        {
            
        }
        break;
        case OUTPUT_DEBUG_STRING_EVENT:
        {
            OUTPUT_DEBUG_STRING_INFO& DebugString = debug_event.u.DebugString;

            WORD wDbStrLn = DebugString.nDebugStringLength;
            WORD wFunicode = DebugString.fUnicode;

            if (wFunicode == 0)
            {
                char* msg = new char[DebugString.nDebugStringLength];
                ZeroMemory(msg, DebugString.nDebugStringLength);
                ReadProcessMemory(pi.hProcess, DebugString.lpDebugStringData, msg, DebugString.nDebugStringLength, NULL);
                
                if (strstr(msg, "[TalkingNow]"))
                {
                    std::cout << "PT: " << msg << "\n";
                    if (strstr(msg, "STARTED"))
                    {
                        char* pszUser = strstr(msg, "STARTED");

                            sprintf_s(gszPtMsg, MAX_PATH, "TEST Timer | %s", pszUser);
                    }
                    else if (strstr(msg, "STOPPED"))
                    {
                        char* pszUser = strstr(msg, "STOPPED");
                        sprintf_s(gszPtMsg, MAX_PATH, "TEST Timer | %s", pszUser);
                    }

                     CopyPasteToPaltalk(gszPtMsg); // I need to clean up this code :P
                }

                delete[]msg;
            }


        }
        break;

        default:

            break;
        }

        ContinueDebugEvent(debug_event.dwProcessId,
            debug_event.dwThreadId,
            DBG_CONTINUE);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}

// Initialise the Paltalk Windows handles
BOOL InitPaltalkWindows(void)
{
    char szTitle[256] = { 0 };
    char szTemp[512] = { 0 };
    // Resetting handle
    ghPtMain = 0;

    ghPtMain = FindWindowW(L"Qt5150QWindowIcon", 0);

    if (!ghPtMain)
    {
        msga("There is No Paltalk Window!");
        return FALSE;
    }


    int iLen = GetWindowTextA(ghPtMain, szTitle, 254);
    if (iLen < 1)
    {
        msga("Error Getting Paltalk Room Title!");
        return FALSE;
    }

    // Getting and outputing Platalk room title
    std::cout << "This should be the room title: " << szTitle << "\n";

    //wsprintf(szTemp, "Paltalk Text - %s", szTitle);
    //SetWindowTextW(ghMain, szTemp);

    return TRUE;
}

// Copy text and paste it to Paltalk
BOOL CopyPasteToPaltalk(char* szText)
{
    BOOL bRet = FALSE;
    size_t iSize = 0;
    INPUT ip;

    iSize = sizeof(szText) * strlen(szText) + 8;

    if (ghPtMain && OpenClipboard(ghMain))
    {
        EmptyClipboard();
        HGLOBAL hGlb = GlobalAlloc(GMEM_MOVEABLE, iSize);
        if (!hGlb) return FALSE;
        LPWSTR lpwsClip = (LPWSTR)GlobalLock(hGlb);
        if (lpwsClip) memcpy(lpwsClip, szText, iSize);
        GlobalUnlock(hGlb);
        if (SetClipboardData(CF_TEXT, hGlb) == NULL)
        {
            GlobalFree(hGlb);
            EmptyClipboard();
            CloseClipboard();
            //Beep(900,200);
        }
        else
        {
            CloseClipboard();

            if (ghPtMain)
            {
                INPUT inpPaste[6] = {};
                ZeroMemory(inpPaste, sizeof(inpPaste));

                SetForegroundWindow(ghPtMain);
      
                // Control key down
                inpPaste[0].type = INPUT_KEYBOARD;
                inpPaste[0].ki.wVk = VK_CONTROL;
                inpPaste[0].ki.dwFlags = 0;

                //  "V" key down
                inpPaste[1].type = INPUT_KEYBOARD;
                inpPaste[1].ki.wVk = 0x56;
                inpPaste[1].ki.dwFlags = 0;
                // "V" key up
                inpPaste[3].type = INPUT_KEYBOARD;
                inpPaste[3].ki.wVk = 0x56;
                inpPaste[3].ki.dwFlags = KEYEVENTF_KEYUP;
                // Control key up
                inpPaste[4].type = INPUT_KEYBOARD;
                inpPaste[4].ki.wVk = VK_CONTROL;
                inpPaste[4].ki.dwFlags = KEYEVENTF_KEYUP;
                // Enter key down
                inpPaste[5].type = INPUT_KEYBOARD;
                inpPaste[5].ki.wVk = VK_RETURN;
                inpPaste[5].ki.dwFlags = 0;
                // Enter key up
                inpPaste[5].type = INPUT_KEYBOARD;
                inpPaste[5].ki.wVk = VK_RETURN;
                inpPaste[5].ki.dwFlags = 0;

                // Send these keys to Paltalk
                UINT uSent = SendInput(ARRAYSIZE(inpPaste), inpPaste, sizeof(INPUT));

                if (uSent != ARRAYSIZE(inpPaste))
                {
                    OutputDebugStringA("Error sending to Paltalk SendInput() failed!");
                }
                               
            }

        }

    }


    return TRUE;
}
