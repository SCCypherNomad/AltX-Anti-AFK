# Verification

AltX uses a native Windows message loop and timer. The integration harness exercises the application's timer and input-building code while intercepting `SendInput`, foreground selection, and window focus operations. It controls a hidden test window and does not inject real keyboard input.

## Recorded results

The initial production integration run completed with **0 failures** and confirmed:

- The first timer fired after **3,000 ms**.
- The next timer fired after **240,000 ms**.
- Each complete sequence was **Left Alt down, X down, X up, Left Alt up**.
- Input descriptors used the expected scan codes and release flags.
- Partial `SendInput` results of 0, 1, 2, or 3 events triggered the appropriate key-release handling.
- A failed cleanup displayed an instruction to release the keys manually.
- Stale timer messages sent no input after the timer was stopped.
- Activation and complete shutdown behaved as expected.

The test used the actual three-second and four-minute timer intervals. It did not replace them with shorter intervals.

## Reproduce the check

Use Tiny C Compiler 0.9.27 for Windows x64 and run this from the repository root:

```powershell
.\scripts\Test.ps1 -TccPath 'C:\Tools\tcc\tcc.exe'
```

Allow slightly more than four minutes for the full timing check. A successful run reports **0 failures**. The compiler is needed only for building and testing, not for running the release executable.

## What these results establish

These checks establish the timer schedule, generated input descriptors, partial-input handling, and shutdown behavior in the test harness. They do not establish compatibility with Star Citizen or any other external application. Whether an application accepts standard Windows simulated input depends on that application and its execution privileges.

The release executable is unsigned. A release checksum checks file integrity against the published checksum; it is not a Windows publisher certificate.
