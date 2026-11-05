#!/usr/bin/env python3
"""Execute built SUBMIT with synthetic BDOS file/CLI services under emu2.

The actual command performs reading, lookahead, expansion, echo and directives.
Only OS services are stubbed; CLI execution emits a RUN marker rather than
launching another application. NASM assembles only the disposable test driver.
"""

from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
CON = ROOT / "commands/submit"


def run(script, tail=b"CASE"):
    symbols = {
        n: int(v, 16)
        for v, n in re.findall(
            r"\b([0-9A-F]{4})\s+([A-Z][A-Z0-9]*)", (CON / "submit.sym").read_text()
        )
    }
    image = (CON / "submit.cmd").read_bytes()
    size = int.from_bytes(image[1:3], "little") * 16
    code = bytearray(image[128 : 128 + size])
    data = bytearray(image[128 + size :])
    data.extend(bytes(4096 - len(data)))
    data[0x908:0x910] = b"SUBMIT  "
    data[0x80] = len(tail)
    data[0x81 : 0x82 + len(tail)] = tail + b"\0"
    data[0x5C:0x68] = b"\0CASE    SUB"
    at = symbols["MPM"]
    code[at : at + 3] = b"\xe9" + ((size - at - 3) & 65535).to_bytes(2, "little")
    with tempfile.TemporaryDirectory(prefix="submit-check-") as tmp:
        folder = Path(tmp)
        (folder / "code.bin").write_bytes(code)
        (folder / "data.bin").write_bytes(data)
        (folder / "script.bin").write_bytes(script)
        (folder / "services.asm").write_text(f"""bits 16
cpu 8086
org {size}
service:
 cmp cl,0
 je terminate
 cmp cl,143
 je terminate
 cmp cl,12
 je version
 cmp cl,156
 je pd
 cmp cl,32
 je user
 cmp cl,25
 je disk
 cmp cl,14
 je select_disk
 cmp cl,9
 je print_string
 cmp cl,2
 je print_char
 cmp cl,15
 je success
 cmp cl,16
 je success
 cmp cl,20
 je read_record
 cmp cl,152
 je parse
 cmp cl,150
 je execute
 cmp cl,47
 je chain
 cmp cl,26
 je success
 cmp cl,145
 je success
 cmp cl,146
 je success
 cmp cl,153
 je success
 mov ax,4c02h
 int 21h
terminate:
 mov ax,4c00h
 int 21h
version:
 mov bx,1400h
 ret
pd:
 push ds
 pop es
 mov bx,0900h
 ret
user:
 cmp dl,0ffh
 je user_query
 mov [cs:user_number],dl
user_query:
 xor bx,bx
 mov bl,[cs:user_number]
 mov ax,bx
 ret
disk:
 xor bx,bx
 mov bl,[cs:drive_number]
 mov ax,bx
 ret
select_disk:
 mov [cs:drive_number],dl
 jmp success
print_string:
 push ax
 mov ah,9
 int 21h
 pop ax
 ret
print_char:
 push ax
 mov ah,2
 int 21h
 pop ax
 ret
success:
 xor ax,ax
 xor bx,bx
 xor cx,cx
 ret
read_record:
 mov si,[cs:position]
 cmp si,script_end-script
 jae eof
 mov di,{symbols['DMA']}
 mov cx,128
read_byte:
 mov al,1ah
 cmp si,script_end-script
 jae pad
 mov al,[cs:script+si]
 inc si
pad:
 mov [di],al
 inc di
 loop read_byte
 mov [cs:position],si
 jmp success
eof:
 mov al,1
 ret
parse:
 mov bx,dx
 mov si,[bx]
 mov di,[bx+2]
 push di
 push ds
 pop es
 xor ax,ax
 mov cx,36
 rep stosb
 pop di
 inc di
 push di
 mov al,' '
 mov cx,11
 rep stosb
 pop di
 mov cx,8
parse_char:
 lodsb
 or al,al
 jz parse_end
 cmp al,' '
 je parse_tail
 cmp al,9
 je parse_tail
 cmp al,'a'
 jb parse_store
 cmp al,'z'
 ja parse_store
 sub al,20h
parse_store:
 stosb
 loop parse_char
parse_skip:
 lodsb
 or al,al
 jz parse_end
 cmp al,' '
 je parse_tail
 cmp al,9
 jne parse_skip
parse_tail:
 lodsb
 cmp al,' '
 je parse_tail
 cmp al,9
 je parse_tail
 or al,al
 jz parse_end
 lea bx,[si-1]
 xor cx,cx
 ret
parse_end:
 xor bx,bx
 xor cx,cx
 ret
execute:
 mov si,dx
 inc si
 call run_marker
 jmp success
chain:
 mov si,dx
 call run_marker
 jmp terminate
run_marker:
 push ds
 push si
 push cs
 pop ds
 mov dx,marker
 mov ah,9
 int 21h
 pop si
 pop ds
run_char:
 lodsb
 or al,al
 jz run_end
 mov dl,al
 mov ah,2
 int 21h
 jmp run_char
run_end:
 mov dl,'>'
 mov ah,2
 int 21h
 mov dl,10
 int 21h
 ret
marker: db '<RUN:','$'
position: dw 0
user_number: db 0
drive_number: db 0
script: incbin "script.bin"
script_end:
""")
        (folder / "test.asm").write_text("""bits 16
cpu 8086
org 100h
 mov ax,cs
 add ax,(data-$$+100h)/16
 mov ds,ax
 mov es,ax
 mov ax,cs
 add ax,(code-$$+100h)/16
 push ax
 xor ax,ax
 push ax
 retf
align 16
code: incbin "code.bin"
 incbin "services.bin"
align 16
data: incbin "data.bin"
""")
        for name in ("services", "test"):
            subprocess.run(
                ["nasm", "-f", "bin", name + ".asm", "-o", name + ".bin"],
                cwd=folder,
                capture_output=True,
                check=True,
            )
        result = subprocess.run(
            ["emu2", "test.bin"], cwd=folder, capture_output=True, timeout=5
        )
        assert result.returncode == 0, (script, result.stdout, result.stderr)
        return result.stdout.decode("latin1").replace("\0", "")


