# AltX Anti-AFK

**Take a break. Stay in the 'verse.**

Tired of losing an active mission to an inactivity logout while you take a bathroom break or walk the dog? Waiting out a prison sentence and need a break from the keyboard?

Meet **AltX Anti-AFK**, a free, lightweight Windows utility by **SCCypherNomad** for the Star Citizen community. It sends the **helmet-wipe shortcut, Left Alt + X**, every **ten minutes** to help keep your avatar active while you step away.

**Two buttons. No installer. A tiny 54 KB program.** Click **Activate**, return focus to Star Citizen within **ten seconds**, and AltX sends the first shortcut, then repeats every ten minutes. Click **Deactivate** to stop and close the program completely.

[**Download AltX for Windows**](https://github.com/SCCypherNomad/AltX-Anti-AFK/releases/latest/download/AltX-windows-x64.zip)

**Star Citizen must remain the foreground window with keyboard focus. This release does not operate the game in the background or while minimized.** If you Alt+Tab, click your browser, or focus another window, the shortcut may reach that application instead of the game. Windowed or borderless mode alone does not keep the game focused.

AltX Anti-AFK uses standard Windows keyboard input and has no game integration. Deactivate it before switching away from Star Citizen.

A dark space look with blue and cyan neon accents gives AltX Anti-AFK a fresh cockpit feel. The minimal window keeps **Activate** and **Deactivate** easy to find.

![AltX Anti-AFK dark space interface with blue neon accents and SCCypherNomad branding](assets/ui-preview.jpg)

## Download

Open the [latest release](https://github.com/SCCypherNomad/AltX-Anti-AFK/releases/latest) and download:

- [AltX-windows-x64.zip](https://github.com/SCCypherNomad/AltX-Anti-AFK/releases/latest/download/AltX-windows-x64.zip): the ready-to-run Windows program and instructions.
- [AltX-source.zip](https://github.com/SCCypherNomad/AltX-Anti-AFK/releases/latest/download/AltX-source.zip): source code, assets, and build scripts.
- [SHA256SUMS.txt](https://github.com/SCCypherNomad/AltX-Anti-AFK/releases/latest/download/SHA256SUMS.txt): checksums for the release downloads.

Choose the Windows ZIP to use the program. You do not need a compiler or Python to run it.

## Setup

1. Use Windows 10 or Windows 11, 64-bit.
2. Download **AltX-windows-x64.zip** and choose **Extract All**.
3. Put the extracted folder somewhere you intend to keep it.
4. Open **AltXTimer.exe**.

No installer, .NET runtime, administrator access, background service, or network connection is needed to run AltX Anti-AFK. It starts stopped each time you open it.

The program is credited to SCCypherNomad. The executable is **unsigned**: this authorship credit is separate from a Windows digital signature or verified publisher status.

## Use

1. Open Star Citizen first.
2. Open **AltXTimer.exe** and click **Activate**. Its window minimizes to the taskbar. Return keyboard focus to Star Citizen within **ten seconds**, then keep it focused. Do not minimize it, Alt+Tab, click your browser, or focus another window. Windowed or borderless mode still requires focus.
3. AltX Anti-AFK presses and releases **Left Alt + X** once, then repeats every **ten minutes** while active.
4. Before browsing or switching to another application, click the AltX taskbar icon to restore its window, then click **Deactivate**. The timer stops and the program exits.

The title-bar close button and **Escape** also stop and exit the program.

Every scheduled press goes to the application in the foreground at that moment. If another application has focus, it may receive **Left Alt + X**. AltX Anti-AFK skips a press if its own window has focus or if you are holding Alt, Ctrl, Shift, a Windows key, or X. It tries again at the next interval.

## Pin the taskbar pictogram

Open AltX Anti-AFK, right-click its taskbar icon, and choose **Pin to taskbar**. You can then reopen it from the pinned icon and click **Activate**.

The pinned icon is a shortcut and remains available after the program closes. Keep the executable in its chosen folder. If you move it, pin it again from the new location.

## Resource use

**Deactivate closes the process**, so AltX Anti-AFK itself uses no RAM or CPU afterward. An open, stopped window still occupies a small amount of memory. It has no inactive timer and waits for Windows messages instead of polling.

While active, the timer wakes for the initial ten-second press and subsequent ten-minute intervals. AltX Anti-AFK does not install automatic startup or leave a hidden process after closing.

## Compatibility and verification

AltX Anti-AFK sends input through the Windows [SendInput API](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput). Applications with higher privileges and some games may reject simulated keyboard input. AltX Anti-AFK does not wake a sleeping or locked computer or build up a queue of missed presses.

**I verified AltX Anti-AFK working on Windows 11 with Star Citizen 4.10.1 on 7 October 2026, with the game in the foreground and receiving keyboard input.** This confirmation applies to that Windows and game version.

I chose the ten-minute repeat after observing an inactivity logout after about fifteen minutes in my own testing.

The timer and key sequence have also been checked with an integration harness that intercepts keyboard injection. That harness verifies timing and generated input descriptors without sending input to the game. See [verification details](docs/VERIFICATION.md) for my live game verification and the automated checks.

Only one AltX Anti-AFK instance runs at a time. Launching it again restores the existing window.

## Build from source

Install **Tiny C Compiler 0.9.27 for Windows x64**, then open PowerShell in the source folder:

```powershell
.\scripts\Build.ps1 -TccPath 'C:\Tools\tcc\tcc.exe'
```

Replace the example compiler path with your own. The repository includes generated resources for the normal build. The asset generator uses Python 3 and Pillow if you want to regenerate those resources.

Run the integration checks with:

```powershell
.\scripts\Test.ps1 -TccPath 'C:\Tools\tcc\tcc.exe'
```

The full timing check takes slightly more than ten minutes. It intercepts keyboard injection so the test does not send shortcuts to your applications.

## Help fellow Star Citizens find AltX

If AltX makes your breaks less frustrating, help spread the word:

- **Star the [repository](https://github.com/SCCypherNomad/AltX-Anti-AFK)** using GitHub's **Star** button to show your support and save the project.
- **[Suggest improvements or report a problem](https://github.com/SCCypherNomad/AltX-Anti-AFK/issues/new)**. Tell us what you would like to see, or share which Windows and Star Citizen versions you tested.
- **Support other players' suggestions** with a thumbs-up reaction on their [issues](https://github.com/SCCypherNomad/AltX-Anti-AFK/issues).
- **Share AltX** with your friends, organization mates, other Star Citizens, and on your socials. Share the [project](https://github.com/SCCypherNomad/AltX-Anti-AFK) or [latest release](https://github.com/SCCypherNomad/AltX-Anti-AFK/releases/latest) link so everyone can find the current downloads and instructions.

Made by **SCCypherNomad** for fellow Star Citizens. See you in the 'verse!

Copyright (c) 2026 SCCypherNomad. Released under the [MIT License](LICENSE).
