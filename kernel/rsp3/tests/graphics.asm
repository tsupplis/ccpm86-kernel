; Host-only regression fixture. Production kernels continue to use ASM86.
bits 16
cpu 8086
org 100h
%ifdef MDA
%define MODE_COUNT 0
%define MONOTEXT
%elifdef HGC
%define MODE_COUNT 1
%define MONOTEXT
%elifdef EGAMONO
%define MODE_COUNT 1
%define MONOTEXT
%elifdef CGA
%define MODE_COUNT 3
%define NO_VGA
%elifdef EGA
%define MODE_COUNT 6
%define NO_VGA
%else
%define MODE_COUNT 10
%endif
%ifdef MONOTEXT
%define TEXT_MODE 7
%else
%define TEXT_MODE 3
%endif
start:
    push cs
    pop ds
    mov si,modes
next_mode:
    lodsb
    test al,al
    jz passed
    mov [mode],al
    mov byte [stage],1
    mov ah,0
    push si
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    pop si
    mov byte [stage],2
    mov ah,0fh
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    cmp al,[mode]
    jne failed
    push es
    mov ax,40h
    mov es,ax
    mov al,[es:49h]
    pop es
    cmp al,[mode]
    jne failed
    mov ah,0fh
    int 10h
    mov [columns],ah
    mov al,[mode]
    mov bp,640
    mov di,200
%ifdef HGC
    mov bp,720
    mov di,348
    jmp dimensions
%endif
    cmp al,0fh
    je height350
    cmp al,10h
    jne not350
height350:
    mov di,350
not350:
    cmp al,11h
    jb not480
    mov di,480
not480:
    cmp al,4
    je narrow
    cmp al,5
    je narrow
    cmp al,0dh
    je narrow
    cmp al,13h
    jne dimensions
    mov di,200
narrow:
    mov bp,320
dimensions:
    mov ax,bp
    shr ax,1
    shr ax,1
    shr ax,1
    cmp al,[columns]
    jne failed
    mov byte [stage],3
    xor bx,bx
    xor cx,cx
    xor dx,dx
    mov al,1
    mov ah,0ch
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    mov ax,0d00h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    cmp al,1
    jne failed
    mov byte [stage],4
    ; Last valid pixel must survive a write beyond each boundary and page 1.
    mov cx,bp
    dec cx
    mov dx,di
    dec dx
    mov ax,0c01h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    mov ax,0d00h
    int 10h
    cmp al,1
    jne failed
    inc cx
    mov ax,0c00h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    dec cx
    mov ax,0d00h
    int 10h
    mov [actual],ax
    cmp al,1
    jne failed
    inc dx
    mov ax,0c00h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    dec dx
    mov ax,0d00h
    int 10h
    mov [actual],ax
    cmp al,1
    jne failed
    mov bh,1
    mov ax,0c00h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    xor bh,bh
    mov ax,0d00h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    cmp al,1
    jne failed
    mov byte [stage],5
    cmp byte [mode],13h
    je packed
    mov ax,0c81h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    mov ax,0d00h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    cmp al,0
    jne failed
    cmp byte [mode],0fh
    jne character
    ; Video and intensity are independent plane bits in monochrome mode.
    mov byte [stage],54
    mov ax,0c05h
    int 10h
    mov ax,0d00h
    int 10h
    cmp al,5
    jne failed
    mov ax,0c84h
    int 10h
    mov ax,0d00h
    int 10h
    cmp al,1
    jne failed
    jmp character
packed:
    mov byte [stage],51
    mov ax,0ce1h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    mov ax,0d00h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    cmp al,0e1h
    jne failed
    ; All 256 DAC entries addressable, with full RGB readback.
    mov byte [stage],52
    mov bx,255
    mov ax,1010h
    mov dh,17
    mov ch,33
    mov cl,49
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    mov byte [stage],53
    mov ax,1015h
    xor cx,cx
    xor dx,dx
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    cmp dh,17
    jne failed
    cmp ch,33
    jne failed
    cmp cl,49
    jne failed
character:
%ifdef HGC
    jmp palette_done
%endif
    cmp byte [mode],6
    jbe palette_done
    mov byte [stage],6
    cmp byte [mode],13h
    je palette_done
    mov ax,1000h
    mov bx,3f01h
    int 10h
