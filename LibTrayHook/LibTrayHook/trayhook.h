#pragma once

#ifdef LIBTRAYHOOK_EXPORTS
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

enum NotifyMsg
{
    WM_NotifyActivate = WM_APP + 1,
    WM_NotifyFocus,
    WM_NotifyCallWndProc,
    WM_NotifyGetMessage,
};

// x64 结构体的声明
typedef struct _TRAY_ICON_DATAW {
    DWORD Signature;
    DWORD dwMessage;   // dwMessage <-- Shell_NotifyIconW(DWORD dwMessage, ...)
    DWORD cbSize;
    DWORD hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    DWORD uIconID; // HICON hIcon; why it changes?
#if (NTDDI_VERSION < NTDDI_WIN2K)
    WCHAR  szTip[64];
#endif
#if (NTDDI_VERSION >= NTDDI_WIN2K)
    WCHAR  szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR  szInfo[256];
#ifndef _SHELL_EXPORTS_INTERNALAPI_H_
    union {
        UINT  uTimeout;
        UINT  uVersion;  // used with NIM_SETVERSION, values 0, 3 and 4
    } DUMMYUNIONNAME;
#endif
    WCHAR  szInfoTitle[64];
    DWORD dwInfoFlags;
#endif
#if (NTDDI_VERSION >= NTDDI_WINXP)
    GUID guidItem;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
    HICON hBalloonIcon;
#endif
} TRAY_ICON_DATAW, * PTRAY_ICON_DATAW;


typedef struct __SHELLWND_MAG
{
    LPARAM  lParam;
    WPARAM  wParam;
    UINT    message;
    HWND    hMsgWnd;
}SHELLWND_MAG, PSHELLWND_MAG;

#ifdef __cplusplus
extern "C"
{
#endif

    DLL_EXPORT bool InstallCBTHook(HWND hNotifyWnd);
    DLL_EXPORT bool UninstallCBTHook();

    DLL_EXPORT bool InstallCallWndProcHook(HWND hNotifyWnd, HWND hCaptureWnd);
    DLL_EXPORT bool UninstallCallWndProcHook();

    DLL_EXPORT bool InstallGetMessageHook(HWND hNotifyWnd, HWND hCaptureWnd);
    DLL_EXPORT bool UninstallGetMessageHook();

#ifdef __cplusplus
}
#endif