"""Native DSKMAINT integration checks on disposable PCE disks under private/."""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import time
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
from cpm86twist import convert

FORMATS = {160: (40, 1, 8, 'F4'), 320: (40, 2, 8, 'F5'),
           720: (80, 2, 9, 'F6'), 1200: (80, 2, 15, 'F7'),
           1440: (80, 2, 18, 'F8')}


def run(folder, *args):
    subprocess.run(args, cwd=folder, check=True, capture_output=True)


class Machine:
    """Monitor input and video assertions; never use a fixed boot delay."""
    def __init__(self, folder):
        self.folder = folder
        self.sequence = 0
        self.log = (folder / 'pce.log').open('wb')
        self.process = subprocess.Popen(
            ['pce-ibmpc', '-c', 'test.cfg', '-t', 'null', '-g', 'vga'],
            cwd=folder, stdin=subprocess.PIPE, stdout=self.log, stderr=self.log)

    def send(self, command):
        self.process.stdin.write((command + '\n').encode())
        self.process.stdin.flush()

    def key(self, key):
        self.send('key ' + key)
        self.send('c 200000')

    def command(self, command):
        for char in command:
            self.key({' ': 'Space', ':': '+Shift Semicolon -Shift'}.get(char, char))
        self.key('Return')

    def wait(self, expected, cycles=1000000):
        deadline = time.monotonic() + 90
        screen = ''
        while time.monotonic() < deadline:
            self.sequence += 1
            path = self.folder / 'screen.bin'
            path.unlink(missing_ok=True)
            self.send(f'c {cycles:x}\nsave "screen.bin" bin b800:0 fa0')
            capture = time.monotonic() + 10
            while not path.exists() or path.stat().st_size != 4000:
                assert self.process.poll() is None, 'PCE exited before capture'
                assert time.monotonic() < capture, 'PCE capture timeout'
                time.sleep(.02)
            data = path.read_bytes()
            screen = '\n'.join(data[i:i+160:2].decode('cp437').rstrip()
                               for i in range(0, 4000, 160))
            (self.folder / 'screen.txt').write_text(screen)
            assert 'BAD DISK' not in screen and 'Unexpected Interrupt' not in screen, screen
            if expected in screen:
                return screen
        raise AssertionError(f'timeout waiting for {expected!r}\n{screen}')

    def close(self):
        if self.process.poll() is None:
            self.send('q')
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        self.log.close()


def fixture(folder, variant, size, copy=False):
    folder.mkdir(parents=True, exist_ok=True)
    boot = folder / 'logical.img'
    (folder / 'diskdefs').write_bytes((ROOT / 'media/diskdefs').read_bytes())
    run(folder, sys.executable, str(ROOT / 'tools/mk320.py'), str(boot),
        '--format', 'ccpm86-320', '--boot', str(ROOT / 'commands/dskmaint/bootsect.bin'),
        '--loader', str(ROOT / 'commands/dskmaint/loader.bin'))
    fmt = 'ccpm86-320'
    if copy and size != 320:
        fmt = 'ibmpc-514ss' if size == 160 else f'cpm86-{size}'
        boot.write_bytes(bytes([0xe5]) * (size * 1024))
        run(folder, 'mkfs.cpm', '-f', fmt, str(boot))
        data = bytearray(boot.read_bytes())
        code = (ROOT / 'commands/dskmaint/bootsect.bin').read_bytes()
        loader = (ROOT / 'commands/dskmaint/loader.bin').read_bytes()
        data[:len(code)] = code
        data[0x900:0x900+len(loader)] = loader
        data[511] = 0 if size == 160 else size//10
        if size >= 720:
            data[510] = 1
        boot.write_bytes(data)
    kernel = 'ccpm-native.sys' if variant == 'tmp' else 'ccpm.sys'
    run(folder, 'cpmcp', '-T', 'raw', '-f', fmt, str(boot),
        str(ROOT / 'kernel/sysgen' / kernel), '0:CCPM.SYS')
    run(folder, 'cpmcp', '-T', 'raw', '-f', fmt, str(boot),
        str(ROOT / 'commands/dskmaint/dskmaint.cmd'), '0:DSKMAINT.CMD')
    image = boot.read_bytes()
    if not copy or size == 320:
        image = convert(image, False)
    (folder / 'boot.img').write_bytes(image)
    (folder / 'target.img').write_bytes(bytes([0xa5]) * (size * 1024))
    if copy:
        data = bytearray((folder / 'target.img').read_bytes())
        data[:512] = image[:512]
        (folder / 'target.img').write_bytes(data)
    cfg = (ROOT / 'media/ccpm86.cfg').read_text()
    cfg = re.sub(r'(?m)^path\s*=.*', f'path = "{ROOT / "media/rom"}"', cfg)
    cfg = re.sub(r'disk\s*\{.*?\}', '', cfg, flags=re.S)
    cfg = cfg.replace('driver = "sdl"', 'driver = "null"').replace('sdl:lowpass=5000', 'null')
    cfg += '\ndisk { drive=0 type="auto" file="boot.img" }\ndisk { drive=1 type="auto" file="target.img" }\n'
    (folder / 'test.cfg').write_text(cfg)


