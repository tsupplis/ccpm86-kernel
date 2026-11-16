dataseg segment para public 'data'
;*****************************************************************************
; FMLOC_(drive,out): Aztec small-model near ABI, 16-bit int parameters.
; out[4] = physical drive (1 based), media sequence, directory cluster, mapped.
; Return 1 on success, 0 for unsupported OS or invalid arguments/pointers.
; An unmapped A:-M: names its physical root. An unmapped N:-P: has physical=0.
; Reads only PD/CAT/HDS; never creates, edits or releases a kernel mapping.
; Kernel layout: kernel/con/black.a86 HDRV=0, MSEQ=1, HBLK=6, CAT=2Eh;
; kernel/rsp3/ms2.a86 nextlevel confirms zero-based physical HDRV and CAT slots.
; Preserves BP, SI, DI, DS and ES. All output stores use caller DS, never ES.
;*****************************************************************************
        public FMLOC_
dataseg ends
codeseg segment para public 'code'
        assume cs:codeseg,ds:dataseg,ss:dataseg
FMLOC_:
        push bp
        mov bp,sp
        push si
        push di
        push ds
        push es
        mov ax,4[bp]
        cmp ax,1
        jb loc_bad1
        cmp ax,16
        ja loc_bad1
        mov di,6[bp]
        or di,di
        jz loc_bad1
        cmp di,0fff8h
        ja loc_bad1
        mov cx,12
        push ds
        int 0e0h
        pop ds
        cmp ax,1441h
        jne loc_bad1
        mov cx,156
        push ds
        int 0e0h
        pop ds
        mov ax,es
        or ax,ax
        jz loc_bad1
        or bx,bx
        jz loc_bad1
        cmp bx,0ffd0h
        ja loc_bad1
        mov bx,es:2eh[bx]
        or bx,bx
        jz loc_root
        mov si,4[bp]
        dec si
        shl si,1
        add bx,si
        jc loc_bad1
        cmp bx,0fffeh
        ja loc_bad1
        mov bx,es:[bx]
        or bx,bx
        jz loc_root
        cmp bx,0fff8h
        ja loc_bad1
        xor cx,cx
        mov cl,es:[bx]
        cmp cx,12
        ja loc_bad1
        inc cx
        xor dx,dx
        mov dl,es:1[bx]
        mov si,es:6[bx]
        mov ax,1
        jmp short loc_write
loc_bad1:
        xor ax,ax
        jmp short loc_done
loc_root:
        mov cx,4[bp]
        cmp cx,13
        jbe loc_physical
        xor cx,cx
loc_physical:
        xor dx,dx
        xor si,si
        xor ax,ax
loc_write:
        mov di,6[bp]
        mov [di],cx
        mov 2[di],dx
        mov 4[di],si
        mov 6[di],ax
        mov ax,1
loc_done:
        pop es
        pop ds
        pop di
        pop si
        pop bp
        ret
codeseg ends
        end
