; Native-only CMD; exercise XIOS 30/31 without a DOS BIOS mode alias.
bits 16
cpu 8086
org -128
db 1
dw (image_end-code+15)/16
dw 0
dw (image_end-code+15)/16
dw 0
times 128-($-$$) db 0
code:
times 100h db 0
start:
 push cs
 pop ds
 mov al,30
 xor bx,bx
 int 0e6h
 mov [gateway+2],es
%ifdef HGC
 test bl,16
 jz failed
 mov ax,0b000h
 mov es,ax
 mov word [es:0],0748h
 mov ax,30
 mov cx,00c0h
 xor dx,dx
 call xios
 test ax,ax
 jnz failed
 mov ax,30
 mov ch,4
 xor dx,dx
 call xios
 cmp ax,5ac0h
 jne failed
 ; Four scan-line banks and final pixel; read independently of input AL.
 mov word [packet+4],0
 mov word [packet+6],0
 mov bp,4
bank:
 mov byte [packet],1
 mov bl,12
 call pixel
 mov byte [packet],0
 mov bl,13
 call pixel
 cmp ax,1
 jne failed
 inc word [packet+6]
 dec bp
 jnz bank
 mov word [packet+4],719
 mov word [packet+6],347
 mov byte [packet],1
 mov bl,12
 call pixel
 ; Direct hardware byte validates bank/row/bit mapping.
 mov ax,0b000h
 mov es,ax
 test byte [es:7e95h],1
 jz failed
 inc word [packet+4]
 mov byte [packet],0
 mov bl,12
 call pixel
 cmp ax,0ffffh
 jne failed
 dec word [packet+4]
 inc word [packet+6]
 mov bl,12
 call pixel
 cmp ax,0ffffh
 jne failed
 dec word [packet+6]
 mov byte [packet+3],1
 mov bl,12
 call pixel
 cmp ax,0ffffh
 jne failed
 mov byte [packet+3],0
 mov bl,13
 call pixel
 cmp ax,1
 jne failed
 mov byte [packet],81h
 mov bl,12
 call pixel
 mov byte [packet],1
 mov bl,13
 call pixel
 test ax,ax
 jnz failed
 ; Preserve a pixel below the text grid through all text operations.
 mov word [packet+4],719
 mov word [packet+6],347
 mov byte [packet],1
 mov bl,12
 call pixel
 ; Historical simple ABI: cursor argument in CX, console in DL.
 mov ax,31
 mov bl,2
 mov cx,0204h
 xor dx,dx
 call xios
 mov ax,31
 mov bl,3
 xor dx,dx
 call xios
 cmp ax,0204h
 jne failed
 mov ax,31
 mov bl,14
 mov cx,'S'
 xor dx,dx
 call xios
 mov ax,31
 mov bl,3
 xor dx,dx
 call xios
 cmp ax,0205h
 jne failed
 ; Cursor and repeated character output are native packet services.
 mov byte [stage],2
 mov word [packet+2],1
 mov word [packet+6],0102h
 mov bl,82h
 call pixel
 mov word [packet],0041h
 mov word [packet+4],1
 mov bl,89h
 call pixel
 test ax,ax
 jnz failed
 mov bl,88h
 call pixel
 cmp ax,0041h
 jne failed
 ; Verify each raster byte independently against the ROM A glyph.
 push ds
 mov ax,0f000h
 mov ds,ax
 mov ax,0b000h
 mov es,ax
 mov si,0fa6eh+65*8
 mov di,182
 mov cx,8
font_compare:
 lodsb
 cmp al,[es:di]
 jne font_failed
 add di,2000h
 test di,8000h
 jz font_next
 sub di,8000h
 add di,90
