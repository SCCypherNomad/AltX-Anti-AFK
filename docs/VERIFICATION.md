# AltX Anti-AFK verification

## Live game check

I verified AltX Anti-AFK working on Windows 11 with Star Citizen 4.10.1 on 7 October 2026, with the game in the foreground and receiving keyboard input.

My check confirmed the **Left Alt + X** helmet-wipe action for anti-AFK use. This confirmation covers that Windows/game combination and focused game operation. The repeat interval is **ten minutes**.

## Foreground requirement

Star Citizen must stay in the foreground with keyboard focus while AltX is active. Minimizing the game, Alt+Tabbing, clicking a browser, or focusing another window prevents the shortcut from reaching Star Citizen and may send it to that other application. Windowed or borderless mode alone does not keep keyboard focus. Deactivate AltX before switching away from the game.

## Automated checks

The integration harness uses a hidden Windows test window and intercepts keyboard input and focus APIs. It checks timer scheduling, the **Left Alt down, X down, X up, Left Alt up** sequence, scan codes and release flags, partial-input cleanup, stale-timer suppression, activation, and complete shutdown. It sends no shortcuts to Star Citizen or any other application.

- Startup delay: measured at exactly **10,000 ms**, allowing **ten seconds** to return focus to Star Citizen after activation. The startup-only run completed with **0 failures**, including input descriptors, partial-input cleanup, stale-timer suppression, and complete shutdown checks.
- Repeat interval: measured at exactly **600,000 ms**, or **ten minutes**, in a separate full-duration integration run using the unchanged repeat-timer routine.
- That full-duration run completed with **0 failures**, including input construction, partial-input cleanup, stale-timer suppression, and complete shutdown checks.

The automated checks establish application behavior and timing. My live game check above establishes the separate helmet-wipe and anti-AFK result in Star Citizen.

## Reproduce the check

Use Tiny C Compiler 0.9.27 for Windows x64 and run this from the repository root:

```powershell
.\scripts\Test.ps1 -TccPath 'C:\Tools\tcc\tcc.exe'
```

Allow slightly more than ten minutes for the full timing check. A successful run reports **0 failures**. The compiler is needed only for building and testing, not for running the release executable.

For the shorter startup and input-handling check, run:

```powershell
.\scripts\Test.ps1 -TccPath 'C:\Tools\tcc\tcc.exe' -StartupOnly
```

This shorter check uses the actual ten-second startup delay and skips the ten-minute repeat wait.

The release executable is unsigned. A release checksum checks file integrity against the published checksum; it is not a Windows publisher certificate.
