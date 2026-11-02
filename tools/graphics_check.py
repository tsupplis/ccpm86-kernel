#!/usr/bin/env python3
"""Live enhanced-graphics regression; local inputs and disposable PCE disks."""
from pathlib import Path
import shutil,subprocess,tempfile,re,sys
ROOT=Path(__file__).resolve().parent.parent
CDOS=(ROOT/'media/cdos86.cfg').exists()
SYSTEM='cdos86' if CDOS else 'ccpm86'
TOOLS='cdostools' if CDOS else 'ccpmtools'

def check():
    adapter=sys.argv[1] if len(sys.argv)>1 else "vga"
    assert adapter in ("vga","ega","cga")
    defines=[] if adapter=="vga" else ["-D"+adapter.upper()]
    folder=Path(tempfile.mkdtemp(prefix=SYSTEM+'-graphics-'))
    print('Diagnostics:',folder,flush=True)
    def run(*args):return subprocess.run(args,cwd=folder,capture_output=True,check=True)
    shutil.copyfile(ROOT/'media'/(SYSTEM+'.img'),folder/(SYSTEM+'.img'))
    shutil.copyfile(ROOT/'media/diskdefs',folder/'diskdefs')
    run('nasm','-f','bin',str(ROOT/'kernel/rsp3/tests/graphics.asm'),*defines,'-o','graphics.com')
    logical=folder/'logical.img'
    twist=str(ROOT/'tools/cpm86twist.py')
    run('python3',twist,'untwist',str(ROOT/'media'/(SYSTEM+'base.img')),str(logical))
    run('cpmcp','-f','ccpm86-320',str(logical),'graphics.com','0:GRAPHICS.COM')
    run('python3',twist,'twist',str(logical),'fixtures.img')
    cfg=(ROOT/'media'/(SYSTEM+'.cfg')).read_text().replace('driver = "sdl"','driver = "null"')
    cfg=re.sub(r'(?m)^path\s*=\s*"([^"\n]+)"',lambda m:'path = "'+str((ROOT/'media'/m[1]).resolve())+'"',cfg)
    cfg=cfg.replace('driver      = "sdl:lowpass=5000"','driver      = "null"').replace(TOOLS+'.img','fixtures.img')
    (folder/(SYSTEM+'.cfg')).write_text(cfg)
    commands=['c f000000']
    for char in 'b:graphics.com\n':
        key={':':'+Shift Semicolon -Shift','.':'Period','\n':'Return'}.get(char,char)
        commands+=['key '+key,'c 200000']
    commands+=['c 4000000']*8
    commands+=['save "result.bin" bin b800:0000 fa0','save "ram.bin" bin 0000:0000 a0000','q']
    with (folder/'pce.log').open('wb') as log:
        subprocess.run(['pce-ibmpc','-c',SYSTEM+'.cfg','-t','null','-g',adapter],cwd=folder,
                       input=('\n'.join(commands)+'\n').encode(),stdout=log,stderr=log,
                       timeout=60,check=True)
    video=(folder/'result.bin').read_bytes()
    screen='\n'.join(''.join(chr(c) if 32<=c<127 else ' ' for c in video[i:i+160:2]) for i in range(0,3840,160))
    assert 'GRAPHICS PASS' in screen and 'FAIL' not in screen,screen
    assert [l.strip() for l in screen.splitlines() if l.strip()][-1]=='A>',screen
    description={"vga":"EGA 0D/0E/10 and VGA 11/12/13, pixels/XOR, bounds/pages, palettes/DAC, cursor/character/scroll",
                 "ega":"EGA 0D/0E/10, pixels/XOR, bounds/pages, palette writes, cursor/character/scroll",
                 "cga":"CGA 4/5/6 pixel read/write and XOR"}[adapter]
    print(adapter+': PASS: '+description+'; text restoration',flush=True)

if __name__=='__main__':check()
