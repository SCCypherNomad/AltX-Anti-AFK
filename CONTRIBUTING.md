# Contributing

AltX is intentionally small: a native Windows utility with an Activate button, a Deactivate button, and a four-minute Left Alt + X timer.

For a bug report, include your Windows version, keyboard layout, the steps to reproduce it, and what happened. State whether you tested the release executable or built from source.

For a code change, explain the behavior it fixes and run `scripts/Test.ps1` with a Windows x64 Tiny C Compiler installation. The full timer test takes slightly more than four minutes and intercepts keyboard injection. See [verification details](docs/VERIFICATION.md).

Keep changes focused, preserve complete key-release and shutdown behavior, and avoid introducing polling, network dependencies, or automatic startup.

Contributions are made under the project's [MIT License](LICENSE).
