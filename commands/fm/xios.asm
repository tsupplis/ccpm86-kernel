; C interface to the same Concurrent 3.1 XIOS gateway used by WINDOW/WMENU.
; All public entries preserve the Aztec small-model C ABI (BP, SI, DI, DS, ES).
; XCALL_(function,bx,cx,dx): far call system:0C03h with DS=system,
; ES=current process's XIOS context. Return full AX, including key type.
; XSEG returns the SYSTEM DATA segment: pointers returned by function 16
; address structures there, not in the process's XIOS context segment.
; Far-memory helpers deliberately live here, outside the portable C logic.
        public XINIT_,XSEG_,XCALL_,FARWORD_,FARBYTE_,FARPUT_,BDOS_
dataseg segment para public 'data'

system  dw 0
context dw 0
gateway dw 0c03h,0
dataseg ends
codeseg segment para public 'code'
        assume cs:codeseg,ds:dataseg,ss:dataseg
XINIT_:
        push bp
        push si
        push di
        push es
        mov cl,154
        int 0e0h
        mov system,es
        mov gateway+2,es
        mov cl,156
        int 0e0h
        mov ax,es:10h[bx]
        mov context,ax
        pop es
        pop di
        pop si
        pop bp
        ret
; Shared read-only accessor; adapters do not own each other's state.
XSEG_:
        mov ax,system
        ret
XCALL_:
        push bp
        mov bp,sp
        push si
        push di
        push es
        mov ax,4[bp]
        mov bx,6[bp]
        mov cx,8[bp]
        mov dx,10[bp]
        mov es,context
        push gateway+2
        push gateway
        mov si,sp
        push ds
        mov ds,system
        call dword ptr ss:[si]
        pop ds
        add sp,4
        pop es
        pop di
        pop si
        pop bp
        ret
FARWORD_:
        push bp
        mov bp,sp
        push es
        mov es,4[bp]
        mov bx,6[bp]
        mov ax,es:[bx]
        pop es
        pop bp
        ret
FARBYTE_:
        push bp
        mov bp,sp
        push es
        mov es,4[bp]
        mov bx,6[bp]
        xor ax,ax
        mov al,es:[bx]
        pop es
        pop bp
        ret
FARPUT_:
        push bp
        mov bp,sp
        push es
        mov es,4[bp]
        mov bx,6[bp]
        mov ax,8[bp]
        mov es:[bx],ax
        pop es
        pop bp
        ret
BDOS_:
        push bp
        mov bp,sp
        push si
        push di
        push es
        mov cx,4[bp]
        mov dx,6[bp]
        int 0e0h
        pop es
        pop di
        pop si
        pop bp
        ret
; FM direct-screen lock. Caller has validated the window structure.
; mode=0 acquires; mode=1 releases. Never hold this across BDOS I/O.
        public FMLOCK_
FMLOCK_:
        push bp
        mov bp,sp
        push es
        mov es,4[bp]
        mov bx,6[bp]
        cmp word ptr 8[bp],0
        jne fm_unlock
fm_lock_retry:
        mov al,0ffh
        xchg al,es:52[bx]
        cmp al,0ffh
        je fm_lock_retry
        jmp short fm_lock_done
fm_unlock:
        xor al,al
        xchg al,es:52[bx]
fm_lock_done:
        mov ax,1
        pop es
        pop bp
        ret
; FMCOPY_(segment,text,attributes,count): update the virtual text image.
; Caller holds FMLOCK. Count is 0..1920; status row is never touched.
; Return the number of changed cells. Preserve the small-model C ABI.
        public FMCOPY_
FMCOPY_:
        push bp
        mov bp,sp
        push si
        push di
        push es
        mov es,4[bp]
        mov si,6[bp]
        mov bx,8[bp]
        mov cx,10[bp]
        xor di,di
        xor dx,dx
        jcxz fm_copy_done
fm_copy_cell:
        mov al,[si]
        mov ah,[bx]
        and ah,7fh
        cmp ax,es:[di]
        je fm_copy_next
        mov es:[di],ax
        inc dx
fm_copy_next:
        inc si
        inc bx
        add di,2
        loop fm_copy_cell
fm_copy_done:
        mov ax,dx
        pop es
        pop di
        pop si
        pop bp
        ret
codeseg ends
        end
