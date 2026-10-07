"""Build AltX Timer icons, manifest, version information and Windows resources.

Copyright (c) 2026 Cypher Nomad.

Run with Python 3 and Pillow: python generate-assets.py
All output is placed beside this script. The .res uses RESOURCEHEADER layout.
The x64 COFF .o wraps a PE resource tree for direct linking with Windows TCC.

Format reference:
https://learn.microsoft.com/en-us/windows/win32/menurc/resource-file-formats
https://learn.microsoft.com/en-us/windows/win32/menurc/resourceheader
https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
https://learn.microsoft.com/en-us/windows/win32/menurc/versioninfo-resource
"""

from io import BytesIO
from pathlib import Path
import os
import struct
import xml.etree.ElementTree as ET

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
SIZES = (16, 24, 32, 48, 64, 128, 256)
FONT_PATH = Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts/segoeuib.ttf"
VERSION = (1, 0, 0, 0)
VERSION_STRINGS = {
    "CompanyName": "Cypher Nomad",
    "FileDescription": "AltX Timer by Cypher Nomad",
    "FileVersion": "1.0.0.0",
    "InternalName": "AltXTimer",
    "LegalCopyright": "Copyright (c) 2026 Cypher Nomad",
    "OriginalFilename": "AltXTimer.exe",
    "ProductName": "AltX",
    "ProductVersion": "1.0.0.0",
}
MANIFEST = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity type="win32" name="AltX.Timer.App" version="1.0.0.0" processorArchitecture="*" />
  <description>AltX Timer</description>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type="win32" name="Microsoft.Windows.Common-Controls" version="6.0.0.0" processorArchitecture="*" publicKeyToken="6595b64144ccf1df" language="*" />
    </dependentAssembly>
  </dependency>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="asInvoker" uiAccess="false" />
      </requestedPrivileges>
    </security>
  </trustInfo>
  <application xmlns="urn:schemas-microsoft-com:asm.v3">
    <windowsSettings>
      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true</dpiAware>
    </windowsSettings>
  </application>
  <compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
    <application>
      <supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}" />
    </application>
  </compatibility>
