#!/usr/bin/env python3
"""Run the assembled XIOS graphics guards with synthetic video-ROM results.

Firmware and chain-4 memory boundaries are stubbed; live tests cover both. Mode tables, validation,
packet marshalling and result capture run from the local production XIOS.
"""
from pathlib import Path
import re,subprocess,tempfile
ROOT=Path(__file__).resolve().parent.parent
CON=ROOT/'kernel/con'
CDOS=(CON/'squid.a86').exists()
symbols=dict((n,int(v,16)) for v,n in re.findall(r'\b([0-9A-F]{4})\s+([A-Z][A-Z0-9]*)',(CON/('squid.sym' if CDOS else 'xios.sym')).read_text()))
b=(CON/'xios.con').read_bytes();BASE=b[128:128+int.from_bytes(b[1:3],'little')*16]
S=lambda n:symbols[n.replace('_','').upper()]

def asm(text,folder,name):
    (folder/(name+'.asm')).write_text(text)
    subprocess.run(['nasm','-f','bin',name+'.asm','-o',name+'.bin'],cwd=folder,check=True,capture_output=True)
    return (folder/(name+'.bin')).read_bytes()

def run_case(mode,selector,x=0,y=0,page=0,owner=True,top=True,palette=0,valid=True):
    with tempfile.TemporaryDirectory(prefix='xios-graphics-') as temp:
        f=Path(temp);code=bytearray(BASE)
        stub=f'''mov [0f100h],ax
 mov [0f102h],bx
 mov [0f104h],cx
 mov [0f106h],dx
 inc byte [0f108h]
 mov word [{S('video_result_ax')}],002ah
 mov word [{S('video_result_bx')}],0011h
 mov word [{S('video_result_cx')}],2233h
 mov word [{S('video_result_dx')}],4455h
 ret
'''
        target=len(code);code+=asm(stub,f,'stub')
        at=S('video_bios');code[at:at+3]=b'\xe9'+((target-at-3)&65535).to_bytes(2,'little')
        at=S('eg_graphics_bios');code[at:at+3]=b'\xe9'+((target-at-3)&65535).to_bytes(2,'little')
        chain=asm(f"mov ax,si\nmov ah,al\nmov [0f100h],ax\ninc byte [0f108h]\nxor ax,ax\ncmp si,13\njne done\nmov ax,42\ndone:ret\n",f,'chain')
        dest=len(code);code+=chain
        at=S('eg_chain4_pixel');code[at:at+3]=b'\xe9'+((dest-at-3)&65535).to_bytes(2,'little')
        at=S('hgc_pixel');code[at:at+3]=b'\xe9'+((dest-at-3)&65535).to_bytes(2,'little')
        # Near return trampoline occupies the unused SYSDAT prefix.
        code[:4]=b'\xe8'+((S('eg_try_video')-3)&65535).to_bytes(2,'little')+b'\xcb'
        (f/'payload.bin').write_bytes(code)
        expected=42 if selector in (8,13) and valid else (0 if valid else 65535)
        body=f'''bits 16
cpu 8086
org 100h
 push cs
 pop ds
 mov ax,cs
 add ax,(payload-$$+100h)/16
 mov [target+2],ax
 mov ds,ax
 mov es,ax
 mov word [{S('table_vs')}],0b000h
 mov byte [0b000h+{S('vs_number')}],0
 mov byte [0b000h+{S('vs_bit')}],1
 mov byte [0b000h+{S('vs_mx')}],0
 mov byte [0b000h+{S('vs_screen_mode')}],{mode}
 mov byte [{S('graphic_bits')}],{int(owner)}
 mov byte [{S('top_screen')}],{0 if top else 1}
 mov byte [{S('bios_graphics')}],{int(mode>=0x50)}
 mov byte [{S('enhanced_vga')}],1
 mov word [0f000h],{palette}
 mov word [0f002h],{page<<8}
 mov word [0f004h],{x}
 mov word [0f006h],{y}
 mov byte [0f008h],0
 mov byte [0f108h],0
 mov ax,{selector|128}
 mov cx,ds
 mov dx,0f000h
 mov si,0a55ah
 mov di,05aa5h
 mov bp,1234h
 mov bx,5678h
 call far [cs:target]
 jc failed
 cmp ax,{expected}
 jne failed
 cmp byte [0f108h],{int(valid)}
 jne failed
 cmp byte [0b000h+{S('vs_mx')}],0
 jne failed
 cmp si,0a55ah
 jne failed
 cmp di,05aa5h
 jne failed
 cmp bp,1234h
 jne failed
 cmp bx,5678h
 jne failed
'''
        if valid:
            body+=f'cmp byte [0f101h],{selector}\njne failed\n'
            if selector in (3,16):
                for off,value in [(2,0x11),(4,0x2233),(6,0x4455)]:
                    body+=f'cmp word [0f000h+{off}],{value}\njne failed\n'
        body+='mov ax,4c00h\nint 21h\nfailed:mov ax,4c01h\nint 21h\ntarget:dw 0,0\nalign 16,db 0\npayload:incbin "payload.bin"\n'
        asm(body,f,'case')
        result=subprocess.run(['emu2','case.bin'],cwd=f,capture_output=True,timeout=5)
        assert result.returncode==0,(hex(mode),selector,x,y,page,owner,top,palette,valid,result.stdout,result.stderr)

