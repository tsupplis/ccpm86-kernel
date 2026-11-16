"""Setup/Backup acceptance on disposable copies.

FM_TEST_FIXTURE: browse.py fixture containing cdos86.img, fat.img, cdos86.cfg
and the STOP launch probe. FM_TEST_HARD: 4.1 disk with FAT at sector 1 and the
reference root catalog (BUFFER, CB86, DRC86, etc.). This test never alters inputs.
BACK/REST are probes here: no archive-success claim is made.
"""
from pathlib import Path
import sys, shutil, tempfile, subprocess, os
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'commands/fm/tests'))
from pce import PCE
folder=Path(tempfile.mkdtemp(prefix='fm-finish-'))
source=Path(os.environ['FM_TEST_FIXTURE'])
for name in ('cdos86.img','fat.img','cdos86.cfg'): shutil.copyfile(source/name,folder/name)
shutil.copyfile(os.environ['FM_TEST_HARD'],folder/'hard.img')
with (folder/'cdos86.cfg').open('a') as f:f.write('\ndisk { drive=0x80 type="auto" file="hard.img" optional=0 }\n')
def run(*args):subprocess.run(args,check=True,stdout=subprocess.DEVNULL)
run('mcopy','-o','-i',str(folder/'fat.img'),str(root/'commands/fm/fm.cmd'),'::FMVIEW.CMD')
image=str(folder/'hard.img')+'@@512'
run('mcopy','-o','-i',image,str(root/'commands/chdir/chdir.cmd'),'::CHDIR.CMD')
run('mmd','-i',image,'::FMTEST')
run('mcopy','-o','-i',image,str(root/'commands/fm/fm.cmd'),'::FMTEST/FMVIEW.CMD')
(folder/'CHECK.TXT').write_bytes(b'FM FINAL CHECK\r\n')
run('mcopy','-i',image,str(folder/'CHECK.TXT'),'::FMTEST/CHECK.TXT')
# Harmless launch probe; verifies handoff without running destructive utilities.
run('mcopy','-i',str(folder/'fat.img'),'::STOP.CMD',str(folder/'PROBE.CMD'))
for name in ('BACK','REST','FUNCTION','SETPORT','SETUP','HDMAINT','DSKMAINT'):
 run('mcopy','-o','-i',image,str(folder/'PROBE.CMD'),'::'+name+'.CMD')
print('Fixture',folder,flush=True)
vm=PCE(folder,timeout=180)
def keys(*items):
 for key in items:vm.key(key)
def text(s):
 for c in s:vm.key({' ': 'Space',':':'+Shift Semicolon -Shift','.':'Period','\\':'Backslash','/':'Slash','=':'Equal'}.get(c,c))
def command(s):text(s);keys('Return')
def action(n):
 keys('F9','Tab','Home')
 for i in range(n):keys('Down')
 keys('Return')
try:
 vm.wait('boot',['A>'],timeout=40)
 command('C:')
 command('CD FMTEST')
 command('B:FMVIEW')
 vm.wait('root',['FMTEST','Select a command'])
 keys('F9','Home','Down','Down','Return')
 vm.wait('directory',['CHECK.TXT'])
 keys('Home','Right')
 vm.snapshot('initial',['CHECK.TXT','Select a command'])
 # Setup menu, attribute on focused first file CHECK.TXT.
 action(17)
 vm.snapshot('setup',['Copy to Sys Disk','System Attributes'])
 keys('Down','Return')
 vm.snapshot('attribute',['SYS','Y confirms'])
 keys('Y')
 vm.wait('attribute done',['Action complete'])
 keys('Esc')
 action(2)
 vm.wait('local attributes',['DIR, RW','Y confirms'])
 keys('Y')
 vm.wait('local complete',['Action complete'])
 keys('Esc')
 action(0)
 vm.snapshot('system copy',['Action complete'])
 keys('Esc')
 # Back to main and invoke whole-directory backup.
 keys('Esc')
 action(9)
 vm.snapshot('backup mode',['1/2: selected'])
 keys('3','Return')
 vm.snapshot('backup confirm',['CONTROLC.BR','erased'])
 keys('Y')
 vm.snapshot('backup probe',['return to FM'],timeout=30)
 keys('Space')
 vm.snapshot('backup returned',['Action complete'])
 keys('Esc','Esc')
 command('DIR')
 vm.snapshot('directory restored',['CHECK','FMVIEW'])
 print('PASS: native Setup attributes, Backup publication/launch and mapped FAT return',flush=True)
 vm.close()
 run('mcopy','-i',image,'::CONTROLC.BR',str(folder/'CONTROL.BR'))
 data=(folder/'CONTROL.BR').read_bytes()
 assert b'PATH: C:\\FMTEST\r\n' in data and b'EXC:  D,C,a,k,*.*\r\n' in data
 print('PASS: persisted physical control file contents',flush=True)
except:
 if vm.last:print(vm.text(vm.last.read_bytes()),flush=True)
 vm.abort();raise
