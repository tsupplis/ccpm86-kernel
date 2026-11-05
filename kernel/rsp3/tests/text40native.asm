; Single-group CP/M-86 CMD: exercise the native-only XIOS build.
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
    mov dx,select40
    call print
    mov ax,0b800h
    mov es,ax
    cmp byte [es:0],'R'
    jne failed
    mov byte [stage],2
    mov dx,insert
    call print
    mov ax,[es:0]
    mov [seen],ax
    mov ax,[es:80]
    mov [seen+2],ax
    mov ax,[es:160]
    mov [seen+4],ax
    cmp byte [es:0],' '
    jne failed
    cmp byte [es:80],'R'
    jne failed
    mov byte [stage],3
    mov dx,delete
    call print
    cmp byte [es:0],'R'
    jne failed
    mov byte [stage],4
    mov dx,bottom
    call print
    cmp byte [es:1838],'W'
    jne failed
    cmp byte [es:1840],' '
    jne failed
    mov dx,restore
    call print
    mov dx,success
    call print
    jmp done
failed:
    mov dx,restore
    call print
    mov dx,error
    call print
done:
    xor dx,dx
    xor cx,cx
    int 0e0h
print:
    push es
    mov cl,9
    int 0e0h
    pop es
    ret
signature: db "NATIVE40 STAGE:"
stage: db 1
seen: times 3 dw 0
select40: db 27,'a',0,'R','$'
insert: db 27,'H',27,'L','$'
delete: db 27,'M','$'
bottom: db 27,'Y',32+23,32+39,'W','$'
restore: db 27,'a',3,'$'
success: db 'TEXT40NATIVE PASS',13,10,'$'
error: db 'TEXT40NATIVE FAIL',13,10,'$'
image_end:
align 16,db 0
