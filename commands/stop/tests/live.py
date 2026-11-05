"""Exercise STOP reporting and a real cross-console abort on disposable CCP/M media."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
WORK = Path(tempfile.mkdtemp(prefix='ccpm-stop-live-'))
print('Diagnostics:', WORK, flush=True)

def run(*args):
    return subprocess.run(args, cwd=WORK, capture_output=True, check=True)

shutil.copyfile(ROOT/'media/diskdefs', WORK/'diskdefs')
shutil.copyfile(ROOT/'media/ccpm86.img', WORK/'system.img')
run('python3', str(ROOT/'tools/cpm86twist.py'), 'untwist',
    str(ROOT/'media/ccpmtools.img'), 'tools.logical.img')
run('cpmcp', '-f', 'ccpm86-320', 'tools.logical.img',
    str(ROOT/'commands/stop/stop.cmd'), '0:STOP.CMD')
run('python3', str(ROOT/'tools/cpm86twist.py'), 'twist',
    'tools.logical.img', 'tools.img')
config = (ROOT/'media/ccpm86.cfg').read_text()
config = re.sub(r'(?m)^path\s*=\s*"([^"\n]+)"',
                lambda m: 'path = "'+str((ROOT/'media'/m[1]).resolve())+'"', config)
config = config.replace('driver = "sdl"', 'driver = "null"')
config = config.replace('sdl:lowpass=5000', 'null').replace('sdl:lowpass=0', 'null')
config = config.replace('"ccpm86.img"', '"system.img"').replace('"ccpmtools.img"', '"tools.img"')
(WORK/'test.cfg').write_text(config)
commands = ['c f000000']
keys = {' ': 'Space', ':': '+Shift Semicolon -Shift', '.': 'Period'}
def enter(text):
    for ch in text:
        commands.extend(['key '+keys.get(ch, ch), 'c 200000'])
    commands.extend(['key Return', 'c 5000000'])
def capture(name):
    commands.append('save "'+name+'.bin" bin b800:0000 fa0')

enter('pip')
commands.extend(['key +Ctrl +Alt 2 -Alt -Ctrl', 'c 2000000'])
enter('b:stop.cmd')
capture('before')
enter('1')
enter('b:stop.cmd')
capture('after')
enter('')
commands.extend(['key +Ctrl +Alt 1 -Alt -Ctrl', 'c 2000000'])
enter('dir')
capture('shell')
commands.append('q')
with (WORK/'pce.log').open('wb') as log:
    subprocess.run(['pce-ibmpc', '-c', 'test.cfg', '-t', 'null', '-g', 'vga'],
                   cwd=WORK, input=('\n'.join(commands)+'\n').encode(),
                   stdout=log, stderr=log, timeout=180, check=True)
def screen(name):
    b = (WORK/(name+'.bin')).read_bytes()
    s = '\n'.join(bytes(b[i:i+160:2]).decode('cp437').rstrip() for i in range(0,3840,160))
    (WORK/(name+'.txt')).write_text(s)
    return s
before, after, shell = (screen(n) for n in ('before','after','shell'))
assert re.search(r'^\s*1\.\s+PIP\s+', before, re.M), before
assert 'Enter the Number' in before, before
# Ignore the previous report left above the latest report on the screen.
latest = after.rsplit('Entry   Program   Memory   Window', 1)[-1]
assert not re.search(r'^\s*\d+\.\s+PIP\s+', latest, re.M), after
assert 'Directory' in shell or 'CCPM' in shell, shell
assert 'A>' in shell, shell
print('PASS: CCP/M STOP report, real PIP abort, refreshed report and recovered shell')
