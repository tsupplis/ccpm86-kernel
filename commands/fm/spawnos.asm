; Local launch subset copied from cpm86-hacking/os.asm (MIT).
; See LICENSE.md. No sibling project is required to build FM.
codeseg segment para public 'code'
dataseg segment para public 'data'
dataseg ends
    assume cs:codeseg, ds:dataseg
    bss clibuf:byte,130
ver_mpm	proc	near
	mov	cx,0Ch
	int	0E0h
	test	ah,05h			; CF cleared
	jnz	ver_mpm_ret
	stc
ver_mpm_ret:
	ret
ver_mpm	endp

; p_cli: BDOS fn 150 (96h, P_CLI) -- send a command line to the CLI
; (resident system process or program load). MP/M and Concurrent CP/M only:
; system type bit 0 (MP/M) or bit 2 (multi-user) set. The command is copied
; (max 127 chars) into a static 130-byte CLI buffer
;   DB 0 / DS 128 (0-terminated command line) / DB 0
;
; int p_cli(char *cmd)  -- 0 = ok, 0xFF = failed or not supported
;
     public	p_cli_
p_cli_	proc	near
        push	bp
        mov 	bp,sp
        push    es
        push	di
        push	si
        push    bx
        call    ver_mpm
        jb      p_cli_no
        push    ds
        pop     es
        mov     si,word ptr 4[bp]
        mov     di,offset clibuf
        mov     byte ptr [di],0		; reserved
        inc     di
        mov     cx,127
p_cli_copy:
        lodsb
        or      al,al
        jz      p_cli_done
        stosb
        loop    p_cli_copy
p_cli_done:
        mov     byte ptr [di],0
        mov     byte ptr clibuf+129,0	; reserved
        mov     dx,offset clibuf
        mov     cx,96h
        int     0E0h
        xor     ah,ah
        jmp     p_cli_end
p_cli_no:
        mov     ax,0FFh
p_cli_end:
        pop     bx
        pop	si
        pop 	di
        pop 	es
        pop 	bp
        ret
p_cli_	endp

;
; ms_p_cli: private CCP/M 3.1 and CDOS 4.1 PCTMP DOS loader (function 1).
; int ms_p_cli(char *opened_fcb, char *command)
; FCB must be a successfully opened 36-byte COM/EXE FCB in current user.
; command includes executable name and arguments (at most 127 characters).
; A successful DOS launch may transfer control back to PCTMP on exit.
; Returns loader status (0 success), or -1 unsupported/invalid input.
; Exact version/type gate: private PD/UDA layouts are not assumed on later OSes.
; Static buffers make this non-reentrant, like p_cli.
    bss mscli_pb:word,10
    bss mscli_line:byte,134
    bss mscli_entry:word,4
    public ms_p_cli_
ms_p_cli_ proc near
    push bp
    mov bp,sp
    push es
    push di
    push si
    push bx
    call pc_lookup
    or ax,ax
    jz mscli_no
    push ds
    pop es
    mov di,offset mscli_line
    mov cx,134
    xor ax,ax
    rep stosb
    mov si,word ptr 6[bp]
    mov di,offset mscli_line+1
    mov cx,128
mscli_copy:
    lodsb
    or al,al
    jz mscli_ready
    stosb
    loop mscli_copy
    jmp mscli_no             ; reject truncation
mscli_ready:
    mov ax,word ptr 4[bp]
    mov word ptr mscli_pb,ax
    mov word ptr mscli_pb+2,ds
    mov word ptr mscli_pb+4,offset mscli_line
    mov word ptr mscli_pb+6,ds
    mov word ptr mscli_pb+8,0 ; default user, as in BATCH
    mov dx,offset mscli_pb
    mov cx,1
    call dword ptr mscli_entry
    mov ax,bx
    jmp mscli_end
mscli_no:
    mov ax,0FFFFh
mscli_end:
    pop bx
    pop si
    pop di
    pop es
    pop bp
    ret
ms_p_cli_ endp

pc_lookup proc near
    mov cx,0Ch
    int 0E0h
    cmp al,31h                ; CCP/M-86 3.1 PC-MODE
    jb pc_lookup_no
    cmp al,41h                ; Concurrent DOS 4.1
    jbe mscli_version
    jmp pc_lookup_no
mscli_version:
    test ah,04h
    jnz mscli_supported
    jmp pc_lookup_no
mscli_supported:
    mov cx,9Ch
    int 0E0h
    mov di,64                 ; bound malformed/cyclic parent chains
mscli_parent:
    or bx,bx
    jnz mscli_name
    jmp pc_lookup_no
mscli_name:
    cmp word ptr es:8[bx],4350h ; PC
    jne mscli_next
    cmp word ptr es:10[bx],6D54h ; Tm
    jne mscli_next
    cmp byte ptr es:12[bx],70h ; p
    je mscli_found
mscli_next:
    mov bx,word ptr es:1Eh[bx]
    dec di
    jnz mscli_parent
    jmp pc_lookup_no
