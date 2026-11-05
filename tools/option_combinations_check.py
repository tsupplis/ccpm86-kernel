#!/usr/bin/env python3
"""Native, disposable combined-option checks for DIR, SDIR and TYPE.

Run from either kernel project. CCP/M exercises DIR.RSP and DIR.CMD; CDOS
exercises shell DIR and DIR.CMD. The projects do not read each other's files.
PCE reads keystrokes in a single bounded run. A Ctrl-C closes each paging case.
"""
import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
CCPM=(ROOT/'media/ccpm86.cfg').exists()
OS='CCP/M' if CCPM else 'CDOS'
PAUSE='Press any key to continue...'
KEYS={' ':'Space',':':'+Shift Semicolon -Shift','.':'Period','[':'LeftBracket',
      ']':'RightBracket',',':'Comma',';':'Semicolon','=':'Equal',
      '(':'+Shift 9 -Shift',')':'+Shift 0 -Shift'}


def check():
    folder=Path(tempfile.mkdtemp(prefix='dir-combinations-'))
    print(OS,'diagnostics:',folder,flush=True)
    def run(*args):
        return subprocess.run(args,cwd=folder,capture_output=True,check=True)
    try:
        shutil.copyfile(ROOT/'media/diskdefs',folder/'diskdefs')
        twist=str(ROOT/'tools/cpm86twist.py')
        base=ROOT/('media/ccpm86base.img' if CCPM else 'media/cdos86base.img')
        run('python3',twist,'untwist',str(base),'boot.logical')
        run('cpmcp','-f','ccpm86-320','boot.logical',str(ROOT/'kernel/sysgen/ccpm.sys'),'0:CCPM.SYS')
        for name in ('dir','sdir','type'):
            run('cpmcp','-f','ccpm86-320','boot.logical',str(ROOT/'commands'/name/(name+'.cmd')),'0:')
        run('python3',twist,'twist','boot.logical','boot.img')
        run('python3',twist,'untwist',str(base),'data.logical')
        data=bytearray((folder/'data.logical').read_bytes())
        data[4096:6144]=b'\xe5'*2048
        (folder/'data.logical').write_bytes(data)
        (folder/'empty').write_bytes(b'\x1a')
        for prefix,user in (('N',0),('S',0),('U',1)):
            for n in range(2 if user else 30):
                run('cpmcp','-f','ccpm86-320','data.logical','empty',f'{user}:{prefix}{n:02}.TXT')
        run('cpmchattr','-f','ccpm86-320','data.logical','s','0:S*.TXT')
        (folder/'long.txt').write_bytes(b''.join(f'line {n:02}\r\n'.encode() for n in range(50))+b'\x1a')
        run('cpmcp','-f','ccpm86-320','data.logical','long.txt','0:LONG.TXT')
        run('python3',twist,'twist','data.logical','data.img')
        config=ROOT/('media/ccpm86.cfg' if CCPM else 'media/cdos86.cfg')
        cfg=config.read_text().replace('driver = "sdl"','driver = "null"')
        cfg=re.sub(r'driver\s*=\s*"sdl[^"]*"','driver = "null"',cfg)
        cfg=re.sub(r'(?m)^path\s*=\s*"([^"\n]+)"',lambda m:'path = "'+str((config.parent/m[1]).resolve())+'"',cfg)
        cfg=re.sub(r'disk\s*\{.*?\}', '', cfg, flags=re.S)
        cfg+='\ndisk { drive=0 type="auto" file="boot.img" optional=0 }\n'
        cfg+='disk { drive=1 type="auto" file="data.img" optional=0 }\n'
        (folder/'cfg').write_text(cfg)
        # command, must pause (None = either), selected filename prefix, error
        cases=[]
        for command in ('dir','dir.cmd'):
            specs='b: /l' if not CCPM and command=='dir' else 'b: b: b: b: b:'
            for option,prefix in (('[sp','S'),('[ps]','S'),('[pg0','N'),('[g0p]','N'),
                                  ('[spg0]','S'),('[g0sp]','S'),('[s,p,g0]','S'),
                                  ('[s p g0]','S'),('[s][p][g0]','S'),('[g1p]','U')):
                cases.append((f'{command} {specs} {option}',prefix!='U',prefix,False))
            cases.append((f'{command} {specs} [sg0]',False,'S',False))
            cases.append((f'{command} {specs}',False,'N',False))
            for option in ('[pg16]','[gp]','[spx]','[pg999]'):
                cases.append((f'{command} b: {option}',False,None,True))
        if not CCPM:
            for option in ('/S /P /G0','[S] /P [G0]','/P [S,G0]'):
                cases.append(('dir b: /l '+option,True,'S',False))
        for option,paused,prefix in (
            ('[full dir sys page user=all]',True,None),
            ('[page,user=all,sys,dir,full]',True,None),
            ('[size dir sys page user=(0,1)]',True,None),
            ('[short sys user=0 nopage]',False,'S'),
            ('[short,user=1,dir,nopage]',False,'U'),
            ('[page nopage full dir sys user=all]',False,None),
            ('[nopage page full dir sys user=all]',True,None),
            ('[page full sys user=0]',None,'S')):
            cases.append(('sdir.cmd b: '+option,paused,prefix,False))
        for tail in ('[page]','[p10]'):
            cases.append(('type.cmd b:long.txt;secret '+tail,True,None,False))
        for tail in ('[page p10]','[page][p10]'):
            cases.append(('type.cmd b:long.txt '+tail,False,None,True))
        script=['c f000000']
        def command(text):
            for char in text:
                script.extend(['key '+KEYS.get(char,char),'c 200000'])
            script.extend(['key Return','c 4000000'])
        for i,(text,_,_,_) in enumerate(cases):
            command('cls')
            command(text)
            script.append(f'save "case{i}.bin" bin b800:0000 fa0')
            script.extend(['key +Ctrl c -Ctrl','c 2000000'])
        script.append('q')
        with (folder/'pce.log').open('wb') as log:
            subprocess.run(['pce-ibmpc','-c','cfg','-t','null','-g','vga'],cwd=folder,
                           input=('\n'.join(script)+'\n').encode(),stdout=log,stderr=log,
                           check=True,timeout=300)
        for i,(text,pause,prefix,error) in enumerate(cases):
            raw=(folder/f'case{i}.bin').read_bytes()
            screen='\n'.join(raw[a:a+160:2].decode('cp437').rstrip() for a in range(0,3840,160))
            (folder/f'case{i}.txt').write_text(screen)
            invalid=any(word in screen.lower() for word in ('invalid','illegal','bad filename','load error','command not found',"can't find"))
            assert invalid==error,(text,'error status',screen)
            if error:continue
            if pause is not None:assert (PAUSE in screen)==pause,(text,'paging status',screen)
            if prefix:
                assert re.search(r'\b'+prefix+r'\d{2}\b',screen),(text,'missing selected files',screen)
                for other in ('N','S','U'):
                    if other!=prefix:assert not re.search(r'\b'+other+r'\d{2}\b',screen),(text,'wrong group',screen)
        print(f'PASS: {OS}, {len(cases)} DIR / SDIR / TYPE option combinations; paging, filters, users, invalid input and state reset',flush=True)
    except BaseException:
        print('Retained diagnostics:',folder,flush=True)
        raise
    else:
        shutil.rmtree(folder)


if __name__=='__main__':
    check()