count=0
for mode,w,h in [(0x50,640,350),(0x60,320,200),(0x70,640,200),(0x80,640,480),(0x90,640,480),(0xa0,320,200),(0xb0,640,350),(0xc0,720,348),(0x20,320,200),(0x30,320,200),(0x40,640,200)]:
    for selector in (12,13):
        for x,y,page,owner,top,valid in [(0,0,0,True,True,True),(w-1,h-1,0,True,True,True),(w,0,0,True,True,False),(0,h,0,True,True,False),(0,0,1,True,True,False),(0,0,0,False,True,False),(0,0,0,True,False,False)]:
            run_case(mode,selector,x,y,page,owner,top,valid=valid);count+=1
    for selector in (1,2,3,8,9,10,14):run_case(mode,selector);count+=1
    for sub in (0,7,0x10,0x15):run_case(mode,16,palette=sub,valid=mode>=0x50 and mode!=0xc0 and (mode!=0xa0 or sub not in (0,7)));count+=1
for selector in (1,2,3,8,12,13,16,127):
    run_case(5,selector,valid=False);count+=1
print(f'PASS: {count} assembled-XIOS enhanced graphics guards, dimensions, pages, ownership, packet results, palettes and preserved registers')

def capability_checks():
    count=0
    cases=[(0xb0,'enhanced_mono',0x0f),(0xb0,'enhanced_vga',0x0f),(0x50,'enhanced_ega',0x10),(0x60,'enhanced_video',0x0d),(0x70,'enhanced_video',0x0e),(0x80,'enhanced_vga',0x11),(0x90,'enhanced_vga',0x12),(0xa0,'enhanced_vga',0x13)]
    for entry,flag,mode in cases:
        for enabled in (0,1):
            with tempfile.TemporaryDirectory(prefix='xios-mode-caps-') as temp:
                f=Path(temp);code=bytearray(BASE)
                code[S('general_graph')]=0xc3
                stub=asm('mov [0f100h],ax\ninc byte [0f102h]\nret\n',f,'stub')
                dest=len(code);code+=stub;at=S('video_bios');code[at:at+3]=b'\xe9'+((dest-at-3)&65535).to_bytes(2,'little')
                code[:4]=b'\xe8'+((S('eg_set_graphics')-3)&65535).to_bytes(2,'little')+b'\xcb'
                (f/'payload.bin').write_bytes(code)
                body=f'''bits 16
cpu 8086
org 100h
push cs
pop ds
mov ax,cs
add ax,(payload-$$+100h)/16
mov [target+2],ax
mov ds,ax
mov es,ax
mov byte [{S('enhanced_video')}],0
mov byte [{S('enhanced_ega')}],0
mov byte [{S('enhanced_vga')}],0
mov byte [{S('enhanced_mono')}],0
mov byte [{S(flag)}],{enabled}
mov byte [0f102h],0
mov ch,{entry}
call far [cs:target]
cmp ax,{0 if enabled else 65535}
jne failed
cmp byte [0f102h],{enabled}
jne failed
'''
                if enabled:body+=f'cmp word [0f100h],{mode}\njne failed\n'
                body+='mov ax,4c00h\nint 21h\nfailed:mov ax,4c01h\nint 21h\ntarget:dw 0,0\nalign 16,db 0\npayload:incbin "payload.bin"\n'
                asm(body,f,'case')
                result=subprocess.run(['emu2','case.bin'],cwd=f,capture_output=True,timeout=5)
                assert result.returncode==0,(entry,enabled,result.stdout,result.stderr)
                count+=1
    print(f'PASS: {count} built-XIOS adapter gates: 64-KB EGA 0D/0E, >=128-KB mode 10 and EGA monochrome/VGA 0F and VGA 11/12/13')