</assembly>
'''


def centered_text(draw, text, font, center_x, top, fill):
    box = draw.textbbox((0, 0), text, font=font)
    width = box[2] - box[0]
    draw.text((center_x - width / 2 - box[0], top - box[1]), text, font=font, fill=fill)


def draw_icon(size):
    # Each size is drawn independently at 4x resolution so small taskbar images
    # have appropriate lettering instead of an indiscriminately shrunken image.
    scale = 4
    n = size * scale
    out = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    draw = ImageDraw.Draw(out)
    inset = max(1, size * .035) * scale
    draw.rounded_rectangle(
        (inset, inset, n - inset - 1, n - inset - 1),
        radius=size * .19 * scale,
        fill=(18, 29, 48, 255),
        outline=(58, 78, 102, 255),
        width=max(scale, round(size * .028 * scale)),
    )
    # The short light line reads as the raised face of a keyboard key.
    line_y = size * .86 * scale
    draw.rounded_rectangle(
        (size * .25 * scale, line_y, size * .75 * scale, line_y + max(1, size * .025) * scale),
        radius=scale,
        fill=(45, 62, 85, 255),
    )
    alt_font = ImageFont.truetype(str(FONT_PATH), round(size * .28 * scale))
    x_font = ImageFont.truetype(str(FONT_PATH), round(size * .44 * scale))
    centered_text(draw, "ALT", alt_font, n / 2, size * .15 * scale, (244, 248, 253, 255))
    centered_text(draw, "X", x_font, n / 2, size * .43 * scale, (45, 211, 229, 255))
    return out.resize((size, size), Image.Resampling.LANCZOS)


def png_bytes(img):
    stream = BytesIO()
    img.save(stream, format="PNG", optimize=True)
    return stream.getvalue()


def ordinal(value):
    return struct.pack("<HH", 0xFFFF, value)


def pad4(data):
    return data + b"\0" * (-len(data) % 4)


def resource(resource_type, resource_id, data, flags=0x1030):
    # Numeric identifiers keep each resource header exactly 32 bytes.
    names = ordinal(resource_type) + ordinal(resource_id)
    header = struct.pack("<II", len(data), 32) + names
    header += struct.pack("<IHHII", 0, flags, 0, 0, 0)
    assert len(header) == 32
    return header + pad4(data)


def build_ico(images):
    offset = 6 + 16 * len(images)
    entries = []
    payloads = []
    for size, data in images:
        dimension = size if size < 256 else 0
        entries.append(struct.pack("<BBBBHHII", dimension, dimension, 0, 0, 1, 32, len(data), offset))
        payloads.append(data)
        offset += len(data)
    return struct.pack("<HHH", 0, 1, len(images)) + b"".join(entries) + b"".join(payloads)


def version_block(key, value=b"", children=(), value_type=0, value_length=None):
    """Pack a DWORD-aligned version block with UTF-16 key and optional value."""
    if value_length is None:
        value_length = len(value) if value_type == 0 else len(value) // 2
    block = bytearray(pad4(b"\0" * 6 + (key + "\0").encode("utf-16le")))
    block.extend(value)
    for child in children:
        block = bytearray(pad4(block))
        block.extend(child)
    struct.pack_into("<HHH", block, 0, len(block), value_length, value_type)
    return bytes(block)


def build_versioninfo():
    """Create a VS_VERSION_INFO resource, including translation 0409/04B0."""
    major, minor, patch, build = VERSION
    version_ms = (major << 16) | minor
    version_ls = (patch << 16) | build
    fixed = struct.pack(
        "<13I", 0xFEEF04BD, 0x00010000, version_ms, version_ls,
        version_ms, version_ls, 0x3F, 0, 0x00040004, 1, 0, 0, 0,
    )
    strings = [
        version_block(key, (value + "\0").encode("utf-16le"), value_type=1)
        for key, value in VERSION_STRINGS.items()
    ]
    table = version_block("040904B0", children=strings, value_type=1)
    string_info = version_block("StringFileInfo", children=[table], value_type=1)
    translation = version_block("Translation", struct.pack("<HH", 0x0409, 1200))
    var_info = version_block("VarFileInfo", children=[translation], value_type=1)
    return version_block("VS_VERSION_INFO", fixed, [string_info, var_info])


def build_rc():
    """Resource script for conventional Microsoft RC or MinGW windres builds."""
    lines = [
        '// Copyright (c) 2026 Cypher Nomad.',
        '1 ICON "AltXTimer.ico"',
        '1 24 "AltXTimer.manifest"',
        '',
        '1 VERSIONINFO',
        'FILEVERSION 1,0,0,0',
        'PRODUCTVERSION 1,0,0,0',
        'FILEFLAGSMASK 0x3f',
        'FILEFLAGS 0x0',
        'FILEOS 0x40004',
        'FILETYPE 0x1',
        'FILESUBTYPE 0x0',
        'BEGIN',
        '    BLOCK "StringFileInfo"',
        '    BEGIN',
        '        BLOCK "040904B0"',
        '        BEGIN',
    ]
    for key, value in VERSION_STRINGS.items():
        lines.append(f'            VALUE "{key}", "{value}\\0"')
    lines.extend([
        '        END',
        '    END',
        '    BLOCK "VarFileInfo"',
        '    BEGIN',
        '        VALUE "Translation", 0x409, 1200',
        '    END',
        'END',
        '',
    ])
    return "\n".join(lines)


def build_res(images, manifest, versioninfo):
    # Standard initial zero resource marks this as a Win32 resource file.
    entries = [resource(0, 0, b"", flags=0)]
    group = struct.pack("<HHH", 0, 1, len(images))
    for resource_id, (size, data) in enumerate(images, start=1):
        dimension = size if size < 256 else 0
        entries.append(resource(3, resource_id, data))  # RT_ICON
        group += struct.pack("<BBBBHHIH", dimension, dimension, 0, 0, 1, 32, len(data), resource_id)
    entries.append(resource(14, 1, group))  # RT_GROUP_ICON
    entries.append(resource(16, 1, versioninfo))  # RT_VERSION
    entries.append(resource(24, 1, manifest))  # RT_MANIFEST
    return b"".join(entries)


def verify_res(res, images, manifest, versioninfo):
    entries = []
    pos = 0
    while pos < len(res):
        data_size, header_size = struct.unpack_from("<II", res, pos)
        type_marker, resource_type, name_marker, resource_id = struct.unpack_from("<HHHH", res, pos + 8)
        assert header_size == 32 and type_marker == name_marker == 0xFFFF
        data = res[pos + header_size : pos + header_size + data_size]
        entries.append((resource_type, resource_id, data))
        pos += header_size + ((data_size + 3) & ~3)
    assert pos == len(res)
    assert entries[0] == (0, 0, b"")
    for resource_id, (_, data) in enumerate(images, start=1):
        assert entries[resource_id] == (3, resource_id, data)
    assert entries[-3][:2] == (14, 1)
    assert entries[-2] == (16, 1, versioninfo)
    assert entries[-1] == (24, 1, manifest)
    assert len(entries[-3][2]) == 6 + 14 * len(images)
    ET.fromstring(manifest)


def build_coff(images, manifest, versioninfo):
    """Write one x64 COFF .rsrc section with ADDR32NB relocations.

    Each leaf is indexed by Type, ID, then neutral language 0. Directory
    pointers are relative to the section. Only data RVAs need relocations.
    """
    group = struct.pack("<HHH", 0, 1, len(images))
    icons = {}
    for resource_id, (size, data) in enumerate(images, start=1):
        dimension = size if size < 256 else 0
        icons[resource_id] = {0: data}
        group += struct.pack("<BBBBHHIH", dimension, dimension, 0, 0, 1, 32, len(data), resource_id)
    tree = {3: icons, 14: {1: {0: group}}, 16: {1: {0: versioninfo}}, 24: {1: {0: manifest}}}
    section = bytearray()
    leaves = []

    def write_directory(children):
        offset = len(section)
        section.extend(struct.pack("<IIHHHH", 0, 0, 0, 0, 0, len(children)))
        section.extend(b"\0" * (8 * len(children)))
        for index, (key, value) in enumerate(sorted(children.items())):
            entry_offset = offset + 16 + index * 8
            if isinstance(value, dict):
                target = write_directory(value) | 0x80000000
                struct.pack_into("<II", section, entry_offset, key, target)
            else:
                # Data descriptor offsets are assigned after every directory.
                struct.pack_into("<I", section, entry_offset, key)
                leaves.append((entry_offset + 4, value))
        return offset

    assert write_directory(tree) == 0
    descriptors = []
    for pointer_offset, data in leaves:
        descriptor_offset = len(section)
        struct.pack_into("<I", section, pointer_offset, descriptor_offset)
        section.extend(b"\0" * 16)
        descriptors.append((descriptor_offset, data))
    relocations = []
    for descriptor_offset, data in descriptors:
        data_offset = len(section)
        struct.pack_into("<IIII", section, descriptor_offset, data_offset, len(data), 0, 0)
        section.extend(pad4(data))
        # Symbol 0 identifies the start of .rsrc. ADDR32NB adds the section RVA.
        relocations.append(struct.pack("<IIH", descriptor_offset, 0, 0x0003))

    raw_offset = 20 + 40
    relocation_offset = raw_offset + len(section)
    symbol_offset = relocation_offset + 10 * len(relocations)
    header = struct.pack("<HHIIIHH", 0x8664, 1, 0, symbol_offset, 2, 0, 0)
    section_header = struct.pack(
        "<8sIIIIIIHHI", b".rsrc\0\0\0", 0, 0, len(section), raw_offset,
        relocation_offset, 0, len(relocations), 0, 0x40300040,
    )
    # IMAGE_SYM_CLASS_STATIC section symbol plus its section-definition aux.
    symbol = struct.pack("<8sIhHBB", b".rsrc\0\0\0", 0, 1, 0, 3, 1)
    aux = struct.pack("<IHHIhB3s", len(section), len(relocations), 0, 0, 0, 0, b"\0\0\0")
    assert len(header) == 20 and len(section_header) == 40
    assert len(symbol) == len(aux) == 18
    string_table = struct.pack("<I", 4)
    return header + section_header + section + b"".join(relocations) + symbol + aux + string_table


def main():
    ROOT.mkdir(parents=True, exist_ok=True)
    images = [(size, png_bytes(draw_icon(size))) for size in SIZES]
    ico = build_ico(images)
    manifest = MANIFEST.encode("utf-8")
    versioninfo = build_versioninfo()
    res = build_res(images, manifest, versioninfo)
    coff = build_coff(images, manifest, versioninfo)
    verify_res(res, images, manifest, versioninfo)
    (ROOT / "AltXTimer.ico").write_bytes(ico)
    (ROOT / "AltXTimer.manifest").write_bytes(manifest)
    (ROOT / "AltXTimer.res").write_bytes(res)
    (ROOT / "AltXTimer.o").write_bytes(coff)
    (ROOT / "AltXTimer.rc").write_text(build_rc(), encoding="utf-8")
    # A preview file is only for visual verification and is not linked.
    preview = Image.new("RGB", (660, 300), (236, 239, 244))
    pd = ImageDraw.Draw(preview)
    label_font = ImageFont.truetype(str(FONT_PATH), 12)
    x = 12
    for size in (16, 24, 32, 48, 64, 128, 256):
        image = draw_icon(size)
        preview.paste(image, (x, 28), image)
        pd.text((x, 8), str(size), font=label_font, fill=(18, 29, 48))
        x += size + 8
    preview.save(ROOT / "icon-preview.png")
    print(f"Created {len(images)} icon sizes: {', '.join(map(str, SIZES))}")
    print(f"ICO: {len(ico):,} bytes; RES: {len(res):,} bytes; COFF: {len(coff):,} bytes")
    print("Verified icon, manifest, version-information payload and resource alignment.")


if __name__ == "__main__":
    main()
