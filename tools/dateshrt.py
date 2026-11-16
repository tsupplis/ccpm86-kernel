#!/usr/bin/env python3
"""Run linked DATE/TOD CMDs with a private clock and intercepted BDOS calls."""
from datetime import date
from pathlib import Path
import struct
import sys
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def check():
    for name in (sys.argv[1:] or ('date', 'tod')):
        command = ROOT / 'commands' / name / (name + '.cmd')
        if not command.parent.exists():
            continue
        raw = command.read_bytes()
        assert raw[0] == 1 and raw[9] == 2
        code_size = struct.unpack_from('<H', raw, 1)[0] * 16
        data_size = struct.unpack_from('<H', raw, 10)[0] * 16
        count = 0
        with tempfile.TemporaryDirectory(prefix='date-short-') as td:
            folder = Path(td)
            (folder / 'code.bin').write_bytes(raw[128:128 + code_size])
            (folder / 'data.bin').write_bytes(raw[128 + code_size:128 + code_size + data_size])
            cases = [(year, 1, 2) for year in range(100)] + [(0, 2, 29), (80, 2, 29)]
            for short, month, day in cases:
                year = 2000 + short if short < 80 else 1900 + short
                days = (date(year, month, day) - date(1978, 1, 1)).days + 1
                tail = f'{month:02}/{day:02}/{short:02} 12:34:56'
                (folder / 'test.asm').write_text(f'''bits 16
cpu 8086
org 100h
 cld
 mov ax,2000h
 mov es,ax
 mov si,code
 xor di,di
 mov cx,{code_size}
 rep movsb
 mov ax,3000h
 mov es,ax
 mov si,data
 xor di,di
 mov cx,{data_size}
 rep movsb
 mov byte [es:5dh],'0'
 mov si,tail
 mov di,81h
 mov cx,{len(tail)}
 rep movsb
 mov byte [es:80h],{len(tail)}
 mov ax,4000h
 mov es,ax
 mov word [es:7eh],0
 mov dx,bdos
 mov ax,25e0h
 int 21h
 mov ax,3000h
 mov ds,ax
 mov es,ax
 jmp 2000h:0
bdos:
 cmp cl,0
 je finished
 cmp cl,12
 je version
 cmp cl,154
 je sysdat
 cmp cl,1
 je input
 cmp cl,2
 je output
 cmp cl,9
 je output
 jmp failed
version:
 mov ax,1431h
 mov bx,ax
 iret
sysdat:
 mov ax,4000h
 mov es,ax
 xor bx,bx
 iret
input:
 mov ax,13
 mov bx,ax
 iret
output:
 iret
finished:
 mov ax,4000h
 mov es,ax
 cmp word [es:7eh],{days}
 jne failed
 cmp word [es:80h],3412h
 jne failed
 cmp byte [es:82h],56h
 jne failed
 mov ax,4c00h
 int 21h
failed:
 mov ax,4c01h
 int 21h
tail db '{tail}'
code: incbin 'code.bin'
data: incbin 'data.bin'
''')
                subprocess.run(['nasm', '-f', 'bin', 'test.asm', '-o', 'test.com'], cwd=folder, check=True)
                result = subprocess.run(['emu2', 'test.com'], cwd=folder, capture_output=True, timeout=5)
                assert result.returncode == 0, (name, tail, year, result.stdout, result.stderr)
                count += 1
        print(f'PASS: {name.upper()}.CMD: {count} short-year/calendar cases; private clock only')


if __name__ == '__main__':
    check()
