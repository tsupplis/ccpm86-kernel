#!/usr/bin/env python3
"""Drive HDMAINT creation through PCE/QEMU on disposable mixed-partition images.

This is an interactive-formatter test: HDMAINT, not the host, creates CP/M.
DOS entries are synthetic boundary guards, not bootable DOS installations.
Fixtures and screen dumps are retained for manual review/replay.
"""
from pathlib import Path
import argparse, hashlib, json, re, select, shutil, struct, subprocess, tempfile, time

P = argparse.ArgumentParser(description=__doc__)
P.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
P.add_argument('--cmd', type=Path)
P.add_argument('--emulator', choices=['pce','qemu'], required=True)
P.add_argument('--personality', choices=['pc','native'], default='pc')
P.add_argument('--expect-hang', choices=['options'])
A = P.parse_args()
R = A.root.resolve()
CMD = (A.cmd or R/'commands/hdmaint/hdmaint.cmd').resolve()
F = Path(tempfile.mkdtemp(prefix='hdmaint-ui-'))
print('Fixture:', F, flush=True)

def run(*args):
    return subprocess.run(args, cwd=F, check=True, capture_output=True)

def digest_range(path, start, end):
    h = hashlib.sha256()
    with path.open('rb') as f:
        f.seek(start)
        while start < end:
            block = f.read(min(1048576, end-start))
            assert block
            h.update(block)
            start += len(block)
    return h.hexdigest()

# Preserve exact inputs in the fixture, independent of repository rebuilds.
shutil.copyfile(CMD,F/'hdmaint.cmd')
kernel = R/'kernel/sysgen'/('ccpm-native.sys' if A.personality=='native' else 'ccpm.sys')
shutil.copyfile(kernel,F/'ccpm.sys')
shutil.copyfile(R/'media/diskdefs', F/'diskdefs')
run('python3',str(R/'tools/cpm86twist.py'),'untwist',str(R/'media/ccpm86base.img'),'boot.logical.img')
run('cpmcp','-f','ccpm86-320','boot.logical.img','ccpm.sys','hdmaint.cmd','0:')
run('python3',str(R/'tools/cpm86twist.py'),'twist','boot.logical.img','boot.img')
spc=16*63
size=1024*spc*512
hard=F/'hard.img'
with hard.open('wb') as f: f.truncate(size)
mbr=bytearray(512)
# Identifiable bootstrap; it is intentionally replaced by the existing default.
mbr[:446]=bytes([0x5a])*446
for index,kind,start,length in [(0,6,63,65*spc-63),(1,5,65*spc,65*spc)]:
    at=446+index*16
    mbr[at]=0x80 if index==0 else 0
    mbr[at+4]=kind
    for pos,lba in [(at+1,start),(at+5,start+length-1)]:
        c,rem=divmod(lba,spc);h,s=divmod(rem,63)
        mbr[pos:pos+3]=bytes([h,s+1|((c>>8)<<6),c&255])
    struct.pack_into('<II',mbr,at+8,start,length)
