/* Copyright (c) 2026 SCCypherNomad. SPDX-License-Identifier: MIT */
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

/* These seams never call the real keyboard injection or focus APIs. */
static UINT WINAPI FakeSendInput(UINT count, LPINPUT inputs, int size);
static HWND WINAPI FakeGetForegroundWindow(void);
static SHORT WINAPI FakeGetAsyncKeyState(int key);
static BOOL WINAPI FakeShowWindow(HWND window, int command);
static HWND WINAPI FakeSetFocus(HWND window);

#define WinMain UtilityWinMain
#define SendInput FakeSendInput
#define GetForegroundWindow FakeGetForegroundWindow
#define GetAsyncKeyState FakeGetAsyncKeyState
#define ShowWindow FakeShowWindow
#define SetFocus FakeSetFocus
#include "../src/AltXTimer.c"
#undef WinMain
#undef SendInput
#undef GetForegroundWindow
#undef GetAsyncKeyState
#undef ShowWindow
#undef SetFocus

#define WATCHDOG_TIMER 99
static int failures;
static int inputCalls;
static int completeCombos;
static int partialReturn = -1;
static int failCleanup;
static int cleanupSeen;
static int realTimingStage;
static DWORD activationTick;
static DWORD firstTick;
static DWORD secondTick;
static UINT observedTimers[8];
static int observedTimerCount;

static void Check(int condition, const char *description)
{
    if (!condition) {
        ++failures;
        printf("FAIL: %s\n", description);
    }
}

static WORD ExpectedScan(UINT key)
{
    DWORD pid = 0;
    DWORD tid = GetWindowThreadProcessId(GetDesktopWindow(), &pid);
    return (WORD)MapVirtualKeyExW(key, MAPVK_VK_TO_VSC, GetKeyboardLayout(tid));
}

static void CheckEvent(const INPUT *input, WORD scan, BOOL release)
{
    Check(input->type == INPUT_KEYBOARD, "keyboard event type");
    Check(input->ki.wVk == 0, "scan-code event has no virtual-key override");
    Check(input->ki.wScan == scan, "expected mapped Left Alt or X scan code");
    Check(input->ki.dwFlags == (KEYEVENTF_SCANCODE | (release ? KEYEVENTF_KEYUP : 0)),
          "exact scan-code and release flags, with no extended Alt");
    Check(input->ki.dwExtraInfo == (ULONG_PTR)0x41585431, "production event tag");
    Check(input->ki.time == 0, "system-generated input timestamp");
}

static UINT WINAPI FakeSendInput(UINT count, LPINPUT inputs, int size)
{
    WORD expectedAlt = ExpectedScan(VK_LMENU);
    WORD expectedX = ExpectedScan('X');
    ++inputCalls;
    Check(size == sizeof(INPUT), "SendInput receives correct structure size");
    Check(expectedAlt != 0 && expectedX != 0, "desktop layout maps both keys");
    if (count == 4) {
        CheckEvent(&inputs[0], expectedAlt, FALSE);
        CheckEvent(&inputs[1], expectedX, FALSE);
        CheckEvent(&inputs[2], expectedX, TRUE);
        CheckEvent(&inputs[3], expectedAlt, TRUE);
        if (partialReturn >= 0) return (UINT)partialReturn;
        ++completeCombos;
        if (realTimingStage) {
            DWORD tick = GetTickCount();
            if (completeCombos == 1) {
                firstTick = tick;
                printf("REAL TIMER: initial press after %lu ms; Left Alt down, X down, X up, Left Alt up validated.\n",
                       (unsigned long)(firstTick - activationTick));
            } else if (completeCombos == 2) {
                secondTick = tick;
                printf("REAL TIMER: repeat after %lu ms; same complete key sequence validated.\n",
                       (unsigned long)(secondTick - firstTick));
            }
        }
        return count;
    }
    ++cleanupSeen;
    if (partialReturn == 2) {
        Check(count == 2, "partial two-event send releases X then Alt");
        if (count >= 1) CheckEvent(&inputs[0], expectedX, TRUE);
        if (count >= 2) CheckEvent(&inputs[1], expectedAlt, TRUE);
    } else {
        Check((partialReturn == 1 || partialReturn == 3) && count == 1,
              "partial one/three-event send releases only Alt");
        if (count >= 1) CheckEvent(&inputs[0], expectedAlt, TRUE);
    }
    return failCleanup ? 0 : count;
}

static HWND WINAPI FakeGetForegroundWindow(void) { return GetDesktopWindow(); }
static SHORT WINAPI FakeGetAsyncKeyState(int key) { (void)key; return 0; }
static BOOL WINAPI FakeShowWindow(HWND window, int command)
{ (void)window; (void)command; return FALSE; }
static HWND WINAPI FakeSetFocus(HWND window) { (void)window; return NULL; }

static LRESULT CALLBACK TestHostProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_TIMER && wParam == WATCHDOG_TIMER) {
        Check(0, "timing run completed before its 630-second watchdog");
        KillTimer(window, WATCHDOG_TIMER);
        return WindowProc(window, WM_CLOSE, 0, 0);
    }
    if (message == WM_TIMER && realTimingStage && observedTimerCount < 8)
        observedTimers[observedTimerCount++] = (UINT)wParam;
    return WindowProc(window, message, wParam, lParam);
}