font_next:
 loop font_compare
 pop ds
 ; Character XOR clears and restores the existing glyph.
 mov word [packet],0041h
 mov word [packet+2],81h
 mov word [packet+4],1
 mov bl,89h
 call pixel
 mov bl,88h
 call pixel
 test ax,ax
 jnz failed
 mov bl,89h
 call pixel
 mov bl,88h
 call pixel
 cmp ax,0041h
 jne failed
 ; Install one distinctive extension glyph, then test a missing extension.
 xor ax,ax
 mov es,ax
 mov ax,[es:7ch]
 mov [saved_font],ax
 mov ax,[es:7eh]
 mov [saved_font+2],ax
 mov byte [font_saved],1
 mov word [es:7ch],extra_font
 mov [es:7eh],cs
 mov word [packet],0080h
 mov word [packet+2],1
 mov bl,89h
 call pixel
 test ax,ax
 jnz failed
 mov bl,88h
 call pixel
 cmp ax,0080h
 jne failed
 xor ax,ax
 mov es,ax
 mov word [es:7ch],0
 mov word [es:7eh],0
 mov bl,89h
 call pixel
 cmp ax,0ffffh
 jne failed
 call restore_font
 mov word [packet],0041h
 mov bl,89h
 call pixel
 ; Down then up within a rectangle.
 mov byte [stage],3
 mov word [packet],0701h
 mov word [packet+2],0
 mov word [packet+4],0102h
 mov word [packet+6],0203h
 mov bl,87h
 call pixel
 mov word [packet+6],0202h
 mov bl,82h
 call pixel
 mov bl,88h
 call pixel
 cmp ax,0041h
 jne failed
 mov word [packet],0601h
 mov word [packet+4],0102h
 mov word [packet+6],0203h
 mov bl,86h
 call pixel
 mov word [packet+6],0102h
 mov bl,82h
 call pixel
 mov bl,88h
 call pixel
 cmp ax,0041h
 jne failed
 ; A repeated write crosses a row boundary without moving the cursor.
 mov byte [stage],4
 mov word [packet+6],0059h
 mov bl,82h
 call pixel
 mov word [packet],0043h
 mov word [packet+2],1
 mov word [packet+4],2
 mov bl,89h
 call pixel
 mov bl,83h
 call pixel
 cmp word [packet+6],0059h
 jne failed
 mov word [packet+6],0100h
 mov bl,82h
 call pixel
 mov bl,88h
 call pixel
 cmp ax,0043h
 jne failed
 ; Bottom-right teletype wraps and scrolls one row, preserving its glyph.
 mov byte [stage],5
 mov word [packet+6],2a59h
 mov bl,82h
 call pixel
 mov word [packet],0054h
 mov word [packet+2],1
 mov bl,8eh
 call pixel
 mov bl,83h
 call pixel
 cmp word [packet+6],2a00h
 jne failed
 mov word [packet+6],2959h
 mov bl,82h
 call pixel
 mov bl,88h
 call pixel
 cmp ax,0054h
 jne failed
 ; Clearing with a foreground fill and rejecting invalid cell coordinates.
 mov byte [stage],6
 mov word [packet],0600h
 mov word [packet+2],0100h
 mov word [packet+4],0102h
 mov word [packet+6],0102h
 mov bl,86h
 call pixel
 mov ax,0b000h
 mov es,ax
 cmp byte [es:182],0ffh
 jne failed
 mov word [packet+2],0
 mov word [packet+6],2b00h
 mov bl,82h
 call pixel
 cmp ax,0ffffh
 jne failed
 mov word [packet+6],005ah
 mov bl,82h
 call pixel
 cmp ax,0ffffh
 jne failed
 ; Repeat counts clip at the final cell, without touching the raster tail.
 mov word [packet+6],2a59h
 mov bl,82h
 call pixel
 mov word [packet],0045h
 mov word [packet+2],1
 mov word [packet+4],0ffffh
 mov bl,89h
 call pixel
 mov bl,88h
 call pixel
 cmp ax,0045h
 jne failed
 mov ax,0b000h
 mov es,ax
 test byte [es:7e95h],1
 jz failed
 ; Palette programming remains unsupported on Hercules.
 mov bl,90h
 call pixel
 cmp ax,0ffffh
 jne failed
 call restore
 mov ax,30
 mov ch,4
 xor dx,dx
 call xios
 cmp ax,5009h
 jne failed
 mov ax,0b000h
 mov es,ax
 cmp word [es:0],0748h
 jne failed
%else
 test bl,16
 jnz failed
 mov ax,30
 mov cx,00c0h
 xor dx,dx
 call xios
 cmp ax,0ffffh
 jne failed
%endif
 mov dx,success
 jmp finish
font_failed:
 pop ds
failed:
 call restore
 mov dx,error
finish:
 mov cl,9
 int 0e0h
 xor cx,cx
 xor dx,dx
 int 0e0h
restore:
 call restore_font
 mov ax,30
 mov cx,9
 xor dx,dx
 call xios
 ret
restore_font:
 cmp byte [font_saved],0
 je font_restored
 push ax
 push es
 xor ax,ax
 mov es,ax
 mov ax,[saved_font]
 mov [es:7ch],ax
 mov ax,[saved_font+2]
 mov [es:7eh],ax
 mov byte [font_saved],0
 pop es
 pop ax
font_restored:
 ret
pixel:
 mov ax,31
 mov cx,ds
 mov dx,packet
 call xios
 ret
xios:
 push ds
 push bp
 mov ds,[cs:gateway+2]
 call far [cs:gateway]
 pop bp
 pop ds
 ret
extra_font: db 0a5h,05ah,081h,07eh,018h,0e7h,042h,0bdh
saved_font: dd 0
font_saved: db 0
stage: db 1
gateway: dw 0c03h,0
packet: dw 0,0,0,0
 db 0
success: db 'HERCULES PASS',13,10,'$'
error: db 'HERCULES FAIL',13,10,'$'
image_end:
align 16,db 0
