/* Copyright (c) 2026 SCCypherNomad. SPDX-License-Identifier: MIT */
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif

#define APP_CLASS L"AltXTimer.MainWindow.1"
#define APP_NAME L"AltX Anti-AFK"
#define APP_VERSION L"v1.1.2"
#define ID_ACTIVATE 101
#define ID_DEACTIVATE 102
#define TIMER_START 1
#define TIMER_REPEAT 2
#define REPEAT_MS 600000U
#define START_MS 10000U
#define INPUT_TAG ((ULONG_PTR)0x41585431)
#define ID_HEADER_TAG 201
#define ID_VERSION 202
#define ID_KEY_LABEL 203
#define ID_KEY_READOUT 204
#define ID_INTERVAL_LABEL 205
#define ID_INTERVAL 206
#define ID_FOOTER 207
#define ID_CREDIT 208

#define COLOR_SPACE RGB(8, 14, 25)
#define COLOR_PANEL RGB(16, 29, 46)
#define COLOR_CYAN RGB(67, 199, 255)
#define COLOR_TEXT RGB(234, 245, 255)
#define COLOR_MUTED RGB(139, 167, 191)
#define COLOR_BORDER RGB(31, 67, 92)

static HWND mainWindow, activateButton, deactivateButton, statusLabel;
static HFONT bodyFont, headingFont, monoFont, keyFont, fineFont, buttonFont;
static HBRUSH background, panelBrush, activateBrush, pressedBrush, disabledBrush;
static HPEN borderPen, accentPen, mutedPen;
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
    RECT indicator = {23, 204, 44, 240};
    SetWindowTextW(statusLabel, message);
    InvalidateRect(mainWindow, &indicator, FALSE);
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
    SetStatus(L"Active: Left Alt + X every 10 minutes.");
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

static void DrawPanel(HDC dc, int left, int top, int right, int bottom)
{
    RECT panel = {left, top, right, bottom};
    FillRect(dc, &panel, panelBrush);
    SelectObject(dc, borderPen);
    SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, left, top, right, bottom);
}

static void PaintInterface(HWND window)
{
    PAINTSTRUCT paint;
    RECT client;
    HDC dc = BeginPaint(window, &paint);
    int saved = SaveDC(dc);
    POINT corner[3] = {{22, 8}, {12, 8}, {12, 18}};
    GetClientRect(window, &client);
    FillRect(dc, &client, background);

    SelectObject(dc, accentPen);
    Polyline(dc, corner, 3);
    MoveToEx(dc, 22, 92, NULL);
    LineTo(dc, 96, 92);
    SelectObject(dc, borderPen);
    LineTo(dc, 418, 92);

    DrawPanel(dc, 22, 111, 418, 188);
    SelectObject(dc, accentPen);
    MoveToEx(dc, 22, 123, NULL);
    LineTo(dc, 22, 175);
    SelectObject(dc, borderPen);
    MoveToEx(dc, 291, 124, NULL);
    LineTo(dc, 291, 175);
    DrawPanel(dc, 22, 203, 418, 241);

    /* Static space marks and the status indicator never schedule redraws. */
    SetPixelV(dc, 429, 112, COLOR_BORDER);
    SetPixelV(dc, 9, 196, COLOR_BORDER);
    SelectObject(dc, mutedPen);
    MoveToEx(dc, 426, 305, NULL);
    LineTo(dc, 434, 305);
    MoveToEx(dc, 430, 301, NULL);
    LineTo(dc, 430, 309);
    SelectObject(dc, active ? accentPen : mutedPen);
    MoveToEx(dc, 34, 218, NULL);
    LineTo(dc, 38, 222);
    LineTo(dc, 34, 226);
    LineTo(dc, 30, 222);
    LineTo(dc, 34, 218);
    RestoreDC(dc, saved);
    EndPaint(window, &paint);
}

