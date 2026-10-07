/* Copyright (c) 2026 Cypher Nomad. SPDX-License-Identifier: MIT */
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif

#define APP_CLASS L"AltXTimer.MainWindow.1"
#define APP_NAME L"AltX Timer"
#define ID_ACTIVATE 101
#define ID_DEACTIVATE 102
#define TIMER_START 1
#define TIMER_REPEAT 2
#define REPEAT_MS 240000U
#define START_MS 3000U
#define INPUT_TAG ((ULONG_PTR)0x41585431)

static HWND mainWindow, activateButton, deactivateButton, statusLabel;
static HFONT bodyFont, headingFont;
static HBRUSH background;
static HICON largeIcon, smallIcon;
static BOOL active;

static void StopTimer(void)
{
    /* A cancelled timer may still have a queued message. Clear this first. */
    active = FALSE;
    KillTimer(mainWindow, TIMER_START);
    KillTimer(mainWindow, TIMER_REPEAT);
}

static void SetStatus(const WCHAR *message)
{
    SetWindowTextW(statusLabel, message);
}

static void ShowFailure(const WCHAR *message)
{
    StopTimer();
    SetWindowTextW(mainWindow, APP_NAME);
    EnableWindow(activateButton, TRUE);
    SetStatus(message);
    ShowWindow(mainWindow, SW_RESTORE);
}