def check():
    count = 0
    for ending in (b"\r", b"\r\n", b"\n"):
        output = run(
            ending.join([b"", b" \t ", b"  ;hidden comment", b"  ONE  ", b"TWO\t", b""])
            + b"\x1a"
        )
        assert re.findall(r"<RUN:(.*?)>", output) == ["ONE", "TWO"], output
        assert output.count("A>") == 2 and "hidden" not in output, output
        count += 1
    for last in (b"ONE\x1a", b"ONE\r\x1a", b"  ONE \t\x1a"):
        output = run(last)
        assert re.findall(r"<RUN:(.*?)>", output) == ["ONE"], output
        count += 1
    for empty in (b"\x1a", b"\r\n \t\r;comment\x1a"):
        assert run(empty) == ""
        count += 1
    output = run(b"FIRST\r$ECHO OFF\rSECOND\r$ECHO ON\rTHIRD\x1a")
    assert re.findall(r"<RUN:(.*?)>", output) == ["FIRST", "SECOND", "THIRD"], output
    assert output.count("A>") == 2 and output.count("SECOND") == 1, output
    assert "$ECHO" not in output, output
    count += 1
    output = run(b" \t$echo\toff \t\r;hidden\r\n \t\rONE\r$echo on\rTWO\x1a")
    assert output.count("A>") == 1 and output.count("ONE") == 1, output
    count += 1
    output = run(b"$GLOBAL\r$LOCAL\r$INCLUDE NEXT\x1a")
    assert (
        "A>" not in output and "mode" not in output and "(submit)" not in output
    ), output
    assert re.findall(r"<RUN:(.*?)>", output) == ["SUBMIT.CMD NEXT"], output
    count += 1
    for directives in (b"$GLOBAL\r$LOCAL\x1a", b"$ECHO OFF\r$ECHO ON\x1a"):
        assert run(directives) == ""
        count += 1
    output = run(b"$ECHO OFF\r$ECHO MAYBE\rONE\x1a")
    assert "Expected ECHO ON or OFF" in output and "can not continue" in output, output
    assert "<RUN:" not in output, output
    count += 1
    # A record boundary can fall between CR/LF or inside the final command.
    output = run(b";" + b"x" * 381 + b"\r\n  ONE\t\x1a")
    assert re.findall(r"<RUN:(.*?)>", output) == ["ONE"], output
    count += 1
    for script in (b"ONE" + b" " * 125, b" " * 256 + b"ONE\x1a"):
        output = run(script)
        assert re.findall(r"<RUN:(.*?)>", output) == ["ONE"], output
        count += 1
    assert run(b" ;" + b"x" * 300 + b"\x1a") == ""
    count += 1
    for invalid in (b"$ECHO", b"$ECHO OFF EXTRA", b"$ECHO ONLY"):
        output = run(invalid + b"\x1a")
        assert "Expected ECHO ON or OFF" in output and "<RUN:" not in output, output
        count += 1
    output = run(b"  ONE  TWO\tTHREE  \x1aIGNORED\r")
    assert re.findall(r"<RUN:(.*?)>", output) == ["ONE  TWO\tTHREE"], output
    count += 1
    assert run(b"ONE\r; trailing comment\x1a") == run(b"ONE\r\x1a")
    count += 1
    print(f"PASS: {count} assembled SUBMIT line, EOF, comment and echo scenarios")


if __name__ == "__main__":
    check()
