#!/usr/bin/env python3
"""Format a disposable FAT12 360K floppy with the actual CCP/M DFORM/XIOS.

Checks physical geometry, complete FAT/root metadata, data fill, an independent
mtools write/read, and the non-system boot sector's cold-boot message.
Only newly created images are modified. Failed tests retain their diagnostics.
"""
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
folder = Path(tempfile.mkdtemp(prefix='dform-live-'))
print('Diagnostics:', folder, flush=True)

def run(*args):
    return subprocess.run(args, cwd=folder, check=True, capture_output=True)

shutil.copyfile(ROOT/'media/diskdefs', folder/'diskdefs')
run('python3', str(ROOT/'tools/cpm86twist.py'), 'untwist',
    str(ROOT/'media/ccpm86base.img'), 'logical.img')
run('cpmcp', '-f', 'ccpm86-320', 'logical.img',
    str(ROOT/'kernel/sysgen/ccpm.sys'), '0:CCPM.SYS')
run('cpmcp', '-f', 'ccpm86-320', 'logical.img',
    str(ROOT/'commands/dskmaint/dform.cmd'), '0:DFORM.CMD')
run('python3', str(ROOT/'tools/cpm86twist.py'), 'twist', 'logical.img', 'source.img')
# Start with eight physical sectors and nonzero old data. Formatting must
# replace the geometry and clear every directory/FAT byte, not merely the IDs.
run('psi', '-n', '320', '-F', '165', '-o', 'target.psi')
cfg = (ROOT/'media/ccpm86.cfg').read_text().replace('driver = "sdl"', 'driver = "null"').replace('sdl:lowpass=5000', 'null')
cfg = cfg.replace('ccpm86.img', 'source.img').replace('ccpmtools.img', 'target.psi')
cfg = re.sub(r'(?m)^path\s*=\s*"([^"\n]+)"',
             lambda m: 'path = "'+str((ROOT/'media'/m[1]).resolve())+'"', cfg)
(folder/'test.cfg').write_text(cfg)
commands = ['c f000000']
keys = {':': '+Shift Semicolon -Shift', ' ': 'Space', '[': 'LeftBracket', ']': 'RightBracket'}
for ch in 'dform b: [dos] [v]':
    commands += ['key '+keys.get(ch, ch), 'c 200000']
commands += ['key Return', 'c 6000000', 'key y', 'c 40000000',
             'save "format.bin" bin b800:0000 fa0', 'q']

def pce(commands, name):
    with (folder/(name+'.log')).open('wb') as log:
        subprocess.run(['pce-ibmpc', '-c', 'test.cfg', '-t', 'null', '-g', 'vga'],
                       cwd=folder, input=('\n'.join(commands)+'\n').encode(),
                       stdout=log, stderr=log, check=True, timeout=240)

def screen(name):
    b = (folder/(name+'.bin')).read_bytes()
    text = '\n'.join(bytes(b[i:i+160:2]).decode('cp437').rstrip() for i in range(0,3840,160))
    (folder/(name+'.txt')).write_text(text)
    return text

pce(commands, 'format')
assert 'Format complete.' in screen('format')
info = run('psi', 'target.psi', '-f').stdout.decode()
assert 'BAD-ID' not in info and '720 + 0' in info, info
run('psi', 'target.psi', '-O', 'raw', '-o', 'target.img')
b = (folder/'target.img').read_bytes()
assert len(b) == 368640
assert b[:512] == (ROOT/'commands/dskmaint/fatboot.bin').read_bytes()
assert struct.unpack_from('<HBHBHHBHHHH', b, 11) == (512,2,1,2,112,720,0xfd,2,9,2,0)
fat = b'\xfd\xff\xff'+bytes(1021)
assert b[512:1536] == b[1536:2560] == fat
assert not any(b[2560:6144]), 'Root directory is not empty'
assert b[6144:9216] == bytes(3072)
assert b[9216:] == bytes([0xf6])*(len(b)-9216), 'Unexpected formatted data fill'
run('mdir', '-i', 'target.img', '::')
probe = b'DFORM FAT12 interoperability check\r\n'
(folder/'probe.txt').write_bytes(probe)
run('mcopy', '-i', 'target.img', 'probe.txt', '::PROBE.TXT')
assert run('mtype', '-i', 'target.img', '::PROBE.TXT').stdout == probe
# Cold boot the formatter's original output, not the copy modified by mtools.
(folder/'test.cfg').write_text(cfg.replace('source.img', 'target.psi'))
pce(['c f000000', 'save "boot.bin" bin b800:0000 fa0', 'q'], 'boot')
assert 'Not a system disk.' in screen('boot')
print('PASS: actual CCP/M DOS format/verify; 360K geometry, FAT12 metadata, mtools file I/O and non-system boot message')
shutil.rmtree(folder)