%ifndef EGA
    mov ax,1007h
    xor bh,bh
    int 10h
    cmp bh,3fh
    jne failed
%endif
palette_done:
    ; Cursor packet and firmware drawing must operate in graphics memory.
    xor bx,bx
    mov dx,0102h
    mov ah,2
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    mov ah,3
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    cmp dx,0102h
    jne failed
    mov byte [stage],62
    mov ax,0941h
    mov bl,1
    mov cx,1
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    mov byte [stage],63
    mov ah,8
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    cmp al,'A'
    jne failed
    ; Graphics teletype uses BL colour and advances the firmware cursor.
    mov ax,0e42h
    mov bx,1
    int 10h
    mov ah,3
    int 10h
    cmp dx,0103h
    jne failed
    mov ah,2
    mov dx,0102h
    int 10h
    mov ah,8
    int 10h
    cmp al,'B'
    jne failed
    mov byte [stage],7
    ; Scroll a valid single-cell rectangle, then return to text.
    mov ax,0600h
    xor bx,bx
    mov cx,0102h
    mov dx,0102h
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
restore_text:
    mov ax,TEXT_MODE
    int 10h
    mov [actual],ax
    mov [actualcx],cx
    mov [actualdx],dx
    inc byte [passes]
    mov dx,progress
    mov ah,9
    int 21h
    jmp next_mode
passed:
%ifdef MDA
    mov ax,7fh
    int 10h
%elifdef HGC
    mov ax,0fh
    int 10h
    mov dx,native_hercules
    mov ah,9
    int 21h
    mov ah,0fh
    int 10h
    cmp ax,5a7fh
    jne failed
    mov ax,TEXT_MODE
    int 10h
%elifdef NO_VGA
    mov ax,0fh
    int 10h
    mov ah,0fh
    int 10h
    cmp ax,5003h
    jne failed
%else
    mov byte [stage],8
    mov dx,native_mono
    mov ah,9
    int 21h
    mov ah,0fh
    int 10h
    cmp ax,500fh
    jne failed
    mov ax,TEXT_MODE
    int 10h
%endif
%ifdef MONOTEXT
    ; Unsupported colour requests leave the monochrome console intact.
    mov si,unsupported
reject_next:
    lodsb
    test al,al
    jz reject_done
    xor ah,ah
    int 10h
    mov ah,0fh
    int 10h
    cmp ax,5007h
    jne failed
    jmp reject_next
reject_done:
%endif
    mov ah,0fh
    int 10h
    cmp ax,5000h+TEXT_MODE
    jne failed
    push es
    mov ax,40h
    mov es,ax
    cmp byte [es:49h],TEXT_MODE
    pop es
    jne failed
%ifndef HGC
    ; The project Hercules extension must not alias an IBM mode.
    mov ax,7fh
    int 10h
    mov ah,0fh
    int 10h
    cmp ax,5000h+TEXT_MODE
    jne failed
%endif
    cmp byte [passes],MODE_COUNT
    jne failed
    mov dx,success
    jmp finish
failed:
    mov ax,TEXT_MODE
    int 10h
    mov dx,error
finish:
    mov ah,9
    int 21h
%ifdef HGC
    ; Leave graphics active: DOS termination must restore the saved text.
    mov ax,7fh
    int 10h
%endif
    int 20h
%ifdef MDA
modes: db 0
%elifdef HGC
modes: db 7fh,0
%elifdef EGAMONO
modes: db 0fh,0
%elifdef CGA
modes: db 4,5,6,0
%elifdef EGA
modes: db 4,5,6,0dh,0eh,10h,0
%else
modes: db 4,5,6,0dh,0eh,0fh,10h,11h,12h,13h,0
%endif
columns: db 0
mode: db 0
stage: db 0
passes: db 0
actual: dw 0
actualcx:dw 0
actualdx:dw 0
progress: db 'MODE PASS',13,10,'$'
success: db 'GRAPHICS PASS',13,10,'$'
error: db 'GRAPHICS FAIL',13,10,'$'

native_mono: db 27,"a",0fh,"$"

unsupported: db 3,4,0dh,0eh,10h,11h,12h,13h,0

native_hercules: db 27,"a",7fh,"$"
