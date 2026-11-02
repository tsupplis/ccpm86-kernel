#!/usr/bin/env python3
"""Same EXE-loader regression in each independent kernel project.

NASM assembles disposable test programs only. Every emulator/disk is temporary;
no sibling source, build output or runtime participates in this test.
"""
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parent.parent
MEDIA=ROOT/'media'
CDOS=(MEDIA/'cdos86.cfg').exists()
SYSTEM='cdos86' if CDOS else 'ccpm86'
TOOLS='cdostools' if CDOS else 'ccpmtools'
FIXTURE=ROOT/'kernel/rsp3/tests/exe_header.asm'
# All possible paragraph offsets within a 128-byte record, larger headers,
# a >64-KiB image and both 0/512 full-final-page conventions.
CASES=[(h,337,'valid') for h in [32,48,64,80,96,112,128,144,256,512,528,65552]]
CASES += [(32,65553,'valid'),(48,464,'full512'),(512,512,'full512'),
          (32,337,'truncated'),(32,337,'bad-header'),(32,337,'bad-signature')]


def check(start):
    cases=CASES[start:start+3]
    with tempfile.TemporaryDirectory(prefix=SYSTEM+'-exe-') as temporary:
        folder=Path(temporary)
        def run(*args):
            result=subprocess.run(args,cwd=MEDIA,capture_output=True)
            assert result.returncode==0,(args,result.stderr.decode(errors='replace'))
            return result
        twist=str(ROOT/'tools/cpm86twist.py')
        shutil.copyfile(MEDIA/(SYSTEM+'.img'),folder/(SYSTEM+'.img'))
        logical=folder/'logical.img'
        run('python3',twist,'untwist',str(MEDIA/(SYSTEM+'base.img')),str(logical))
        assert logical.exists(), ('twist output missing',list(folder.iterdir()))
        for local,(header_bytes,image_bytes,kind) in enumerate(cases):
            number=start+local
            payload=folder/'payload.bin'
            run('nasm','-f','bin',str(FIXTURE),'-DCASEID='+str(number),
                '-DTARGET_BYTES='+str(image_bytes),'-o',str(payload))
            image=payload.read_bytes()
            size=header_bytes+len(image)
            last=size%512
            if kind=='full512':last=512
            # Relocation at header byte 28 targets image offset 0, segment 0.
            header=struct.pack('<14H',0x5a4d,last,(size+511)//512,1,
                               header_bytes//16,0x200,0xffff,0,0x1800,0,0,1,28,0)
            header+=struct.pack('<HH',0,0)+b'\0'*(header_bytes-32)
            executable=bytearray(header+image)
            if kind=='truncated':executable=executable[:header_bytes+16]
            if kind=='bad-header':struct.pack_into('<H',executable,8,1)
            if kind=='bad-signature':executable[:2]=b'ZZ'
            target=folder/f'h{number:02}.exe'
            target.write_bytes(executable)
            run('cpmcp','-f','ccpm86-320',str(logical),str(target),'0:'+target.name)
        # COM loading and post-failure recovery in every emulator batch.
        com=folder/'control.com'
        run('nasm','-f','bin',str(FIXTURE),'-DCOM','-DCASEID='+str(start),'-o',str(com))
        run('cpmcp','-f','ccpm86-320',str(logical),str(com),'0:control.com')
        run('python3',twist,'twist',str(logical),str(folder/'fixtures.img'))
        cfg=(MEDIA/(SYSTEM+'.cfg')).read_text()
        cfg=cfg.replace('driver = "sdl"','driver = "null"')
        cfg=cfg.replace('driver      = "sdl:lowpass=5000"','driver      = "null"')
        cfg=cfg.replace(TOOLS+'.img','fixtures.img')
        # Optional extra drives are absent in this isolated run.
        (folder/(SYSTEM+'.cfg')).write_text(cfg)
        commands=['c f000000']
        def command(text,label):
            for char in text+'\n':
                key={':':'+Shift Semicolon -Shift','.':'Period','\n':'Return'}.get(char,char)
                commands.extend(['key '+key,'c 200000'])
            commands.extend(['c 8000000',f'save "{label}.bin" bin b800:0000 fa0'])
        for local,_ in enumerate(cases):
            number=start+local
            command(f'b:h{number:02}.exe',f'h{number:02}')
        command('b:control.com','control')
        commands.append('q')
        with (folder/'pce.log').open('wb') as log:
            subprocess.run(['pce-ibmpc','-c',SYSTEM+'.cfg','-t','null','-g','vga'],
                           cwd=folder,input=('\n'.join(commands)+'\n').encode(),
                           stdout=log,stderr=log,timeout=60,check=True)
        def screen(label):
            video=(folder/(label+'.bin')).read_bytes()
            return '\n'.join(''.join(chr(v) if 32<=v<127 else ' ' for v in video[i:i+160:2])
                             for i in range(0,3840,160))
        for local,(header_bytes,image_bytes,kind) in enumerate(cases):
            number=start+local
            text=screen(f'h{number:02}')
            expected=f'EXE H{number:02} PASS'
            if kind in ['valid','full512']:
                assert expected in text and f'EXE H{number:02} FAIL' not in text,(header_bytes,image_bytes,text)
            else:
                assert expected not in text,(kind,text)
            lines=[line.strip() for line in text.splitlines() if line.strip()]
            assert lines[-1]=='A>',(kind,text)
        text=screen('control')
        assert f'COM H{start:02} PASS' in text,text
        assert [line.strip() for line in text.splitlines() if line.strip()][-1]=='A>',text
    print(f'PASS {SYSTEM}: EXE cases {start:02}–{start+len(cases)-1:02}; relocation, entry, tail bytes and COM recovery',flush=True)


if __name__=='__main__':
    for start in range(0,len(CASES),3):check(start)