static void ResetRecorder(void)
{
    inputCalls = completeCombos = cleanupSeen = 0;
    partialReturn = -1;
    failCleanup = 0;
}

int main(void)
{
    HINSTANCE instance = GetModuleHandleW(NULL);
    WNDCLASSEXW hostClass;
    HWND testWindow;
    MSG message;
    int result, n;
    WCHAR status[256];
    int before;

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Safety: keyboard injection, show/minimize and focus APIs are intercepted; only this hidden test window is controlled.\n");
    ZeroMemory(&hostClass, sizeof(hostClass));
    hostClass.cbSize = sizeof(hostClass);
    hostClass.hInstance = instance;
    hostClass.lpfnWndProc = TestHostProc;
    hostClass.lpszClassName = L"AltXTimer.HiddenIntegrationTest.1";
    if (!RegisterClassExW(&hostClass)) {
        printf("FAIL: test class creation (%lu)\n", (unsigned long)GetLastError());
        return 1;
    }
    testWindow = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        hostClass.lpszClassName, L"AltXTimer hidden production integration test",
        WS_OVERLAPPED, 0, 0, 360, 216, NULL, NULL, instance, NULL);
    if (!testWindow) {
        printf("FAIL: hidden test window creation (%lu)\n", (unsigned long)GetLastError());
        return 1;
    }
    Check(!IsWindowVisible(testWindow), "test window remains hidden");
    Check(mainWindow == testWindow && activateButton && deactivateButton && statusLabel,
          "production WM_CREATE made the real controls");

    for (n = 0; n <= 3; ++n) {
        ResetRecorder();
        partialReturn = n;
        active = TRUE;
        PressCombo();
        Check(!active, "partial SendInput disables production timer state");
        Check(IsWindowEnabled(activateButton), "partial failure re-enables Activate");
        Check(inputCalls == (n ? 2 : 1), "only necessary partial-send release call");
        Check(cleanupSeen == (n ? 1 : 0), "partial-send cleanup was observed");
        printf("PASS CASE: SendInput returned %d/4; exact necessary key releases validated.\n", n);
    }
    ResetRecorder();
    partialReturn = 2;
    failCleanup = 1;
    active = TRUE;
    PressCombo();
    GetWindowTextW(statusLabel, status, 256);
    Check(!active, "failed cleanup disables production timer state");
    Check(wcscmp(status, L"Tap Left Alt and X to release them, then retry.") == 0,
          "failed cleanup tells user how to release keys");
    printf("PASS CASE: failed release reports manual key-release instruction.\n");

    ResetRecorder();
    active = TRUE;
    StopTimer();
    WindowProc(testWindow, WM_TIMER, TIMER_START, 0);
    WindowProc(testWindow, WM_TIMER, TIMER_REPEAT, 0);
    Check(!active && inputCalls == 0, "stale start/repeat messages cannot send keys after StopTimer");
    printf("PASS CASE: stale WM_TIMER messages send no input after StopTimer.\n");

    ResetRecorder();
    realTimingStage = 1;
    activationTick = GetTickCount();
    WindowProc(testWindow, WM_COMMAND, MAKEWPARAM(ID_ACTIVATE, BN_CLICKED), 0);
    Check(active && !IsWindowEnabled(activateButton), "real production activation state and control");
    Check(!IsWindowVisible(testWindow), "activation did not show any test UI");
    if (!SetTimer(testWindow, WATCHDOG_TIMER, 630000, NULL)) {
        printf("FAIL: could not set test watchdog\n");
        WindowProc(testWindow, WM_CLOSE, 0, 0);
        return 1;
    }
    printf("RUNNING: waiting for actual production 3-second first timer, then actual 600-second repeat timer.\n");
    while ((result = GetMessageW(&message, NULL, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
        if (completeCombos == 2 && active) {
            Check(firstTick - activationTick >= 2900 && firstTick - activationTick <= 10000,
                  "initial timer measured near 3 seconds");
            Check(secondTick - firstTick >= 599900 && secondTick - firstTick <= 610000,
                  "repeat timer measured near 600 seconds");
            Check(observedTimerCount == 2 && observedTimers[0] == TIMER_START &&
                  observedTimers[1] == TIMER_REPEAT, "real timer messages arrive in expected order");
            KillTimer(testWindow, WATCHDOG_TIMER);
            before = inputCalls;
            WindowProc(testWindow, WM_COMMAND, MAKEWPARAM(ID_DEACTIVATE, BN_CLICKED), 0);
            Check(!active && !IsWindow(testWindow), "Deactivate stops timers and destroys window");
            Check(inputCalls == before, "Deactivate sends no additional input");
            WindowProc(testWindow, WM_TIMER, TIMER_REPEAT, 0);
            Check(inputCalls == before, "late timer cannot send input after Deactivate");
        }
    }
    Check(result == 0, "production deactivation posted WM_QUIT and ended GetMessage");
    Check(completeCombos == 2, "exactly two real timed combo batches");
    Check(!active && !IsWindow(testWindow), "no active state or live test window after loop exit");
    printf("%s: %d failure(s); production activation, 10-minute interval, key descriptors and complete shutdown checked.\n",
           failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}
