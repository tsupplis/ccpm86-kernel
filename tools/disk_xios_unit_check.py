#!/usr/bin/env python3
"""Execute built XIOS raw-floppy validation/state and format DMA arithmetic.

Hardware service boundaries are stubbed; PCE tests cover actual controller I/O.
Both native-only and PC-MODE builds must expose identical function numbers.
"""
from pathlib import Path
import re,subprocess,tempfile,struct
ROOT=Path(__file__).resolve().parent.parent
C=ROOT/'kernel/con'
RUNS=ROOT/'private/native-tests/xios-unit'
RUNS.mkdir(parents=True,exist_ok=True)
count=0
for module in ['xios','xiosnat']:
    symbols={n:int(v,16) for v,n in re.findall(r'\b([0-9A-F]{4})\s+([A-Z][A-Z0-9]*)',(C/(module+'.sym')).read_text())}
    S=lambda n:symbols[n.replace('_','').upper()]
    b=(C/(module+'.con')).read_bytes();base=bytearray(b[128:128+int.from_bytes(b[1:3],'little')*16])
    # Verify every generated descriptor in the linked production image.
    # Expected layout is the shared filesystem contract, not generator text.
    for name,spt,bsh,blm,exm,dsm,drm,al0,cks,off in [
            ('dpb0',8,3,7,0,155,63,0xc0,16,1),
            ('dpb1',8,4,15,1,157,63,0x80,16,1),
            ('dpb2',9,4,15,0,354,255,0xf0,64,2),
            ('dpb5',15,5,31,1,295,255,0xc0,64,2),
            ('dpb3',18,5,31,1,354,255,0xc0,64,2)]:
        expected=struct.pack('<HBBBHHBBHHBB',spt,bsh,blm,exm,
                             dsm,drm,al0,0,cks,off,2,3)
        assert base[S(name):S(name)+17]==expected,(module,name)
        count+=1
    table=S('function_table')
    for number,name in [(29,'io_variable_format'),(35,'io_raw_read'),(36,'io_raw_write')]:
        assert int.from_bytes(base[table+2*number:table+2*number+2],'little')==S(name),(module,number)
    def assemble(src,p,name,origin):
        (p/(name+'.asm')).write_text(f'bits 16\ncpu 8086\norg {origin}\n'+src)
        subprocess.run(['nasm','-f','bin',name+'.asm','-o',name+'.bin'],cwd=p,check=True,capture_output=True)
        return (p/(name+'.bin')).read_bytes()
    def run(entry,setup,checks,patches=None,packet=None,*,following=()):
        global count
        with tempfile.TemporaryDirectory(prefix='case-',dir=RUNS) as td:
            p=Path(td);code=bytearray(base)
            for name,stub in (patches or {}).items():
                dest=len(code);code+=assemble(stub,p,'stub',dest)
                at=S(name);code[at:at+3]=b'\xe9'+((dest-at-3)&65535).to_bytes(2,'little')
            calls=[]
            targets=[]
            for subroutine,iopb in ((entry,packet),*following):
                target=len(code)
                code+=assemble(f'call {S(subroutine)}\nretf',p,'entry',target)
                label=f'target{len(targets)}'
                targets.append((label,target))
                if iopb:
                    drive,track,sector,sectors,formatting=iopb
                    words=[drive+(sectors<<8),track,sector,'ds',0 if following else 0xd000]
                    if formatting:
                        words.insert(0,0xf650)
                    for word in words:
                        calls.append('push ds' if word=='ds' else f'mov ax,{word}\n push ax')
                calls.append(f'call far [cs:{label}]')
                if iopb:
                    calls.append(f'add sp,{12 if formatting else 10}')
                if following:
                    calls.append('test al,al\n jnz failed')
            (p/'payload.bin').write_bytes(code)
            segments='\n'.join(f'mov [{label}+2],ax' for label,_ in targets)
            pointers='\n'.join(f'{label} dw {target},0' for label,target in targets)
            invocation='\n'.join(calls)
            driver=f'''bits 16
cpu 8086
org 100h
 mov ax,cs
 add ax,(payload-$$+100h)/16
{segments}
 mov ds,ax
 mov es,ax
 mov word [0f000h],0
 mov word [0f010h],0ffffh
{setup}
{invocation}
{checks}
 mov ax,4c00h
 int 21h
failed:
 mov ax,4c01h
 int 21h
{pointers}
align 16
payload: incbin "payload.bin"
'''
            (p/'test.asm').write_text(driver)
            subprocess.run(['nasm','-f','bin','test.asm','-o','test.com'],cwd=p,check=True,capture_output=True)
            result=subprocess.run(['emu2','test.com'],cwd=p,capture_output=True,timeout=5)
            assert result.returncode==0,(module,entry,setup,packet,result.stderr)
        count+=1
    # Execute the production marker decoder; neither OS calls nor FDC I/O
    # participate in this boundary. Check both native and legacy signatures.
    for signature,marker,profile in [(0,0,'dpb0'),(0,1,'dpb1'),
            (1,72,'dpb2'),(1,120,'dpb5'),(1,144,'dpb3'),
            (0,72,None),(0,120,None),(0,144,None),(1,99,None)]:
        expected=S(profile) if profile else 0
        run('floppyprofile',
            f'mov byte [{S("localbuffer")+510}],{signature}\n'
            f'mov byte [{S("floppytype")}],{marker}',
            f' cmp ax,{expected}\n jne failed')
    # Exercise login probing through the real IOPB construction. Only the
    # controller read boundary is replaced; rate order and cleanup are real.
    for successful_attempt,mode,response in [
            (1,0x23,None),(2,0x23,None),(3,0x23,None),(6,0x23,'R'),
            (0,0x23,'A'),(0,0x23,'I'),(0,0xfe,None),(0,0xff,None)]:
        stub=f"""mov bp,sp
 cmp word [ss:bp+14],0101h
 jne failed
 cmp word [ss:bp+12],0
 jne failed
 cmp word [ss:bp+10],0
 jne failed
 cmp byte [10h],0feh
 jne failed
 xor bx,bx
 mov bl,[0f000h]
 inc byte [0f000h]
 cmp bl,6
 jae failed
 mov al,[cs:rates+bx]
 cmp byte [{S('flprates')+1}],al
 jne failed
 mov byte [{S('flprate')}],al
 cmp byte [0f000h],{successful_attempt}
 jne readerror
 mov byte [{S('localbuffer')+510}],1
 mov byte [{S('floppytype')}],120
 xor ax,ax
 ret
readerror:
 mov byte [{S('diskerror')}],4
 mov ax,1
 ret
failed:
 mov ax,4c02h
 int 21h
rates db 2,0,1,2,0,1
"""
        # Respond through the production error formatter. A recognised but
        # stale 720K marker must never be published after a failed read.
        prompt=f"""cmp byte [10h],{mode}
 jne failed
 cmp si,{S('diskermsg')}
 jne failed
 cmp di,{S('diskerresp')}
 jne failed
 cmp byte [{S('diskerdrive')}],'B'
 jne failed
 cmp word [{S('diskertrack')}],'0 '
 jne failed
 cmp word [{S('diskertrack')+2}],'  '
 jne failed
 push ds
 push cs
 pop ds
 mov si,detail
 mov di,{S('diskersub')}
 mov cx,detail_end-detail
 repe cmpsb
 pop ds
 jne failed
 inc byte [0f001h]
 mov al,{ord(response) if response else 0}
 ret
failed:
 mov ax,4c03h
 int 21h
detail db ' Record Not Found  '
detail_end:
"""
        dph=S('dph1')
        setup=f"""cld
 mov cx,1
 mov bx,{dph}
 mov byte [10h],{mode}
 mov word [{S('flprates')}],0
 mov byte [{S('localbuffer')+510}],1
 mov byte [{S('floppytype')}],72"""
        checks=f"""cmp byte [10h],{mode}
 jne failed
 cmp byte [0f000h],{successful_attempt or 3}
 jne failed
 cmp byte [0f001h],{int(response is not None)}
 jne failed
 cmp word [{S('flprates')}],{[0,2,0,1,0,0,1][successful_attempt]<<8}
 jne failed
 cmp bx,{dph if successful_attempt else 0}
 jne failed
 cmp word [{dph+8}],{S('dpb5' if successful_attempt else 'dpb3')}
 jne failed
 cmp word [{S('flpsects')}],{0x0f08 if successful_attempt else 0x0808}
 jne failed
 cmp word [{S('flpcyls')}],{0x5028 if successful_attempt else 0x2828}
 jne failed"""
        run('floppysize',setup,checks,{'io_read':stub,'slerrorout':prompt})
    # Formatting must not inherit a previous high-density drive's rate.
    for previous_rate in (0,1,2):
        run('io_format',
            f'mov byte [{S("flprate")}],{previous_rate}\n'
            f'mov byte [{S("flpcylinders")}],80',
            ' cmp byte [0f000h],1\n jne failed',
            {'disk_init':f"""cmp byte [{S('flprate')}],2
 jne failed
 cmp byte [{S('flpcylinders')}],40
 jne failed
 mov byte [0f000h],1
 ret
failed:
 mov ax,4c02h
 int 21h
""", 'check_disk_op':'xor ax,ax\nret'},(0,0,0,8,False))
    # Follow the maintenance path into normal XIOS I/O without a login
    # between calls. Check the actual SEEK command, not a duplicated mapping.
    for profile,rate in [(0,2),(9,2),(15,0),(18,0)]:
        sequence=[]
        expected=[]
        read,write,format_cmd=0x66,0x45,0x4d

        def operation(entry,packet,command,geometry,position):
            sequence.append((entry,packet))
            expected.append((command,*geometry,*position))

        legacy=(2,40,8)
        # The DPH reserves the largest DPB even before any disk has logged in.
        operation('io_read',(0,40,7,1,False),read,legacy,(0,39,1))
        if profile:
            native=(rate,80,profile)
            operation('io_variable_format',(0,40,profile<<8,profile,True),
                      format_cmd,native,(0,40,0))
            operation('io_read',(0,40,profile-1,1,False),read,native,(0,40,0))
            # Give B its own geometry before formatting A back to 320K.
            operation('io_variable_format',(1,40,15<<8,15,True),
                      format_cmd,(0,80,15),(1,40,0))
        operation('io_format',(0,40,0,8,False),format_cmd,legacy,(0,39,1))
        operation('io_write',(0,40,7,1,False),write,legacy,(0,39,1))
        operation('io_read',(0,40,7,1,False),read,legacy,(0,39,1))
        operation('io_raw_read',(0,40,18<<8,1,False),read,(0,80,18),(0,40,0))
        operation('io_read',(1,40,7,1,False),read,
                  (0,80,15) if profile else legacy,(1,40,0) if profile else (1,39,1))
        operation('io_read',(0,40,7,1,False),read,legacy,(0,39,1))

        # diskinit normally calls SEEK, so keep it in the path while replacing
        # the actual controller boundary with a command validator.
        command_checks=[]
        for i,(command,speed,cylinders,eot,drive,cylinder,head) in enumerate(expected):
            command_checks.append(f'case{i}:')
            fields=[('fdccommand',command),('flprate',speed),('flpcylinders',cylinders),
                    ('dadrive',drive+4*head),('dacylinder',cylinder),('dahead',head),
                    ('raweot',eot)]
            for field,value in fields:
                # Keep each conditional within the 8086's short-jump range.
                command_checks.append(f'cmp byte [{S(field)}],{value}\n je $+5\n jmp near failed')
            command_checks.append('ret')
        cases='\n'.join(command_checks)
        targets=','.join(f'case{i}' for i in range(len(expected)))
        controller=f"""xor bx,bx
 mov bl,[0f000h]
 cmp bl,{len(expected)}
 jae failed
 inc byte [0f000h]
 shl bx,1
 jmp word [cs:targets+bx]
failed:
 mov ax,[0f000h]
 add al,10
 mov ah,4ch
 int 21h
targets dw {targets}
{cases}
"""
        setup=f"""cmp word [{S('dph0')+8}],{S('dpb3')}
 jne failed
 cmp word [{S('dph1')+8}],{S('dpb3')}
 jne failed"""
        checks=f"""cmp byte [0f000h],{len(expected)}
 jne failed
 cmp word [{S('dph0')+8}],{S('dpb3')}
 jne failed
 cmp word [{S('dph1')+8}],{S('dpb3')}
 jne failed"""
        entry,packet=sequence[0]
        run(entry,setup,checks,{'disk_init':f"call {S('seek')}\nret",
                               'fdc_command_put':controller},
            packet,following=sequence[1:])
    run('io_pointer','mov cx,0\n mov dx,444bh\n mov si,534ch',
        ' cmp ax,4b44h\n jne failed\n cmp dx,0102h\n jne failed')
    run('io_pointer','mov cx,0\n mov dx,444bh\n mov si,534bh', ' cmp ax,4b44h\n jne failed\n cmp dx,0101h\n jne failed')
    run('io_pointer','mov cx,0\n xor dx,dx\n xor si,si',f' cmp ax,{S("equip_data")}\n jne failed')
    cases=[(0,0,0,8),(1,0x127,8,1),(0,0x100,0,9),
           (2,0,0,8),(0,40,0,8),(0,0x200,0,8),
           (0,0,9,1),(0,0,0,0),(0,0,0,10),(0,0,8,2),
           (0,79,9<<8,9),(1,0x14f,(15<<8)+14,1),
           (0,0x100,18<<8,18),(0,80,9<<8,1),
           (0,0,(15<<8)+14,2),(0,0,10<<8,1)]
    for entry,callee in [('io_raw_read','io_read'),('io_raw_write','io_write')]:
        for drive,track,sector,sectors in cases:
            profile=sector>>8
            eot=profile or 9
            cylinders=80 if profile else 40
            physical_sector=sector&255
            valid=(profile in (0,9,15,18) and drive<2 and
                   (track&255)<cylinders and (track>>8)<2 and
                   physical_sector<eot and 1<=sectors<=eot and
                   physical_sector+sectors<=eot)
            logical=(2*cylinders-1-(track&255)) if track>>8 else track&255
            stub=f'''mov bp,sp
 cmp byte [{S('raw_eot')}],{eot}
 jne failed
 cmp word [ss:bp+12],{logical}
 jne failed
 cmp word [ss:bp+10],{physical_sector}
 jne failed
 mov byte [0f000h],1
 mov ax,1
 ret
failed: mov ax,4c02h
 int 21h
'''
            run(entry,'',f''' cmp ax,{1 if valid else 0x0401}
 jne failed
 cmp byte [0f000h],{int(valid)}
 jne failed
 cmp byte [{S('raw_eot')}],8
 jne failed
''',{callee:stub},(drive,track,sector,sectors,False))
    for drive,track,sectors,profile in [
            (0,0,8,0),(1,79,9,0),(2,0,8,0),(0,80,8,0),
            (0,0,7,0),(0,0,10,0),
            (0,159,9,9),(1,159,15,15),(0,159,18,18),
            (0,160,18,18),(0,0,8,9),(0,0,9,15),
            (2,0,18,18),(0,0,10,10)]:
        valid=(drive<2 and track<(160 if profile else 80) and
               profile in (0,9,15,18) and
               (sectors==profile if profile else sectors in (8,9)))
        stub=f'''cmp byte [{S('raw_format_count')}],{sectors}
 jne failed
 cmp word [{S('raw_gap_fill')}],0f650h
 jne failed
 mov byte [0f000h],1
 mov ax,1
 ret
failed: mov ax,4c02h
 int 21h
'''
        run('io_variable_format','',f''' cmp ax,{1 if valid else 0x0401}
 jne failed
 cmp byte [0f000h],{int(valid)}
 jne failed
 cmp byte [{S('raw_format_count')}],0
 jne failed
''',{'format_floppy':stub},(drive,track,profile<<8,sectors,True))
    # Execute real DMA count selection, stubbing port writes only.
    original=base
    with tempfile.TemporaryDirectory(prefix="dma-",dir=RUNS) as td:
        p=Path(td);begin=S('dma_setup');end=S('motor_on')
        (p/'dma.bin').write_bytes(base[begin:end])
        output=subprocess.check_output(['ndisasm','-b','16','dma.bin'],cwd=p,text=True)
        base=bytearray(original)
        for line in output.splitlines():
            parts=line.split()
            if len(parts)>2 and parts[2].lower()=='out':
                at=begin+int(parts[0],16);base[at:at+len(parts[1])//2]=b'\x90'*(len(parts[1])//2)
        at=S('variable_dma_count')
        stub=f'''dec ax
 mov [0f010h],ax
 jmp near {at+3}
'''
        # The generic hook is a relocated near jump. Assemble a stub using
        # its real appended origin so the return enters the existing routine.
        destination=len(base)
        (p/'capture.asm').write_text(f'bits 16\norg {destination}\n'+stub)
        subprocess.run(['nasm','-f','bin','capture.asm','-o','capture.bin'],cwd=p,check=True,capture_output=True)
        base+=(p/'capture.bin').read_bytes()
        base[at:at+3]=b'\xe9'+((destination-at-3)&65535).to_bytes(2,'little')
        for formatted in [0,8,9,15,18]:
            run('dma_setup',f'mov byte [{S("raw_format_count")}],{formatted}\n mov byte [{S("num_sec")}],9',f' cmp word [0f010h],{formatted*4-1 if formatted else 4607}\n jne failed')
    base=original
print(f'PASS: {count} assembled XIOS cases across PC-MODE/native builds: capability/table numbers, CHS/count bounds, state restoration, seek geometry, login error responses and format/read DMA byte counts')