capability_checks()

def bda_checks():
    for mode in range(7):
        columns=40 if mode in (0,1,4,5) else 80
        size=0x800 if mode<2 else 0x1000 if mode<4 else 0x4000
        with tempfile.TemporaryDirectory(prefix='xios-cga-bda-') as temp:
            f=Path(temp);code=bytearray(BASE)
            code[:4]=b'\xe8'+((S('eg_legacy_bda')-3)&65535).to_bytes(2,'little')+b'\xcb'
            (f/'payload.bin').write_bytes(code)
            body=f'''bits 16
cpu 8086
org 100h
push cs
pop ds
mov ax,cs
add ax,(payload-$$+100h)/16
mov [target+2],ax
mov ds,ax
mov es,ax
mov ax,{mode+columns*256}
mov bx,1234h
call far [cs:target]
cmp ax,{mode+columns*256}
jne failed
cmp bx,1234h
jne failed
mov ax,40h
mov es,ax
cmp byte [es:49h],{mode}
jne failed
cmp word [es:4ah],{columns}
jne failed
cmp word [es:4ch],{size}
jne failed
cmp word [es:4eh],0
jne failed
cmp byte [es:62h],0
jne failed
mov ax,4c00h
int 21h
failed:mov ax,4c01h
int 21h
target:dw 0,0
align 16,db 0
payload:incbin "payload.bin"
'''
            asm(body,f,'case');result=subprocess.run(['emu2','case.bin'],cwd=f,capture_output=True,timeout=5)
            assert result.returncode==0,(mode,result.stdout,result.stderr)
    print('PASS: seven assembled CGA BIOS mode/column/page-size/offset synchronizations')

bda_checks()

def query_checks():
    cases=[(2,40),(3,40),(4,80),(5,80),(9,80),(0x20,40),(0x30,40),(0x40,80),(0x50,80),(0x60,40),(0x70,80),(0x80,80),(0x90,80),(0xa0,40),(0xb0,80),(0xc0,90)]
    for mode,columns in cases:
        with tempfile.TemporaryDirectory(prefix='xios-mode-query-') as temp:
            f=Path(temp);code=bytearray(BASE)
            code[:4]=b'\xe8'+((S('get_set_screen')-3)&65535).to_bytes(2,'little')+b'\xcb'
            (f/'payload.bin').write_bytes(code)
            body=f'''bits 16
cpu 8086
org 100h
push cs
pop ds
mov ax,cs
add ax,(payload-$$+100h)/16
mov [target+2],ax
mov ds,ax
mov word [{S('table_vs')}],0b000h
mov byte [0b000h+{S('vs_screen_mode')}],{mode}
xor dx,dx
mov ch,4
call far [cs:target]
cmp ax,{mode+columns*256}
jne failed
; The existing native query must keep its historical CDOS disguise.
xor dx,dx
mov ch,1
call far [cs:target]
cmp ax,{(5 if CDOS and mode in (0x20,0x30,0x40) else mode)+80*256}
jne failed
mov ax,4c00h
int 21h
failed:mov ax,4c01h
int 21h
target:dw 0,0
align 16,db 0
payload:incbin "payload.bin"
'''
            asm(body,f,'case')
            result=subprocess.run(['emu2','case.bin'],cwd=f,capture_output=True,timeout=5)
            assert result.returncode==0,(mode,columns,result.stdout,result.stderr)
    print('PASS: 16 truthful mode/column queries and unchanged legacy native queries')

query_checks()

for mode,width in [(0x20,40),(0x30,40),(0x40,80)]:
    for x,y,valid in [(width-1,24,True),(width,0,False),(0,25,False)]:
        run_case(mode,2,y=x+256*y,valid=valid)
    for selector in (6,7):
        run_case(mode,selector,x=0,y=(24<<8)+width-1)
        run_case(mode,selector,x=0,y=(24<<8)+width,valid=False)
        run_case(mode,selector,x=0,y=(25<<8),valid=False)
        run_case(mode,selector,x=0x0101,y=0,valid=False)
print('PASS: CGA cursor and scroll rectangle bounds')
