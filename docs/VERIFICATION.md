# AltX Anti-AFK verification

AltX Anti-AFK uses a native Windows message loop and timer. The integration harness exercises the application's timer and input-building code while intercepting `SendInput`, foreground selection, and window focus operations. It controls a hidden test window and does not inject real keyboard input.

## Game test report

I verified AltX Anti-AFK v1.1.1 working on Windows 11 with Star Citizen 4.10.1 on 7 October 2026, with the game in the foreground and receiving keyboard input.

I also verified the original v1.0.0 release on the same Windows and game version on 7 October 2026, with Star Citizen in the foreground and receiving keyboard input. These are my personal game tests as SCCypherNomad and cover those releases and that Windows/game combination.

My original v1.0.0 game test predates the v1.1.0 visual refresh. Version 1.1.0 updates the interface to a dark space look with blue and cyan neon accents while retaining the original four-minute repeat. A separate live game test of v1.1.0 has not been recorded. Version 1.1.1 corrects the branding to SCCypherNomad, clarifies the foreground requirement, and changes the repeat interval to ten minutes. Both preserve the three-second first press, keyboard input method, and complete exit on Deactivate.

## Foreground requirement and withdrawn experiment

The released program sends standard Windows keyboard input to the foreground application. Star Citizen must have keyboard focus at every scheduled press. Minimizing the game, Alt+Tabbing, clicking a browser, or focusing another window prevents the shortcut from reaching Star Citizen and may send it to that other application. Windowed or borderless mode alone does not satisfy this requirement.

On 7 October 2026, I tested a separate local experimental build that sent window messages directly to the Star Citizen window. It did not work when the game was unfocused. Its status only indicated that Windows queued the messages, not that the game accepted them. This failed method does not establish that all possible background-input methods are impossible. The experiment was withdrawn, and background or minimized operation is not a supported public mode.

## Recorded harness results

The v1.1.0 production integration run on 7 October 2026 completed with **0 failures**, matching the earlier run, and confirmed:

- The first timer fired after **3,000 ms**.
- The next timer fired after **240,000 ms**.
- Each complete sequence was **Left Alt down, X down, X up, Left Alt up**.
- Input descriptors used the expected scan codes and release flags.
- Partial `SendInput` results of 0, 1, 2, or 3 events triggered the appropriate key-release handling.
- A failed cleanup displayed an instruction to release the keys manually.
- Stale timer messages sent no input after the timer was stopped.
- Activation and complete shutdown behaved as expected.

That v1.1.0 test used the actual three-second and four-minute timer intervals. It did not replace them with shorter intervals.

The v1.1.1 full-duration integration run on 7 October 2026 completed with **0 failures** and measured:

- The initial timer fired after **3,016 ms** for the configured **3,000 ms** delay.
- The repeat timer fired after exactly **600,000 ms**, or **ten minutes**.
- Keyboard input construction, partial-input cleanup, stale-timer suppression, activation, and complete shutdown checks passed.

Keyboard input and focus APIs were intercepted during the run. The harness did not send shortcuts to Star Citizen or any other application, and these results do not establish a new live game test.

## Reproduce the check

Use Tiny C Compiler 0.9.27 for Windows x64 and run this from the repository root:

```powershell
.\scripts\Test.ps1 -TccPath 'C:\Tools\tcc\tcc.exe'
```

Allow slightly more than ten minutes for the current release's full timing check. A successful run reports **0 failures**. The compiler is needed only for building and testing, not for running the release executable.

## What these results establish

The automated harness checks establish the timer schedule, generated input descriptors, partial-input handling, and shutdown behavior. The recorded runs cover v1.1.0's four-minute schedule and v1.1.1's ten-minute schedule. The harness intercepts keyboard injection and does not send input to Star Citizen or any other external application. My separate personal game tests above verify the original v1.0.0 release and current v1.1.1 release on Windows 11 with Star Citizen 4.10.1 in the foreground. Whether another application accepts standard Windows simulated input depends on that application and its execution privileges.

The release executable is unsigned. A release checksum checks file integrity against the published checksum; it is not a Windows publisher certificate.