def check(variant, size, copy=False):
    parent = ROOT / 'private/dskmaint-native'
    parent.mkdir(parents=True, exist_ok=True)
    folder = Path(tempfile.mkdtemp(prefix=f'{variant}-{size}-'+('copy-' if copy else 'format-'), dir=parent))
    fixture(folder, variant, size, copy)
    before_boot = (folder / 'boot.img').read_bytes()
    machine = Machine(folder)
    try:
        machine.wait('A>')
        machine.command('dskmaint')
        menu = machine.wait('Exit this Program')
        for value in FORMATS:
            assert f'Format {value}K' in menu, menu
        (folder / 'menu.bin').write_bytes((folder / 'screen.bin').read_bytes())
        if copy:
            machine.key('F1')
            machine.wait('Exit to main menu')
            machine.key('F2')  # Verify the boot/source disk, including its last track.
            machine.wait('Exit to main menu', cycles=4000000)
            machine.key('F10')
            machine.wait('Exit this Program')
            machine.key('F3')
            machine.wait('Exit to main menu')
            machine.key('F2')
            machine.wait('SELECT DESTINATION DISKETTE')
            machine.wait('Exit to main menu')
            machine.key('F4')
        else:
            machine.key(FORMATS[size][3])
            machine.wait('Exit to main menu')
            machine.key('F4')
        machine.wait('Is this what you want to do')
        machine.key('y')
        machine.wait('Exit to main menu', cycles=4000000)
        machine.key('F10')
        machine.wait('Exit this Program')
        machine.key('F9')
        machine.wait('A>')
    finally:
        machine.close()
    expected = bytearray([0xe5]) * (size * 1024)
    boot = (ROOT / 'commands/dskmaint/bootsect.bin').read_bytes()
    loader = (ROOT / 'commands/dskmaint/loader.bin').read_bytes()
    expected[:len(boot)] = boot
    expected[511] = FORMATS[size][1]-1 if size < 720 else size//10
    if size >= 720:
        expected[510] = 1
    expected[0x900:0x900+len(loader)] = loader
    if copy:
        expected = before_boot
    assert (folder / 'target.img').read_bytes() == expected, f'{variant}/{size}: disk contents'
    assert (folder / 'boot.img').read_bytes() == before_boot, 'system disk changed'
    action = 'copy and standalone verify' if copy else 'format and boot data'
    print(f'PASS: {variant} {size}K {action}; exact images and cleanup', flush=True)


def safety(variant):
    """Declining a format and refusing unlike copy media must write nothing."""
    parent = ROOT / 'private/dskmaint-native'
    folder = Path(tempfile.mkdtemp(prefix=f'{variant}-safety-', dir=parent))
    fixture(folder, variant, 720)
    target = bytearray((folder / 'target.img').read_bytes())
    target[510:512] = bytes([1, 72])
    (folder / 'target.img').write_bytes(target)
    source = (folder / 'boot.img').read_bytes()
    machine = Machine(folder)
    try:
        machine.wait('A>')
        machine.command('dskmaint')
        machine.wait('Exit this Program')
        machine.key('F6')
        machine.wait('Exit to main menu')
        machine.key('F4')
        machine.wait('Is this what you want to do')
        machine.key('n')
        machine.wait('Exit this Program')
        machine.key('F3')
        machine.wait('Exit to main menu')
        machine.key('F2')
        machine.wait('SELECT DESTINATION DISKETTE')
        machine.wait('Exit to main menu')
        machine.key('F4')
        machine.wait('Press any key to return to Menu')
        assert 'not of the same type' in (folder / 'screen.txt').read_text()
        machine.key('Space')
        machine.wait('Exit to main menu')
        machine.key('F10')
        machine.wait('Exit this Program')
        machine.key('F9')
        machine.wait('A>')
    finally:
        machine.close()
    assert (folder / 'boot.img').read_bytes() == source
    assert (folder / 'target.img').read_bytes() == target
    print(f'PASS: {variant} format cancellation and unlike-media refusal; images unchanged', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--variants', nargs='+', choices=['pctmp', 'tmp'], default=['pctmp', 'tmp'])
    parser.add_argument('--formats', nargs='+', type=int, choices=FORMATS, default=list(FORMATS))
    parser.add_argument('--actions', nargs='+', choices=['format', 'copy', 'safety'], default=['format', 'copy', 'safety'])
    args = parser.parse_args()
    for variant in args.variants:
        for size in args.formats:
            for action in args.actions:
                if action != 'safety':
                    check(variant, size, action == 'copy')
        if 'safety' in args.actions:
            safety(variant)


if __name__ == '__main__':
    main()
