// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <windows.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <Richedit.h>
#include <Ime.h>
#include <shellapi.h>
#include <dde.h>
#include <stdio.h>
#include <map>
#include "trayhook.h"

//Initialized Data to be shared with all instance of the dll
#pragma data_seg("Shared")
HWND g_hNotifyWnd = NULL;
HWND g_hCaptureWnd = NULL;
HINSTANCE g_hInstance = NULL;
HHOOK g_hCBTHook = NULL;
HHOOK g_hCallWndProcHook = NULL;
HHOOK g_hGetMessageHook = NULL;
#pragma data_seg()
// Initialised data End of data share
#pragma comment(linker,"/section:Shared,RWS")

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        g_hInstance = hModule;
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


#ifdef __cplusplus
extern "C"
{
#endif

    static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode >= 0)
        {
            if (nCode == HCBT_ACTIVATE)  //Called when the application window is activated
            {
                ::PostMessage(g_hNotifyWnd, WM_NotifyActivate, wParam, NULL);
            }
            else if (nCode == HCBT_SETFOCUS)
            {
                ::PostMessage(g_hNotifyWnd, WM_NotifyFocus, wParam, NULL);
            }
            else if (nCode == HCBT_DESTROYWND) //Called when the application window is destroyed
            {

            }
        }
        return CallNextHookEx(g_hCBTHook, nCode, wParam, lParam);
    }

    DLL_EXPORT bool InstallCBTHook(HWND hNotifyWnd)
    {
        g_hNotifyWnd = hNotifyWnd;

        if (!g_hCBTHook)
        {
            g_hCBTHook = SetWindowsHookEx(WH_CBT, (HOOKPROC)CBTProc, g_hInstance, 0);

            if (g_hCBTHook)
            {
                OutputDebugStringA("Hook CBT succeed\n");
                return true;
            }
            else
            {
                DWORD dwError = GetLastError();
                char szError[MAX_PATH];
                _snprintf_s(szError, MAX_PATH, "Hook CBT failed, error = %u\n", dwError);
                OutputDebugStringA(szError);
            }
        }

        return false;
    }

    DLL_EXPORT bool UninstallCBTHook()
    {
        if (g_hCBTHook)
        {
            UnhookWindowsHookEx(g_hCBTHook);
            g_hCBTHook = NULL;
            OutputDebugStringA("Uninstall CBT Hook\n");
        }

        return true;
    }

    //note:
    //CallWndProc will be executed in the process which myhook.dll injected, not the MySpy process
    static LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam)
    {
        if (code == HC_ACTION)
        {
            PCWPSTRUCT pMsg = (PCWPSTRUCT)lParam;

            if (pMsg->hwnd == g_hCaptureWnd && pMsg->message == WM_COPYDATA)
            {
                PTRAY_ICON_DATAW lpTrayData = nullptr;
                COPYDATASTRUCT* lpShellCDS =
                    (COPYDATASTRUCT*)pMsg->lParam;
                COPYDATASTRUCT lpSevCDS = { 0 };
                DWORD_PTR lpdwResult = 0;

                if (lpShellCDS->dwData == 1)  // 判断是否是 Shell_NotifyIcon 调用
                {
                    lpTrayData = (TRAY_ICON_DATAW*)lpShellCDS->lpData;

                    if (lpTrayData->Signature == 0x34753423)  // 判断是否是 NOTIFYICONDATA 结构体封送过程
                    {
                        // 填充 COPYDATASTRUCT 结构体
                        lpSevCDS.dwData = WM_NotifyCallWndProc;
                        lpSevCDS.cbData = sizeof(TRAY_ICON_DATAW);
                        lpSevCDS.lpData = lpTrayData;

                        // 发送消息到我们自己的窗口(10 秒)
                        SendMessageTimeoutW(g_hNotifyWnd,
                            WM_COPYDATA, (WPARAM)pMsg->wParam,
                            (LPARAM)&lpSevCDS, SMTO_NOTIMEOUTIFNOTHUNG, 0x0A, &lpdwResult);

                    }
                }
                //char szBuf[MAX_PATH] = {0};
                //_snprintf_s(szBuf, MAX_PATH, "CallWndProc Handle: 0x%08X SendMsg: %s(%04X), wParam: %08X, lParam: %08X\n", 
                //    pMsg->hwnd, GetMsgStringA(pMsg->message), pMsg->message, (int)pMsg->wParam, (int)pMsg->lParam);
                //OutputDebugStringA(szBuf);
            }
        }

        return CallNextHookEx(g_hCallWndProcHook, code, wParam, lParam);
    }

    DLL_EXPORT bool InstallCallWndProcHook(HWND hNotifyWnd, HWND hCaptureWnd)
    {
        g_hNotifyWnd = hNotifyWnd;
        g_hCaptureWnd = hCaptureWnd;

        if (!g_hCallWndProcHook)
        {
            DWORD dwThreadId = ::GetWindowThreadProcessId(g_hCaptureWnd, NULL);
            g_hCallWndProcHook = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC)CallWndProc, g_hInstance, dwThreadId);

            if (g_hCallWndProcHook)
            {
                OutputDebugStringA("Hook CallWndProc succeed\n");
                return true;
            }
            else
            {
                DWORD dwError = GetLastError();
                char szError[MAX_PATH];
                _snprintf_s(szError, MAX_PATH, "Hook CallWndProc failed, error = %u\n", dwError);
                OutputDebugStringA(szError);
            }
        }

        return false;
    }

    DLL_EXPORT bool UninstallCallWndProcHook()
    {
        if (g_hCallWndProcHook)
        {
            UnhookWindowsHookEx(g_hCallWndProcHook);
            g_hCallWndProcHook = NULL;
            OutputDebugStringA("Uninstall CallWndProc Hook\n");
        }

        return true;
    }


    //note:
    //CallWndProc will be executed in the process which myhook.dll injected, not the MySpy process
    static LRESULT CALLBACK GetMessageProc(int code, WPARAM wParam, LPARAM lParam)
    {
        if (code == HC_ACTION)
        {
            PCWPSTRUCT pMsg = (PCWPSTRUCT)lParam;

            SHELLWND_MAG lpTrayData = { 0 };
            COPYDATASTRUCT lpSevCDS = { 0 };
            DWORD_PTR lpdwResult = 0;

            lpTrayData.hMsgWnd = pMsg->hwnd;
            lpTrayData.message = pMsg->message;
            lpTrayData.wParam = pMsg->wParam;
            lpTrayData.lParam = pMsg->lParam;

            // 填充 COPYDATASTRUCT 结构体
            lpSevCDS.dwData = WM_NotifyGetMessage;
            lpSevCDS.cbData = sizeof(SHELLWND_MAG);
            lpSevCDS.lpData = &lpTrayData;

            SendMessageTimeoutW(g_hNotifyWnd,
                WM_COPYDATA, (WPARAM)wParam,
                (LPARAM)&lpSevCDS, SMTO_NOTIMEOUTIFNOTHUNG, 0x0A, &lpdwResult);
            //char szBuf[MAX_PATH] = {0};
            //_snprintf_s(szBuf, MAX_PATH, "GetMessage Handle: 0x%08X PostMsg: %s(%04X), wParam: %08X, lParam: %08X\n", 
            //    pMsg->hwnd, GetMsgStringA(pMsg->message), pMsg->message, (int)pMsg->wParam, (int)pMsg->lParam);
            //OutputDebugStringA(szBuf);
        }

        return CallNextHookEx(g_hGetMessageHook, code, wParam, lParam);
    }

    DLL_EXPORT bool InstallGetMessageHook(HWND hNotifyWnd, HWND hCaptureWnd)
    {
        g_hNotifyWnd = hNotifyWnd;
        g_hCaptureWnd = hCaptureWnd;

        if (!g_hGetMessageHook)
        {
            DWORD dwThreadId = ::GetWindowThreadProcessId(g_hCaptureWnd, NULL);
            g_hGetMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)GetMessageProc, g_hInstance, dwThreadId);

            if (g_hGetMessageHook)
            {
                OutputDebugStringA("Hook GetMessage succeed\n");
                return true;
            }
            else
            {
                DWORD dwError = GetLastError();
                char szError[MAX_PATH];
                _snprintf_s(szError, MAX_PATH, "Hook GetMessage failed, error = %u\n", dwError);
                OutputDebugStringA(szError);
            }
        }

        return false;
    }

    DLL_EXPORT bool UninstallGetMessageHook()
    {
        if (g_hGetMessageHook)
        {
            UnhookWindowsHookEx(g_hGetMessageHook);
            g_hGetMessageHook = NULL;
            OutputDebugStringA("Uninstall GetMessage Hook\n");
        }

        return true;
    }

}