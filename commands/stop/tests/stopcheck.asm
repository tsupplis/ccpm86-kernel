; Run a CMD image with synthetic SYSDAT and a fully intercepted BDOS gate.
bits 16
cpu 8086
org 100h
%include "case.inc"

start:
    push cs
    pop ds
    cld
    mov ax,35e0h
    int 21h
    mov [old_e0],bx
    mov [old_e0+2],es
    mov dx,bdos
    mov ax,25e0h
    int 21h

    mov ax,CODE_SEG
    mov es,ax
    xor di,di
    mov si,code_image
    mov cx,code_end-code_image
    rep movsb

    mov ax,DATA_SEG
    mov es,ax
    xor ax,ax
    xor di,di
    mov cx,8000h
    rep stosw
    mov si,data_image
    xor di,di
    mov cx,data_end-data_image
    rep movsb
    mov byte [es:80h],TAIL_LENGTH
    mov di,81h
    mov si,tail_data
    mov cx,TAIL_LENGTH
    rep movsb

    mov ax,SYS_SEG
    mov es,ax
    xor ax,ax
    xor di,di
    mov cx,8000h
    rep stosw
    mov si,sys_image
    xor di,di
    mov cx,sys_end-sys_image
    rep movsb

    mov ax,GROUP_A_SEG
    mov es,ax
    xor ax,ax
    xor di,di
    mov cx,16
    rep stosw
    mov si,group_a_image
    xor di,di
    mov cx,group_a_end-group_a_image
    rep movsb
    mov ax,GROUP_B_SEG
    mov es,ax
    xor ax,ax
    xor di,di
    mov cx,16
    rep stosw
    mov si,group_b_image
    xor di,di
    mov cx,group_b_end-group_b_image
    rep movsb

    mov ax,DATA_SEG
    mov ds,ax
    mov es,ax
    mov ax,CODE_SEG
    push ax
    xor ax,ax
    push ax
    retf

bdos:
    cmp cl,0
    je exit_bdos
    cmp cl,2
    je output_character
    cmp cl,9
    je output_string
    cmp cl,10
    je input_line
    cmp cl,12
    je version
    cmp cl,143
    je exit_bdos
    cmp cl,154
    je sysdat
    cmp cl,157
    je abort_process
    mov byte [cs:exit_code],1
    jmp exit_bdos

output_character:
    push ax
    mov al,dl
    call emit
    pop ax
    iret

output_string:
    push ax
    push si
    mov si,dx
.next:
    lodsb
    cmp al,'$'
    je .done
    call emit
    jmp .next
.done:
    pop si
    pop ax
    iret

input_line:
    inc byte [cs:read_count]
    push ax
    push bx
    push cx
    push si
    push di
    mov bx,dx
    mov byte [bx+1],INPUT_LENGTH
    mov di,bx
    add di,2
    mov si,input_data
    mov cx,INPUT_LENGTH
.copy:
    jcxz .done
    mov al,[cs:si]
    mov [di],al
    inc si
    inc di
    loop .copy
.done:
    mov byte [di],13
    pop di
    pop si
    pop cx
    pop bx
    pop ax
    iret

version:
    mov ax,VERSION
    mov bx,0beefh
    iret

sysdat:
    mov bx,SYS_OFFSET
    mov ax,SYS_SEG
    mov es,ax
    iret

abort_process:
    inc byte [cs:abort_count]
    push ax
    push cx
    push si
    push di
    mov si,dx
    mov di,abort_data
    mov cx,14
.copy:
    mov al,[si]
    mov [cs:di],al
    inc si
    inc di
    loop .copy
    pop di
    pop si
    pop cx
    pop ax
    mov ax,ABORT_RESULT
    mov bx,0faceh
    iret

exit_bdos:
    mov [cs:exit_function],cl
    cli
    mov ax,cs
    mov ss,ax
    mov sp,0fff0h
    sti
    mov ds,ax
    xor ax,ax
    mov es,ax
    mov ax,[old_e0]
    mov [es:380h],ax
    mov ax,[old_e0+2]
    mov [es:382h],ax
    mov dx,marker
    mov ah,9
    int 21h
    mov al,[exit_function]
    call print_hex
    mov al,[abort_count]
    call print_hex
    mov al,[read_count]
    call print_hex
    mov si,abort_data
    mov cx,14
.record:
    mov al,[si]
    call print_hex
    inc si
    loop .record
    mov al,'>'
    call emit
    mov al,[exit_code]
    mov ah,4ch
    int 21h

print_hex:
    push ax
    push bx
    push cx
    mov bl,al
    mov cl,4
    shr al,cl
    call hex_digit
    mov al,bl
    and al,0fh
    call hex_digit
    pop cx
    pop bx
    pop ax
    ret

hex_digit:
    push bx
    xor bh,bh
    mov bl,al
    mov al,[cs:hex_digits+bx]
    call emit
    pop bx
    ret

emit:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push ds
    push es
    mov dl,al
    mov ah,2
    int 21h
    pop es
    pop ds
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

old_e0: dd 0
exit_function: db 0ffh
exit_code: db 0
abort_count: db 0
read_count: db 0
abort_data: times 14 db 0
marker: db '<STOPTEST:$'
hex_digits: db '0123456789ABCDEF'
tail_data: incbin "tail.bin"
input_data: incbin "input.bin"
code_image: incbin "code.bin"
code_end:
data_image: incbin "data.bin"
data_end:
sys_image: incbin "sysdata.bin"
sys_end:
group_a_image: incbin "group_a.bin"
group_a_end:
group_b_image: incbin "group_b.bin"
group_b_end:
