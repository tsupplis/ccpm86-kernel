#!/usr/bin/env python3
"""Execute built XIOS raw-floppy validation/state and format DMA arithmetic.

Hardware service boundaries are stubbed; PCE tests cover actual controller I/O.
Both native-only and PC-MODE builds must expose identical function numbers.
"""
from pathlib import Path
import re,subprocess,tempfile
ROOT=Path(__file__).resolve().parent.parent
C=ROOT/'kernel/con'
count=0
for module in ['xios','xiosnat']:
    symbols={n:int(v,16) for v,n in re.findall(r'\b([0-9A-F]{4})\s+([A-Z][A-Z0-9]*)',(C/(module+'.sym')).read_text())}
    S=lambda n:symbols[n.replace('_','').upper()]
    b=(C/(module+'.con')).read_bytes();base=bytearray(b[128:128+int.from_bytes(b[1:3],'little')*16])
    table=S('function_table')
    for number,name in [(29,'io_variable_format'),(35,'io_raw_read'),(36,'io_raw_write')]:
        assert int.from_bytes(base[table+2*number:table+2*number+2],'little')==S(name),(module,number)
    def assemble(src,p,name):
        (p/(name+'.asm')).write_text('bits 16\ncpu 8086\n'+src)
        subprocess.run(['nasm','-f','bin',name+'.asm','-o',name+'.bin'],cwd=p,check=True,capture_output=True)
        return (p/(name+'.bin')).read_bytes()
    def run(entry,setup,checks,patches=None,packet=None):
        global count
        with tempfile.TemporaryDirectory(prefix='xios-disk-unit-') as td:
            p=Path(td);code=bytearray(base)
            for name,stub in (patches or {}).items():
                dest=len(code);code+=assemble(stub,p,'stub')
                at=S(name);code[at:at+3]=b'\xe9'+((dest-at-3)&65535).to_bytes(2,'little')
            target=len(code);code+=b'\xe8'+((S(entry)-target-3)&65535).to_bytes(2,'little')+b'\xcb'
            (p/'payload.bin').write_bytes(code)
            pushes=''
            if packet:
                drive,track,sector,sectors,formatting=packet
                if formatting:pushes+=' push word 0f650h\n'
                pushes+=f' push word {drive+(sectors<<8)}\n push word {track}\n push word {sector}\n push ds\n push word 0d000h\n'
            pushes=re.sub(r' push word ([0-9a-fh]+)',r' mov ax,\1\n push ax',pushes)
            cleanup=f' add sp,{12 if packet and packet[4] else 10}\n' if packet else ''
            driver=f'''bits 16
cpu 8086
org 100h
 mov ax,cs
 add ax,(payload-$$+100h)/16
 mov [target+2],ax
 mov ds,ax
 mov es,ax
 mov byte [0f000h],0
 mov word [0f010h],0ffffh
{setup}
{pushes} call far [cs:target]
{cleanup}{checks}
 mov ax,4c00h
 int 21h
failed:
 mov ax,4c01h
 int 21h
target dw {target},0
align 16
payload: incbin "payload.bin"
'''
            (p/'test.asm').write_text(driver)
            subprocess.run(['nasm','-f','bin','test.asm','-o','test.com'],cwd=p,check=True,capture_output=True)
            result=subprocess.run(['emu2','test.com'],cwd=p,capture_output=True,timeout=5)
            assert result.returncode==0,(module,entry,setup,packet,result.stderr)
        count+=1
    run('io_pointer','mov cx,0\n mov dx,444bh\n mov si,534bh', ' cmp ax,4b44h\n jne failed\n cmp dx,0101h\n jne failed')
    run('io_pointer','mov cx,0\n xor dx,dx\n xor si,si',f' cmp ax,{S("equip_data")}\n jne failed')
    cases=[(0,0,0,8),(1,0x127,8,1),(0,0x100,0,9),
           (2,0,0,8),(0,40,0,8),(0,0x200,0,8),
           (0,0,9,1),(0,0,0,0),(0,0,0,10),(0,0,8,2)]
    for entry,callee in [('io_raw_read','io_read'),('io_raw_write','io_write')]:
        for drive,track,sector,sectors in cases:
            valid=drive<2 and (track&255)<40 and (track>>8)<2 and 0<=sector<9 and 1<=sectors<=9 and sector+sectors<=9
            logical=(79-(track&255)) if track>>8 else track&255
            stub=f'''mov bp,sp
 cmp byte [{S('raw_eot')}],9
 jne failed
 cmp word [ss:bp+12],{logical}
 jne failed
 cmp word [ss:bp+10],{sector}
 jne failed
 mov byte [0f000h],1
 mov ax,1
 ret
failed: mov ax,4c02h
 int 21h
'''
            run(entry,'',f''' cmp ax,{1 if valid else 0x0401}
 jne failed
 cmp byte [0f000h],{int(valid)}
 jne failed
 cmp byte [{S('raw_eot')}],8
 jne failed
''',{callee:stub},(drive,track,sector,sectors,False))
    for drive,track,sectors in [(0,0,8),(1,79,9),(2,0,8),(0,80,8),(0,0,7),(0,0,10)]:
        valid=drive<2 and track<80 and sectors in [8,9]
        stub=f'''cmp byte [{S('raw_format_count')}],{sectors}
 jne failed
 cmp word [{S('raw_gap_fill')}],0f650h
 jne failed
 mov byte [0f000h],1
 mov ax,1
 ret
failed: mov ax,4c02h
 int 21h
'''
        run('io_variable_format','',f''' cmp ax,{1 if valid else 0x0401}
 jne failed
 cmp byte [0f000h],{int(valid)}
 jne failed
 cmp byte [{S('raw_format_count')}],0
 jne failed
''',{'format_floppy':stub},(drive,track,0,sectors,True))
    # Execute real DMA count selection, stubbing port writes only.
    original=base
    with tempfile.TemporaryDirectory() as td:
        p=Path(td);begin=S('dma_setup');end=S('motor_on')
        (p/'dma.bin').write_bytes(base[begin:end])
        output=subprocess.check_output(['ndisasm','-b','16','dma.bin'],cwd=p,text=True)
        base=bytearray(original)
        for line in output.splitlines():
            parts=line.split()
            if len(parts)>2 and parts[2].lower()=='out':
                at=begin+int(parts[0],16);base[at:at+len(parts[1])//2]=b'\x90'*(len(parts[1])//2)
        at=S('variable_dma_count')
        stub=f'''dec ax
 mov [0f010h],ax
 jmp near {at+3}
'''
        # The generic hook is a relocated near jump. Assemble a stub using
        # its real appended origin so the return enters the existing routine.
        destination=len(base)
        (p/'capture.asm').write_text(f'bits 16\norg {destination}\n'+stub)
        subprocess.run(['nasm','-f','bin','capture.asm','-o','capture.bin'],cwd=p,check=True,capture_output=True)
        base+=(p/'capture.bin').read_bytes()
        base[at:at+3]=b'\xe9'+((destination-at-3)&65535).to_bytes(2,'little')
        for formatted in [0,8,9]:
            run('dma_setup',f'mov byte [{S("raw_format_count")}],{formatted}\n mov byte [{S("num_sec")}],9',f' cmp word [0f010h],{formatted*4-1 if formatted else 4607}\n jne failed')
    base=original
print(f'PASS: {count} assembled XIOS cases across PC-MODE/native builds: capability/table numbers, CHS/count bounds, state restoration and format/read DMA byte counts')
