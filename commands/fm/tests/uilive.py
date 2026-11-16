"""One disposable native UI pass. FM_TEST_FIXTURE supplies cdos86.cfg,
cdos86.img, fat.img and hard.img (C: FAT at byte 512, boot A: CP/M).
Only copies are changed; the emulator is closed on success or failure.
"""
from pathlib import Path
import os,sys,shutil,tempfile,subprocess
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tests'))
from pce import PCE
source=Path(os.environ['FM_TEST_FIXTURE'])
folder=Path(tempfile.mkdtemp(prefix='fm-ui-final-'))
for name in ('hard.img','fat.img','cdos86.img','cdos86.cfg'):shutil.copyfile(source/name,folder/name)
subprocess.run(['mcopy','-o','-i',str(folder/'hard.img')+'@@512',str(root/'fm.cmd'),'::FMNEW.CMD'],check=True)
print(folder,flush=True)
with PCE(folder,timeout=120) as vm:
 def command(text):
  for c in text:vm.key({' ': 'Space',':':'+Shift Semicolon -Shift'}.get(c,c))
  vm.key('Return')
 def snap(name,*words):
  frame=vm.snapshot(name,words,timeout=15)
  print('PASS',name,flush=True)
  return frame
 vm.wait('boot',['A>'],timeout=40)
 command('C:');command('FMNEW')
 frame=snap('files','Select a command','Size/Date ON')
 assert 'BACK.CMD' in vm.text(frame)
 assert chr(16)+'BACK.CMD' not in vm.text(frame)
 assert '<DIR>' not in vm.text(frame)
 vm.key('End');vm.key('Up');vm.key('Return')
 frame=snap('details','Size/Date OFF','KiB')
 assert '<DIR>' not in vm.text(frame)
 vm.key('F7');snap('bookmark-empty','Empty bookmark');vm.key('Return')
 vm.key('F7');snap('bookmark-saved','Go to stored location','Clear stored location','C:\\');vm.key('Esc')
 vm.key('F9');snap('directory-picker','Select directory','F2:choose','F3:drives')
 vm.key('Esc');snap('cancel-navigation','Size/Date OFF')
 vm.key('Tab');vm.key('Space')
 frame=snap('marked','*BACK.CMD')
 vm.key('c')
 snap('copy-picker','Copy BACK.CMD - destination','Choose a drive to browse')
 vm.key('Esc');snap('cancel-copy','Size/Date OFF')
 vm.key('F9');vm.key('Backspace');snap('drive-picker','Select drive')
 vm.key('Home');vm.key('Return');snap('users','Select user area')
 vm.key('Return');snap('cpm-location','CP/M Media','A: User 0')
 vm.key('F7');vm.key('Return');snap('bookmark-return','DOS Media','C:\\')
 vm.key('F7');vm.key('Down');vm.key('Return')
 vm.key('F7');snap('bookmark-cleared','Empty bookmark');vm.key('Esc')
 print('PASS combined native UI acceptance',flush=True)
