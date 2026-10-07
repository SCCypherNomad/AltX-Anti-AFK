# AltX Anti-AFK Windows resources

Created by Cypher Nomad. Copyright (c) 2026 Cypher Nomad.

`AltXTimer.o` is an x64 COFF object for linking with Windows TCC. It embeds:

- Icon group ID 1 with 16, 24, 32, 48, 64, 128 and 256 pixel images.
- Manifest ID 1 with ordinary user privileges, Common Controls v6 and DPI awareness.
- Version information ID 1 for AltX Anti-AFK, with file and product version `1.1.0.0` and Cypher Nomad attribution.

The matching `.ico`, `.manifest`, `.rc` and `.res` files are included. Windows TCC requires the COFF `.o` file; it does not accept the raw `.res` format. Conventional Microsoft RC or MinGW windres builds can use `AltXTimer.rc`.

To regenerate these files on Windows, install Python 3 and Pillow, then run:

```powershell
python .\assets\generate-assets.py
```

The script uses the Windows Segoe UI Bold font to draw the icon and writes every generated file beside itself. The icon and resource files are deterministic. `icon-preview.png` shows the icon sizes and is not linked into the executable. Building with the included object does not require Python or Pillow.

The resource layout follows Microsoft's [Win32 resource format](https://learn.microsoft.com/en-us/windows/win32/menurc/resource-file-formats), [PE/COFF specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), and [VERSIONINFO documentation](https://learn.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource).