static LRESULT DrawButton(const DRAWITEMSTRUCT *item)
{
    HDC dc = item->hDC;
    RECT bounds = item->rcItem, textBounds = item->rcItem;
    BOOL primary = item->CtlID == ID_ACTIVATE;
    BOOL disabled = (item->itemState & ODS_DISABLED) != 0;
    BOOL pressed = (item->itemState & ODS_SELECTED) != 0;
    BOOL focused = (item->itemState & ODS_FOCUS) != 0;
    HBRUSH fill = disabled ? disabledBrush :
        (pressed ? pressedBrush : (primary ? activateBrush : panelBrush));
    WCHAR label[32];
    int saved = SaveDC(dc);

    FillRect(dc, &bounds, fill);
    SelectObject(dc, disabled ? borderPen : (primary || focused ? accentPen : mutedPen));
    SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, bounds.left, bounds.top, bounds.right, bounds.bottom);
    if (focused && !disabled) {
        RECT focus = bounds;
        InflateRect(&focus, -4, -4);
        Rectangle(dc, focus.left, focus.top, focus.right, focus.bottom);
    }
    if (pressed) OffsetRect(&textBounds, 1, 1);
    GetWindowTextW(item->hwndItem, label, 32);
    SelectObject(dc, buttonFont);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, disabled ? RGB(91, 118, 139) : COLOR_TEXT);
    DrawTextW(dc, label, -1, &textBounds,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    RestoreDC(dc, saved);
    return TRUE;
}