static BOOL KeyIsDown(int key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

static void KeyInput(INPUT *input, WORD scan, BOOL release)
{
    ZeroMemory(input, sizeof(*input));
    input->type = INPUT_KEYBOARD;
    input->ki.wScan = scan;
    input->ki.dwFlags = KEYEVENTF_SCANCODE | (release ? KEYEVENTF_KEYUP : 0);
    input->ki.dwExtraInfo = INPUT_TAG;
}

static void PressCombo(void)
{
    HWND foreground;
    DWORD processId = 0, threadId;
    HKL layout;
    WORD altScan, xScan;
    INPUT events[4], cleanup[2];
    UINT sent, cleanupCount = 0;

    if (!active) return;
    foreground = GetForegroundWindow();
    if (!foreground) {
        SetStatus(L"Active: waiting for an unlocked desktop.");
        return;
    }
    threadId = GetWindowThreadProcessId(foreground, &processId);
    if (!threadId) {
        SetStatus(L"Active: waiting for your app.");
        return;
    }
    if (processId == GetCurrentProcessId()) {
        SetStatus(L"Active: select your app for the next press.");
        return;
    }
    /* Never change keys the user is holding or add unwanted modifiers. */
    if (KeyIsDown(VK_MENU) || KeyIsDown(VK_CONTROL) || KeyIsDown(VK_SHIFT) ||
        KeyIsDown(VK_LWIN) || KeyIsDown(VK_RWIN) || KeyIsDown('X')) {
        SetStatus(L"Active: held keys skipped this press.");
        return;
    }

    layout = GetKeyboardLayout(threadId);
    altScan = (WORD)MapVirtualKeyExW(VK_LMENU, MAPVK_VK_TO_VSC, layout);
    xScan = (WORD)MapVirtualKeyExW('X', MAPVK_VK_TO_VSC, layout);
    if (!altScan || !xScan) {
        ShowFailure(L"Unable to map Left Alt + X. Try Activate again.");
        return;
    }

    KeyInput(&events[0], altScan, FALSE);
    KeyInput(&events[1], xScan, FALSE);
    KeyInput(&events[2], xScan, TRUE);
    KeyInput(&events[3], altScan, TRUE);
    sent = SendInput(4, events, sizeof(INPUT));
    if (sent != 4) {
        /* Release only keys this batch pressed but could not release. */
        if (sent >= 2 && sent < 3)
            KeyInput(&cleanup[cleanupCount++], xScan, TRUE);
        if (sent >= 1 && sent < 4)
            KeyInput(&cleanup[cleanupCount++], altScan, TRUE);
        if (cleanupCount && SendInput(cleanupCount, cleanup, sizeof(INPUT)) != cleanupCount) {
            ShowFailure(L"Tap Left Alt and X to release them, then retry.");
            return;
        }
        ShowFailure(L"Input blocked. Check your app, then Activate.");
        return;
    }
    SetStatus(L"Active: Left Alt + X every 4 minutes.");
}

static HWND AddControl(HWND parent, const WCHAR *className, const WCHAR *text,
                       DWORD style, int x, int y, int w, int h, int id,
                       HFONT font)
{
    HWND control = CreateWindowExW(0, className, text, WS_CHILD | WS_VISIBLE | style,
        x, y, w, h, parent, (HMENU)(INT_PTR)id,
        GetModuleHandleW(NULL), NULL);
    SendMessageW(control, WM_SETFONT, (WPARAM)font, TRUE);
    return control;
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message,
                                   WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        mainWindow = window;
        AddControl(window, L"STATIC", L"Left Alt + X", 0,
            22, 18, 320, 32, 0, headingFont);
        AddControl(window, L"STATIC", L"Repeat every 4 minutes", 0,
            24, 54, 320, 22, 0, bodyFont);
        statusLabel = AddControl(window, L"STATIC", L"Ready. Activate to begin.", 0,
            24, 91, 320, 26, 0, bodyFont);
        activateButton = AddControl(window, L"BUTTON", L"Activate",
            WS_TABSTOP | BS_DEFPUSHBUTTON, 24, 126, 150, 37, ID_ACTIVATE, bodyFont);
        deactivateButton = AddControl(window, L"BUTTON", L"Deactivate",
            WS_TABSTOP | BS_PUSHBUTTON, 186, 126, 150, 37, ID_DEACTIVATE, bodyFont);
        AddControl(window, L"STATIC", L"Deactivate stops and closes the app.", 0,
            24, 177, 320, 23, 0, bodyFont);
        SetFocus(activateButton);
        return 0;
    case WM_COMMAND:
        if (HIWORD(wParam) != BN_CLICKED) break;
        if (LOWORD(wParam) == ID_ACTIVATE && !active) {
            active = TRUE;
            if (!SetTimer(window, TIMER_START, START_MS, NULL)) {
                ShowFailure(L"Could not start timer. Try Activate again.");
                return 0;
            }
            EnableWindow(activateButton, FALSE);
            SetWindowTextW(window, L"AltX Timer - Active");
            SetStatus(L"Active: first press in 3 seconds.");
            SetFocus(deactivateButton);
            ShowWindow(window, SW_MINIMIZE);
            return 0;
        }
        if (LOWORD(wParam) == ID_DEACTIVATE) {
            SendMessageW(window, WM_CLOSE, 0, 0);
            return 0;
        }
        break;
    case WM_TIMER:
        if (!active) return 0;
        if (wParam == TIMER_START) {
            KillTimer(window, TIMER_START);
            if (!SetTimer(window, TIMER_REPEAT, REPEAT_MS, NULL)) {
                ShowFailure(L"Could not start repeat timer. Try Activate again.");
                return 0;
            }
            PressCombo();
        } else if (wParam == TIMER_REPEAT) {
            PressCombo();
        }
        return 0;
    case WM_CTLCOLORSTATIC:
        SetTextColor((HDC)wParam, RGB(36, 46, 62));
        SetBkColor((HDC)wParam, RGB(250, 251, 253));
        return (LRESULT)background;
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        if (!wParam) return 0;
        /* Intentional fallthrough to the same cleanup as a normal close. */
    case WM_CLOSE:
        StopTimer();
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        StopTimer();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR commandLine, int showCommand)
{
    HANDLE singleton;
    WNDCLASSEXW windowClass;
    RECT bounds = {0, 0, 360, 216};
    MSG message;
    int result;
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    singleton = CreateMutexW(NULL, FALSE, L"Local\\AltXTimer.FourMinute.1");
    if (!singleton) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(APP_CLASS, NULL);
        if (existing) {
            ShowWindow(existing, SW_RESTORE);
            SetForegroundWindow(existing);
        }
        CloseHandle(singleton);
        return 0;
    }

    /* Use the shell's default identity so normal Explorer pins group correctly. */
    background = CreateSolidBrush(RGB(250, 251, 253));
    bodyFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    headingFont = CreateFontW(-24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    largeIcon = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    smallIcon = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    windowClass.hbrBackground = background;
    windowClass.hIcon = largeIcon;
    windowClass.hIconSm = smallIcon;
    windowClass.lpszClassName = APP_CLASS;
    if (!RegisterClassExW(&windowClass)) return 1;
    AdjustWindowRectEx(&bounds, windowStyle, FALSE, 0);
    mainWindow = CreateWindowExW(0, APP_CLASS, APP_NAME, windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left,
        bounds.bottom - bounds.top, NULL, NULL, instance, NULL);
    if (!mainWindow) return 1;
    ShowWindow(mainWindow, showCommand);
    UpdateWindow(mainWindow);

    /* GetMessage sleeps until work arrives. No polling or inactive timer. */
    while ((result = GetMessageW(&message, NULL, 0, 0)) > 0) {
        if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) {
            SendMessageW(mainWindow, WM_CLOSE, 0, 0);
        } else if (!IsDialogMessageW(mainWindow, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (smallIcon) DestroyIcon(smallIcon);
    if (largeIcon) DestroyIcon(largeIcon);
    DeleteObject(bodyFont);
    DeleteObject(headingFont);
    DeleteObject(background);
    CloseHandle(singleton);
    return result == -1 ? 1 : 0;
}