mscli_found:
    mov es,word ptr es:10h[bx] ; P_UDA
    mov bx,50h                ; U_INITCS
    mov ax,word ptr es:[bx]
    or ax,ax
    jz pc_lookup_no
    mov word ptr mscli_entry,3
    mov word ptr mscli_entry+2,ax
    ret
pc_lookup_no:
    xor ax,ax
    ret
pc_lookup endp

    public pcsysdisk_
pcsysdisk_ proc near
    push es
    push bx
    mov cx,0ch
    int 0e0h
    push ax
    mov cx,9ch
    int 0e0h
    pop ax
    cmp al,31h
    je pc_disk31
    cmp al,41h
    jne pc_disk_unknown
    mov bx,9dh
    jmp pc_disk_read
pc_disk31:
    mov bx,4bh
pc_disk_read:
    xor ax,ax
    mov al,es:[bx]
    inc ax
    jmp pc_disk_end
pc_disk_unknown:
    xor ax,ax
pc_disk_end:
    pop bx
    pop es
    ret
pcsysdisk_ endp

    public pcorder_
pcorder_ proc near
    push bp
    mov bp,sp
    push es
    push bx
    push si
    push di
    call pc_lookup
    or ax,ax
    jz pcorder_no
    mov dx,4[bp]
    mov cx,6
    call dword ptr mscli_entry
    mov ax,bx
    jmp pcorder_end
pcorder_no:
    mov ax,0ffffh
pcorder_end:
    pop di
    pop si
    pop bx
    pop es
    pop bp
    ret
pcorder_ endp

    public pcclean_
pcclean_ proc near
    push bp
    push bx
    push si
    push di
    push es
    xor cx,cx
    call dword ptr mscli_entry
    pop es
    pop di
    pop si
    pop bx
    pop bp
    ret
pcclean_ endp

; p_priority: BDOS fn 145 (91h, P_PRIORITY) -- set the process priority.
; MP/M, Concurrent CP/M (ver_mpm). 0 is the highest priority, 0FFh the
; lowest; transient programs start at 200.
;
; int p_priority(int priority)  -- 0 = ok, -1 = not supported
;
     public	p_priority_
p_priority_	proc	near
        push	bp
        mov 	bp,sp
        push    es
        push	di
        push	si
        push    bx
        call    ver_mpm
        jb      prio_no
        mov 	dx,word ptr 4[bp]
        mov     cx,91h
        int     0E0h
        xor     ax,ax
        jmp     prio_end
prio_no:
        mov     ax,-1
prio_end:
        pop     bx
        pop	    si
        pop 	di
        pop 	es
        pop 	bp
        ret
p_priority_	endp

;
; conattach: BDOS fn 146 (92h, C_ATTACH) -- block until the process's default
; console is available and attach to it. MP/M, Concurrent CP/M (ver_mpm).
; Done automatically by any console I/O while detached.
;
; int conattach(void)  -- 0 = ok, -1 = not supported
;
     public	conattach_
conattach_	proc	near
        push	bp
        mov 	bp,sp
        push    es
        push	di
        push	si
        push    bx
        call    ver_mpm
        jb      attach_no
        xor     dx,dx
        mov     cx,92h
        int     0E0h
        xor     ax,ax
        jmp     attach_end
attach_no:
        mov     ax,-1
attach_end:
        pop     bx
        pop	    si
        pop 	di
        pop 	es
        pop 	bp
        ret
conattach_	endp

;
pd_get	proc	near
	call	ver_mpm
	jb	pd_get_ret		; CF set: unsupported
	mov	cx,9Ch
	int	0E0h			; ES:BX = process descriptor
	clc
pd_get_ret:
	ret
pd_get	endp

     public	getpriority_
getpriority_	proc	near
        push	bp
        mov 	bp,sp
        push    es
        push	di
        push	si
        push    bx
        mov     dx,2
        jmp     pd_go
getpriority_	endp

     public	getppriority_
getppriority_	proc	near
        push	bp
        mov 	bp,sp
        push    es
        push	di
        push	si
        push    bx
        mov     dx,3
pd_go:
        push    dx
        call    pd_get
        pop     dx
        jb      pd_no
        cmp     dx,2
        je      pd_prio
        mov     bx,word ptr es:1Eh[bx]	; parent PD
        or      bx,bx
        jz      pd_no			; no parent
pd_prio:
        xor     ax,ax
        mov     al,byte ptr es:5[bx]	; p_prior
        jmp     pd_end
pd_no:
        mov     ax,-1
pd_end:
        pop     bx
        pop	    si
        pop 	di
        pop 	es
        pop 	bp
        ret
getppriority_	endp


; The DOS translator can leave the caller's BDOS DMA segment changed.
; Return our data segment so the native adapter restores both DMA coordinates.
    public spawn_data_
spawn_data_ proc near
    mov ax,ds
    ret
spawn_data_ endp

codeseg ends
end
