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
    assert adapter in ("vga","ega","cga","egamono","hgc","mda")
    fixture=sys.argv[2] if len(sys.argv)>2 else "graphics"
    assert fixture in ("graphics","text40","text40native","hercules")
    defines=[] if adapter=="vga" else ["-D"+adapter.upper()]
    if adapter=="egamono": defines.append("-DEGA")
    if CDOS: defines.append("-DCDOS")
    folder=Path(tempfile.mkdtemp(prefix=SYSTEM+'-graphics-'))
    print('Diagnostics:',folder,flush=True)
    def run(*args):return subprocess.run(args,cwd=folder,capture_output=True,check=True)
    boot='ccpmnative.img' if fixture in ('text40native','hercules') else SYSTEM+'.img'
    extension='cmd' if fixture in ('text40native','hercules') else 'com'
    shutil.copyfile(ROOT/'media'/boot,folder/(SYSTEM+'.img'))
    shutil.copyfile(ROOT/'media/diskdefs',folder/'diskdefs')
    run('nasm','-f','bin',str(ROOT/'kernel/rsp3/tests'/(fixture+'.asm')),*defines,'-o','graphics.'+extension)
    logical=folder/'logical.img'
    twist=str(ROOT/'tools/cpm86twist.py')
    run('python3',twist,'untwist',str(ROOT/'media'/(SYSTEM+'base.img')),str(logical))
    run('cpmcp','-f','ccpm86-320',str(logical),'graphics.'+extension,'0:GRAPHICS.'+extension.upper())
    run('python3',twist,'twist',str(logical),'fixtures.img')
    cfg=(ROOT/'media'/(SYSTEM+'.cfg')).read_text().replace('driver = "sdl"','driver = "null"')
    cfg=re.sub(r'(?m)^path\s*=\s*"([^"\n]+)"',lambda m:'path = "'+str((ROOT/'media'/m[1]).resolve())+'"',cfg)
    cfg=cfg.replace('driver      = "sdl:lowpass=5000"','driver      = "null"').replace(TOOLS+'.img','fixtures.img')
    if adapter=="egamono": cfg=cfg.replace("switches = 0x09","switches = 0x0a")
    (folder/(SYSTEM+'.cfg')).write_text(cfg)
    commands=['c f000000']
    for char in 'b:graphics.'+extension+'\n':
        key={':':'+Shift Semicolon -Shift','.':'Period','\n':'Return'}.get(char,char)
        commands+=['key '+key,'c 200000']
    commands+=['c 4000000']*8
    segment='b000' if adapter in ('egamono','hgc','mda') else 'b800'
    commands+=[f'save "result.bin" bin {segment}:0000 fa0','save "ram.bin" bin 0000:0000 a0000','q']
    with (folder/'pce.log').open('wb') as log:
        subprocess.run(['pce-ibmpc','-c',SYSTEM+'.cfg','-t','null','-g','ega' if adapter=='egamono' else adapter],cwd=folder,
                       input=('\n'.join(commands)+'\n').encode(),stdout=log,stderr=log,
                       timeout=60,check=True)
    video=(folder/'result.bin').read_bytes()
    screen='\n'.join(''.join(chr(c) if 32<=c<127 else ' ' for c in video[i:i+160:2]) for i in range(0,3840,160))
    if fixture=='text40':
        ram=(folder/'ram.bin').read_bytes()
        marker=ram.find(b'TEXT40 RESULT:')
        assert marker>=0 and ram[marker+16:marker+22]==b'PASSED', (screen,ram[marker:marker+22])
    else:
        assert fixture.upper()+' PASS' in screen and 'FAIL' not in screen,screen
    assert [l.strip() for l in screen.splitlines() if l.strip()][-1]=='A>',screen
    if adapter=='hgc' and fixture=='graphics':
        ram=(folder/'ram.bin').read_bytes()
        assert ram[0x449]==7 and int.from_bytes(ram[0x44a:0x44c],'little')==80, 'DOS exit did not restore monochrome BIOS mode/columns'
    description={"mda":"MDA text and rejection of Hercules graphics",
                 "hgc":"Hercules project DOS mode 7F: pixels/XOR, graphics text, mode query and DOS-exit restoration",
                 "egamono":"EGA monochrome 0F, pixels/XOR, bounds/pages, palette, cursor/character/scroll",
                 "vga":"CGA 4/5/6, EGA 0D/0E/0F/10 and VGA 11/12/13, pixels/XOR, bounds/pages, palettes/DAC, cursor/character/scroll",
                 "ega":"CGA 4/5/6 and EGA 0D/0E/10, pixels/XOR, bounds/pages, palette writes, cursor/character/scroll",
                 "cga":"CGA 4/5/6 mode/columns, pixels/XOR, bounds/pages, cursor/character/TTY/scroll"}[adapter]
    if fixture=='text40': description='40-column text modes 0/1, query, physical stride, cursor, characters, TTY/wrap, scroll and DOS exit'
    if fixture=='text40native': description='native-only 40-column selection, output, line insertion/deletion, bottom-row wrap and restoration'
    if fixture=='hercules': description=('native Hercules pixels/XOR, glyphs, simple/packet cursors, repeat/wrap, scrolling, bounds/pages and text restoration' if adapter=='hgc' else 'Hercules capability absent and mode rejected')
    print(adapter+': PASS: '+description+'; text restoration',flush=True)

if __name__=='__main__':check()
