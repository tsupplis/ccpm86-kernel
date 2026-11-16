"""Native FM BACK/REST round trips on disposable FAT and CP/M disk copies.

FM_TEST_FIXTURE supplies hard.img, cdos86.img and cdos86.cfg from finish.py.
Optional first argument resumes an existing FAT backup at restoration.
Restored from the saved development driver; navigation uses the current picker.
"""
from pathlib import Path
import os, sys, subprocess, shutil, tempfile, time, hashlib
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'commands/fm/tests'))
from pce import PCE
SOURCE=Path(os.environ['FM_TEST_FIXTURE'])
BASE=Path(tempfile.mkdtemp(prefix='fm-roundtrip-'))
print('RESULTS',BASE,flush=True)
def run(*args):
 return subprocess.run([str(x) for x in args],check=True,cwd=ROOT/'media',stdout=subprocess.PIPE,stderr=subprocess.PIPE).stdout

def setup(kind):
 f=BASE/kind;f.mkdir()
 for name in ('hard.img','cdos86.img','cdos86.cfg'):shutil.copyfile(SOURCE/name,f/name)
 image=str(f/'hard.img')+'@@512'
 for src in (ROOT/'extra/back.cmd',ROOT/'extra/rest.cmd',ROOT/'commands/fm/fm.cmd'):
  run('mcopy','-o','-i',image,src,'::'+src.name.upper())
  subprocess.run(['cpmrm','-f','cdos41-xt-hd',str(f/'hard.img'),'0:'+src.name.upper()],cwd=ROOT/'media',stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  run('cpmcp','-f','cdos41-xt-hd',f/'hard.img',src,'0:'+src.name.upper())
 if kind=='fat':
  run('mformat','-C','-i',f/'fat.img','-t','40','-h','2','-n','8','-c','2','::')
  # Remove old acceptance inputs, keeping only deliberately varied test files.
  run('mdel','-i',image,'::FMTEST/FMVIEW.CMD','::FMTEST/CHECK.TXT')
 else:
  raw=f/'blank.img'
  run('python3',ROOT/'tools/cpm86twist.py','untwist',f/'cdos86.img',raw)
  raw.write_bytes(raw.read_bytes()[:4096]+b'\xe5'*(320*1024-4096))
  run('python3',ROOT/'tools/cpm86twist.py','twist',raw,f/'fat.img')
 payloads={'EMPTY.TST':b'', 'ODD.TST':bytes(range(256))*2+b'\x1a',
           'LARGE.TST':bytes((i*37+11)%256 for i in range(8193)),
           'TEXT.TST':b'First line\r\nSecond line\r\n\x1a'}
 for name,data in payloads.items():
  p=f/name;p.write_bytes(data)
  if kind=='fat':run('mcopy','-o','-i',image,p,'::FMTEST/'+name)
  else:run('cpmcp','-f','cdos41-xt-hd',f/'hard.img',p,'15:'+name)
 # CP/M comparison uses the actual stored record image, including padding.
 expected=f/'expected';expected.mkdir()
 for name in payloads:
  if kind=='fat':run('mcopy','-i',image,'::FMTEST/'+name,expected/name)
  else:run('cpmcp','-f','cdos41-xt-hd',f/'hard.img','15:'+name,expected/name)
 return f,list(payloads)

class Session:
 def __init__(self,f,phase):
  self.f=f;self.phase=phase;self.vm=PCE(f,timeout=180);self.serial=0
 def keys(self,*items):
  frame=None
  for key in items:frame=self.vm.key(key)
  return frame
 def text(self,s):
  for c in s:self.keys({' ': 'Space',':':'+Shift Semicolon -Shift','.':'Period','\\':'Backslash','*':'+Shift 8 -Shift','=':'Equal','/':'Slash'}.get(c,c))
 def cmd(self,s):self.text(s);self.keys('Return')
 def wait(self,label,words,timeout=25):
  frame=self.vm.wait(label,words,timeout=timeout)
  (self.f/(self.phase+'-'+label+'.txt')).write_text(self.vm.text(frame))
  print(self.f.name,self.phase,label,flush=True)
  return frame
 def action(self,n):
  # Location picker preserves Actions focus.
  self.keys('Home')
  for i in range(n):self.keys('Down')
  self.keys('Return')
 def start(self,kind):
  self.wait('boot',['A>'],40)
  self.cmd('PRINTER 1')
  if kind=='fat':self.cmd('C:')
  else:self.cmd('D:');self.cmd('USER 15')
  self.cmd('C:FM')
  self.wait('fm',['Select a command'])
  if kind=='fat':
   self.enter_directory()
  self.wait('source',['LARGE.TST','ODD.TST','TEXT.TST'])
 def enter_directory(self):
  self.keys('F9')
  frame=self.wait('directory-picker',['Select directory'])
  for page in range(256):
   lines=self.vm.text(frame).splitlines()
   matches=[row for row,line in enumerate(lines) if 'FMTEST\\' in line]
   if matches:
    self.keys('Home')
    for unused in range(matches[0]-6):self.keys('Down')
    self.keys('Return')
    self.wait('directory-open',['C:\\FMTEST'])
    self.keys('F2')
    return
   self.keys('PageDown')
   next_frame=self.wait('directory-page',['Select directory'])
   if next_frame==frame:break
   frame=next_frame
  raise AssertionError('FMTEST directory not found in picker')
 def backup(self):
  self.action(9);self.wait('choice',['1/2: selected']);self.keys('4','Return')
  self.wait('confirm',['erased']);self.keys('Y')
  self.wait('insert',['Please insert','drive B']);self.keys('Return')
  self.wait('back-done',['return to FM'],60)
  self.keys('Space');self.wait('fm-return',['Action complete']);self.keys('Esc','Esc')
 def restore(self,kind):
  # Source catalog is empty after clearing: invoke Backup from the same location.
  self.action(9);self.wait('choice',['1/2: selected']);self.keys('R','Return')
  self.wait('confirm',['Run REST']);self.keys('Y')
  self.wait('restore-menu',['Other file(s)']);self.keys('2')
  self.wait('drive',['Enter hard disk drive to restore']);self.keys('C' if kind=='fat' else 'D')
  self.wait('location',['Enter path' if kind=='fat' else 'Enter user'])
  self.text('\\FMTEST' if kind=='fat' else '15');self.keys('Return')
  self.wait('pattern',['Enter file']);self.text('*.TST');self.keys('Return')
  self.wait('date',['Enter restore date']);self.keys('Return')
  deadline=time.monotonic()+30
  while True:
   frame,keyboard=self.vm.frame(deadline);screen=self.vm.text(frame)
   if 'return to FM' in screen:break
   if 'Enter option' in screen and 'Other file(s)' in screen:
    self.keys('Return');break
   if 'Please insert' in screen and 'volume' in screen:
    self.keys('Return');continue
   if 'did not match' in screen:raise AssertionError(screen)
  self.wait('rest-done',['return to FM'],60)
  self.keys('Space');self.wait('fm-return',['Action complete']);self.keys('Esc','Esc')
 def close(self):self.vm.close()
 def fail(self):
  if self.vm.last:print(self.vm.text(self.vm.last.read_bytes()),flush=True)
  self.vm.abort()

def case(kind):
 if len(sys.argv)>1 and kind=='fat':
  f=Path(sys.argv[1]);names=[p.name for p in (f/'expected').iterdir()]
 else:
  f,names=setup(kind);s=Session(f,'backup')
  try:s.start(kind);s.backup();s.close()
  except:s.fail();raise
 image=str(f/'hard.img')+'@@512'
 for name in ([] if len(sys.argv)>1 and kind=='fat' else names):
  if kind=='fat':run('mdel','-i',image,'::FMTEST/'+name)
  else:run('cpmrm','-f','cdos41-xt-hd',f/'hard.img','15:'+name)
 # Fresh folder prevents monitor snapshots from an earlier boot being reused.
 restored=Path(tempfile.mkdtemp(prefix='restore-',dir=f))
 for name in ('hard.img','fat.img','cdos86.img','cdos86.cfg'):shutil.copyfile(f/name,restored/name)
 s=Session(restored,'restore')
 try:
  s.wait('boot',['A>'],40);s.cmd('PRINTER 1')
  s.cmd('C:' if kind=='fat' else 'D:')
  if kind=='cpm':s.cmd('USER 15')
  s.cmd('C:FM');s.wait('fm',['Select a command'])
  if kind=='fat':
   s.enter_directory()
  s.restore(kind);s.close()
 except:s.fail();raise
 for name in names:
  target=restored/name
  if kind=='fat':run('mcopy','-i',str(restored/'hard.img')+'@@512','::FMTEST/'+name,target)
  else:run('cpmcp','-f','cdos41-xt-hd',restored/'hard.img','15:'+name,target)
  assert target.read_bytes()==(f/'expected'/name).read_bytes(),name+' mismatch'
  print('MATCH',kind,name,len(target.read_bytes()),hashlib.sha256(target.read_bytes()).hexdigest(),flush=True)
 print('PASS',kind,'byte-for-byte backup/restore',flush=True)

for kind in ('fat','cpm'):case(kind)
print('PASS: BOTH FILESYSTEM ROUND TRIPS',flush=True)