mbr[510:]=b'\x55\xaa'
with hard.open('r+b') as f:
    f.write(mbr)
    for lba in [63,65*spc,130*spc-1,196*spc,size//512-1]:
        f.seek(lba*512);f.write(bytes([0xa5])*512)
start=130*spc*512; end=196*spc*512
before=(digest_range(hard,512,start),digest_range(hard,end,size))
(F/'baseline-mbr.bin').write_bytes(mbr)
(F/'manifest.json').write_text(json.dumps(dict(emulator=A.emulator,personality=A.personality,
    geometry=[1024,16,63],hdmaint_sha256=hashlib.sha256(CMD.read_bytes()).hexdigest(),
    kernel_sha256=hashlib.sha256(kernel.read_bytes()).hexdigest(),protected_hashes=before),indent=2)+'\n')

class Qemu:
    def __init__(self):
        self.log=(F/'qemu.log').open('wb')
        args=['qemu-system-i386','-machine','pc','-accel','tcg','-m','16',
              '-display','none','-serial','none','-monitor','none','-qmp','stdio',
              '-net','none','-boot','a','-no-fd-bootchk','-no-reboot',
              '-drive','file=boot.img,format=raw,if=floppy,index=0',
              '-drive','file=hard.img,format=raw,if=none,id=hd0',
              '-device','ide-hd,drive=hd0,cyls=1024,heads=16,secs=63,bios-chs-trans=none']
        (F/'qemu-command.json').write_text(json.dumps(args,indent=2)+'\n')
        self.p=subprocess.Popen(args,cwd=F,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.log,bufsize=0)
        self.receive();self.command('qmp_capabilities')
    def receive(self):
        assert select.select([self.p.stdout],[],[],10)[0], 'QMP timeout'
        return json.loads(self.p.stdout.readline())
    def command(self,name,args=None):
        self.p.stdin.write((json.dumps({'execute':name,'arguments':args or {}})+'\n').encode())
        while True:
            r=self.receive()
            assert 'error' not in r,r
            if 'return' in r:return r['return']
    def hmp(self,s):return self.command('human-monitor-command',{'command-line':s})
    def screen(self,path):
        self.hmp(f'pmemsave 0xb8000 4000 "{path}"')
        return path.read_bytes()
    def key(self,key):
        self.hmp('sendkey '+{'Return':'ret','Space':'spc'}.get(key,key.lower())+' 50')
        time.sleep(.12)
    def close(self):
        if self.p.poll() is None:self.command('quit')
        self.p.wait(timeout=10);self.log.close()

class Pce:
    def __init__(self):
        cfg=(R/'media/ccpm86.cfg').read_text().replace('driver = "sdl"','driver = "null"').replace('sdl:lowpass=5000','null')
        cfg=re.sub(r'(?m)^path\s*=\s*"([^"\n]+)"',lambda m:'path = "'+str((R/'media'/m[1]).resolve())+'"',cfg)
        cfg=re.sub(r'floppy_disk_drives\s*=\s*\d+','floppy_disk_drives = 2',cfg)
        cfg=re.sub(r'disk\s*\{.*?\}','',cfg,flags=re.S)
        cfg+='\ndisk { drive=0 type="auto" file="boot.img" }\n'
        cfg+='disk { drive=128 type="image" file="hard.img" c=1024 h=16 s=63 }\n'
        (F/'pce.cfg').write_text(cfg)
        self.log=(F/'pce.log').open('wb')
        self.p=subprocess.Popen(['pce-ibmpc','-c','pce.cfg','-t','null','-g','vga'],cwd=F,stdin=subprocess.PIPE,stdout=self.log,stderr=self.log)
    def send(self,s):self.p.stdin.write((s+'\n').encode());self.p.stdin.flush()
    def screen(self,path):
        self.send(f'c 2000000\nsave "{path}" bin b800:0000 fa0')
        until=time.monotonic()+30
        while not path.exists() or path.stat().st_size!=4000:
            assert self.p.poll() is None,'PCE exited'
            assert time.monotonic()<until,'PCE monitor timeout'
            time.sleep(.05)
        return path.read_bytes()
    def key(self,key):self.send('key '+key+'\nc 200000')
    def close(self):
        if self.p.poll() is None:self.send('q')
        self.p.wait(timeout=10);self.log.close()

vm = Qemu() if A.emulator=='qemu' else Pce()
seq=0
last=''
def screen():
    global seq,last
    seq+=1
    p=F/f'screen-{seq:04}.bin'
    data=vm.screen(p)
    last='\n'.join(bytes(data[i:i+160:2]).decode('cp437') for i in range(0,4000,160))
    p.with_suffix('.txt').write_text(last)
    return last

def wait(text,name,seconds=60):
    until=time.monotonic()+seconds
    while time.monotonic()<until:
        if text in screen():
            (F/(name+'.txt')).write_text(last)
            print('SCREEN:',name,flush=True)
            return
        time.sleep(.15)
    raise AssertionError(f'{name}: missing {text!r}\n{last}')

def key(name):vm.key(name)

success=False
try:
    wait('A>','boot')
    for ch in 'hdmaint':key(ch)
    key('Return')
    wait('SELECT FUNCTION','main')
    key('F1');wait('Change or create CP/M partition','partition')
    key('F6');wait('Enter starting cylinder','start')
    assert '130' in last,last
    key('F10');wait('Enter partition size','size')
    key('F10')
    if A.expect_hang=='options':
        try:wait('Accept CP/M options shown','options',15)
        except AssertionError:
            assert 'reading the data back.' in last,last
            print('PASS: reproduced options-screen hang',flush=True);success=True
        else:raise AssertionError('Broken binary unexpectedly passed options')
    else:
        wait('Accept CP/M options shown','options')
        # Exercise an option redraw before accepting the final settings.
        key('F4');wait('Accept CP/M options shown','options-redraw')
        key('F10')
        wait('Creation of CP/M volume complete','complete',180)
        key('F9');wait('Please insert a system diskette','reboot')
        success=True
finally:
    vm.close()

if not A.expect_hang:
    with hard.open('rb') as f:
        after=f.read(512);f.seek(start+1536);label=f.read(512)
    assert after[446:478]==mbr[446:478], 'Foreign partition entries changed'
    assert after[510:]==b'\x55\xaa'
    entries=[after[i:i+16] for i in range(446,510,16)]
    cpm=[e for e in entries if e[4]==0xdb]
    assert len(cpm)==1 and struct.unpack_from('<II',cpm[0],8)==(130*spc,66*spc)
    assert label[43]==6 and struct.unpack_from('<H',label,46)[0]==0xfbf, 'Unexpected 8 KiB DPB'
    assert sum(struct.unpack('<256H',label))&65535==0,'Invalid label checksum'
    assert before==(digest_range(hard,512,start),digest_range(hard,end,size)), 'Out-of-partition write'
    raw=CMD.read_bytes();n=int.from_bytes(raw[1:3],'little')*16;data=raw[128+n:]
    assert after[:446]==data[0xefe:0xefe+446], 'Default MBR replacement changed'
    print('PASS: guest-created 66-cylinder volume at 130; DPB/checksum/MBR and protected regions verified',flush=True)
assert success
print(f'PASS: {A.emulator} {A.personality}; fixture retained at {F}',flush=True)
