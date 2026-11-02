; Host-test fixture only. Production kernels use ASM86/RASM86, never NASM.
bits 16
cpu 8086
%ifndef CASEID
%define CASEID 0
%endif
%ifdef COM
org 100h
 mov dx,message
 mov ah,9
 int 21h
 int 20h
message db 'COM H', '0'+CASEID/10, '0'+CASEID % 10, ' PASS',13,10,'$'
%else
org 0
; One MZ relocation rebases this word to the actual image segment.
relocated_segment dw 0
 times 16-($-$$) db 0cch
; Header CS=1, IP=0: exercise a nonzero entry segment.
entry:
 mov ax,cs
 dec ax
 mov ds,ax
 cmp [relocated_segment],ax
 jne failed
 mov bx,ss
 cmp bx,ax
 jne failed
 mov bx,ds
 add bx,(TARGET_BYTES-1)/16
 mov es,bx
 cmp byte [es:(TARGET_BYTES-1) % 16],0a5h
 jne failed
 mov dx,success
 jmp report
failed:
 mov dx,failure
report:
 mov ah,9
 int 21h
 ; Legacy termination is supported by both personalities. MAXALLOC=FFFFh
 ; selects low loading, so the PSP is one paragraph group before the image.
 mov ax,ds
 sub ax,10h
 push ax
 xor ax,ax
 push ax
 retf
success db 'EXE H', '0'+CASEID/10, '0'+CASEID % 10, ' PASS',13,10,'$'
failure db 'EXE H', '0'+CASEID/10, '0'+CASEID % 10, ' FAIL',13,10,'$'
 times TARGET_BYTES-1-($-$$) db 066h
 db 0a5h
%endif
