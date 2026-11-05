; Booted regression shared with the CDOS reference implementation.
bits 16
cpu 8086
org 100h
start:
    push cs
    pop ds
    mov byte [mode],0
next_mode:
    mov byte [stage],1
    xor ax,ax
    mov al,[mode]
    int 10h
    mov ah,0fh
    int 10h
    cmp al,[mode]
    jne failed
    cmp ah,40
    jne failed
    cmp bh,0
    jne failed
    ; Character repeats cross from column 39 to the next physical row.
    mov byte [stage],2
    xor bx,bx
    mov dx,0027h
    mov ah,2
    int 10h
    mov ax,0958h
    mov bl,1eh
    mov cx,2
    int 10h
    mov ax,0b800h
    mov es,ax
    cmp word [es:78],1e58h
    jne failed
    cmp word [es:80],1e58h
    jne failed
    mov ah,3
    int 10h
    cmp dx,0027h
    jne failed
    mov ah,8
    int 10h
    cmp ax,1e58h
    jne failed
    ; Teletype wraps at 40 and supports ordinary CR/LF/backspace.
    mov byte [stage],3
    mov ax,0e59h
    int 10h
    mov ah,3
    int 10h
    cmp dx,0100h
    jne failed
    mov ax,0e5ah
    int 10h
    mov ax,0e08h
    int 10h
    mov ah,3
    int 10h
    cmp dx,0100h
    jne failed
    mov ax,0e0dh
    int 10h
    mov ax,0e0ah
    int 10h
    mov ah,3
    int 10h
    cmp dx,0200h
    jne failed
    ; Clear, scroll upward then downward, and verify physical row stride.
    mov byte [stage],4
    mov ax,0600h
    mov bx,0700h
    xor cx,cx
    mov dx,1827h
    int 10h
    xor bx,bx
    mov dx,0100h
    mov ah,2
    int 10h
    mov ax,0951h
    mov bl,2eh
    mov cx,40
    int 10h
    mov ax,0601h
    mov bx,0700h
    xor cx,cx
    mov dx,1827h
    int 10h
    cmp word [es:0],2e51h
    jne failed
    cmp word [es:78],2e51h
    jne failed
    cmp word [es:80],0720h
    jne failed
    mov ax,0701h
    int 10h
    cmp word [es:0],0720h
    jne failed
    cmp word [es:80],2e51h
    jne failed
    ; Bottom-right TTY must scroll, leaving a blank bottom row.
    mov byte [stage],5
    xor bx,bx
    mov dx,1827h
    mov ah,2
    int 10h
    mov ax,0e57h
    int 10h
    mov ah,3
    int 10h
    cmp dx,1800h
    jne failed
    cmp byte [es:1918],'W'
    jne failed
    cmp byte [es:1920],' '
    jne failed
    ; Native escape line insertion/deletion must use the same 40-cell stride.
    mov byte [stage],6
    mov dx,insert_line
    mov ah,9
    int 21h
    cmp byte [es:0],' '
    jne failed
    cmp byte [es:80],'R'
    jne failed
    mov dx,delete_line
    mov ah,9
    int 21h
    cmp byte [es:0],'R'
    jne failed
    mov dx,insert_middle
    mov ah,9
    int 21h
    cmp byte [es:0],'R'
    jne failed
    cmp byte [es:160],' '
    jne failed
    cmp byte [es:240],'S'
    jne failed
    ; Explicit restoration, then enter the other 40-column text variant.
    mov ax,3
    int 10h
    inc byte [mode]
    cmp byte [mode],2
    jb next_mode
%ifndef CDOS
    ; Native ESC a selects the newly supported text mode too.
    mov byte [stage],7
    mov dx,native_mode
    mov ah,9
    int 21h
    mov ah,0fh
    int 10h
    cmp ax,2800h
    jne failed
    cmp byte [es:0],'N'
    jne failed
    mov ax,3
    int 10h
%endif
    ; Exit while still in 40-column mode: the DOS session must restore text.
    mov ax,1
    int 10h
    mov word [result],4150h
    mov word [result+2],5353h
    mov word [result+4],4445h
    int 20h
failed:
    mov ax,3
    int 10h
    mov dx,error
    mov ah,9
    int 21h
    int 20h
signature: db "TEXT40 RESULT:"
mode: db 0
stage: db 0
result: db "......"
error: db 'TEXT40 FAIL',13,10,'$'

insert_line: db 27,'H','R',27,'H',27,'L','$'
delete_line: db 27,'M','$'
native_mode: db 27,'a',0,'N','$'

insert_middle: db 27,"Y",34,32,"S",27,"Y",34,32,27,"L","$"