static void ApplyDarkFrame(HWND window)
{
    HMODULE library = LoadLibraryW(L"dwmapi.dll");
    HRESULT (WINAPI *setAttribute)(HWND, DWORD, LPCVOID, DWORD);
    BOOL enabled = TRUE;
    if (!library) return;
    setAttribute = (void *)GetProcAddress(library, "DwmSetWindowAttribute");
    if (setAttribute) {
        /* Older Windows builds simply retain their standard title bar. */
        setAttribute(window, 20, &enabled, sizeof(enabled));
    }
    FreeLibrary(library);
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message,
                                   WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        mainWindow = window;
        AddControl(window, L"STATIC", L"SESSION UTILITY", SS_NOPREFIX,
            22, 16, 300, 16, ID_HEADER_TAG, monoFont);
        AddControl(window, L"STATIC", APP_NAME, SS_NOPREFIX,
            22, 38, 305, 36, 0, headingFont);
        AddControl(window, L"STATIC", APP_VERSION, SS_RIGHT | SS_NOPREFIX,
            346, 49, 72, 18, ID_VERSION, monoFont);
        AddControl(window, L"STATIC", L"KEY COMBINATION", SS_NOPREFIX,
            40, 124, 235, 16, ID_KEY_LABEL, monoFont);
        AddControl(window, L"STATIC", L"Left Alt + X", SS_NOPREFIX,
            40, 145, 241, 31, ID_KEY_READOUT, keyFont);
        AddControl(window, L"STATIC", L"INTERVAL", SS_NOPREFIX,
            307, 128, 101, 16, ID_INTERVAL_LABEL, monoFont);
        AddControl(window, L"STATIC", L"EVERY 10 MIN", SS_NOPREFIX,
            307, 150, 101, 20, ID_INTERVAL, monoFont);
        statusLabel = AddControl(window, L"STATIC", L"Ready. Activate to begin.", 0,
            46, 213, 357, 20, 0, bodyFont);
        activateButton = AddControl(window, L"BUTTON", L"Activate",
            WS_TABSTOP | BS_OWNERDRAW, 22, 258, 190, 44, ID_ACTIVATE, buttonFont);
        deactivateButton = AddControl(window, L"BUTTON", L"Deactivate",
            WS_TABSTOP | BS_OWNERDRAW, 228, 258, 190, 44, ID_DEACTIVATE, buttonFont);
        AddControl(window, L"STATIC", L"Deactivate stops and closes the app.", SS_NOPREFIX,
            22, 314, 267, 16, ID_FOOTER, fineFont);
        AddControl(window, L"STATIC", L"SCCypherNomad", SS_RIGHT | SS_NOPREFIX,
            303, 314, 115, 16, ID_CREDIT, fineFont);
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
            SetWindowTextW(window, APP_NAME L" - Active");
            SetStatus(L"Active: first press in 10 seconds.");
            SetFocus(deactivateButton);
            ShowWindow(window, SW_MINIMIZE);
            return 0;
        }
        if (LOWORD(wParam) == ID_DEACTIVATE) {
            SendMessageW(window, WM_CLOSE, 0, 0);
            return 0;
        }
        break;
    case WM_PAINT:
        PaintInterface(window);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_DRAWITEM:
        if (((const DRAWITEMSTRUCT *)lParam)->CtlType == ODT_BUTTON)
            return DrawButton((const DRAWITEMSTRUCT *)lParam);
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
        {
            int id = GetDlgCtrlID((HWND)lParam);
            BOOL inPanel = (HWND)lParam == statusLabel ||
                (id >= ID_KEY_LABEL && id <= ID_INTERVAL);
            COLORREF color = COLOR_TEXT;
            if (id == ID_HEADER_TAG || id == ID_INTERVAL) color = COLOR_CYAN;
            else if (id == ID_VERSION || id == ID_KEY_LABEL || id == ID_INTERVAL_LABEL ||
                     id == ID_FOOTER || id == ID_CREDIT) color = COLOR_MUTED;
            SetTextColor((HDC)wParam, color);
            SetBkColor((HDC)wParam, inPanel ? COLOR_PANEL : COLOR_SPACE);
            return (LRESULT)(inPanel ? panelBrush : background);
        }
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
    RECT bounds = {0, 0, 440, 332};
    MSG message;
    int result;
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;

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
    background = CreateSolidBrush(COLOR_SPACE);
    panelBrush = CreateSolidBrush(COLOR_PANEL);
    activateBrush = CreateSolidBrush(RGB(13, 52, 78));
    pressedBrush = CreateSolidBrush(RGB(19, 78, 111));
    disabledBrush = CreateSolidBrush(RGB(15, 25, 38));
    borderPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    accentPen = CreatePen(PS_SOLID, 1, COLOR_CYAN);
    mutedPen = CreatePen(PS_SOLID, 1, RGB(49, 107, 141));
    bodyFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    headingFont = CreateFontW(-26, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    monoFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
    keyFont = CreateFontW(-24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
    fineFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    buttonFont = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
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
    ApplyDarkFrame(mainWindow);
    ShowWindow(mainWindow, showCommand);
    UpdateWindow(mainWindow);

    /* GetMessage sleeps until work arrives. No polling or inactive timer. */
    while ((result = GetMessageW(&message, NULL, 0, 0)) > 0) {
        if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) {
            SendMessageW(mainWindow, WM_CLOSE, 0, 0);
        } else if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN) {
            HWND focused = GetFocus();
            if (focused != activateButton && focused != deactivateButton)
                focused = active ? deactivateButton : activateButton;
            if (IsWindowEnabled(focused))
                SendMessageW(focused, BM_CLICK, 0, 0);
        } else if (!IsDialogMessageW(mainWindow, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (smallIcon) DestroyIcon(smallIcon);
    if (largeIcon) DestroyIcon(largeIcon);
    DeleteObject(bodyFont);
    DeleteObject(headingFont);
    DeleteObject(monoFont);
    DeleteObject(keyFont);
    DeleteObject(fineFont);
    DeleteObject(buttonFont);
    DeleteObject(background);
    DeleteObject(panelBrush);
    DeleteObject(activateBrush);
    DeleteObject(pressedBrush);
    DeleteObject(disabledBrush);
    DeleteObject(borderPen);
    DeleteObject(accentPen);
    DeleteObject(mutedPen);
    CloseHandle(singleton);
    return result == -1 ? 1 : 0;
}
