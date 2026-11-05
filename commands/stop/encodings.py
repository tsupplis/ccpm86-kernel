"""Restore same-length alternate 8086 encodings emitted by the release compiler.

Offsets are relative to CODE. Each entry names the mnemonic and both encodings.
The input is the local ASM86/GENCMD output. No reference binary is read here.
"""
import argparse
from pathlib import Path

# (ASM86 encoding, release encoding, CODE offsets)
ENCODINGS = [
    # mov al,ah
    ("8ac4", "88e0", [
        0x118, 0x4a5,
    ]),
    # xor dx,dx
    ("33d2", "31d2", [
        0x178, 0x48e, 0x5e9, 0x5f5, 0x605, 0x611, 0x66f, 0x679,
        0x686, 0x690, 0x69d, 0x6a7, 0x6b4, 0x6be, 0xca2, 0xd34,
        0xd67, 0xd7b,
    ]),
    # mov bx,cx
    ("8bd9", "89cb", [
        0x215,
    ]),
    # mov bx,ax
    ("8bd8", "89c3", [
        0x451, 0x943, 0xa1b, 0xa32, 0xb80, 0xba9, 0xbc0, 0xc87,
        0xc99, 0xcc7, 0xd14, 0xd2b, 0xdbc, 0xdd7,
    ]),
    # mov bx,dx
    ("8bda", "89d3", [
        0x492,
    ]),
    # mov ax,dx
    ("8bc2", "89d0", [
        0x681, 0x698, 0x6af,
    ]),
    # mov si,ax
    ("8bf0", "89c6", [
        0x89e, 0x908, 0x979, 0xb00, 0xc21,
    ]),
    # mov ax,cx
    ("8bc1", "89c8", [
        0x8b1, 0xb76,
    ]),
    # cmp ax,2
    ("3d0200", "83f802", [
        0x9ec, 0xb68,
    ]),
    # mov si,cx
    ("8bf1", "89ce", [
        0xa11, 0xb9f, 0xdc0,
    ]),
]

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('command', type=Path)
path = parser.parse_args().command
image = bytearray(path.read_bytes())
if len(image) != 7168 or image[:3] != bytes.fromhex('01e100'):
    raise ValueError('Unexpected STOP command layout')
for assembler, release, sites in ENCODINGS:
    before, after = bytes.fromhex(assembler), bytes.fromhex(release)
    assert len(before) == len(after)
    for offset in sites:
        start = 128 + offset
        end = start + len(before)
        if image[start:end] != before:
            raise ValueError(f'Unexpected ASM86 instruction at CODE:{offset:04X}')
        image[start:end] = after
path.write_bytes(image)
