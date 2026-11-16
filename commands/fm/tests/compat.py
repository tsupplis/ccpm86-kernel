"""One FM binary on CDOS, CCP/M PCTMP and CCP/M TMP, using disposable disks.

Pass --cdos and --ccpm repository roots containing already-built media. Nothing
outside the temporary fixture is modified. Native child probes deliberately
replace no production tools. The same binary is copied into all three guests.
"""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from pce import PCE

FM = Path(__file__).resolve().parents[1]


def run(args, cwd):
    subprocess.run([str(arg) for arg in args], cwd=cwd, check=True,
                   stdout=subprocess.DEVNULL)


def prepare(root, boot, folder, probe):
    media = root / 'media'
    config_name = 'cdos86.cfg' if boot == 'cdos86.img' else 'ccpm86.cfg'
    config = (media / config_name).read_text()
    config = re.sub(r'(?m)^path\s*=\s*"([^"\n]+)"',
                    lambda m: 'path = "' + str((media / m[1]).resolve()) + '"', config)
    images = re.findall(r'file\s*=\s*"([^"\n]+\.img)"', config)
    for index, name in enumerate(images):
        source = boot if index == 0 else ('cdos86base.img' if boot == 'cdos86.img'
                                          else 'ccpm86base.img')
        shutil.copyfile(media / source, folder / name)
    logical = folder / 'logical.img'
    twist = root / 'tools/cpm86twist.py'
    run(['python3', twist, 'untwist', folder / images[1], logical], media)
    for source, target in ((FM / 'fm.cmd', 'FM.CMD'), (probe, 'PROBE.CMD'),
                           (probe.with_suffix('.com'), 'PROBE.COM'),
                           (folder / 'READTEST.TXT', 'READTEST.TXT')):
        run(['cpmcp', '-f', 'ccpm86-320', logical, source, '0:' + target], media)
    run(['python3', twist, 'twist', logical, folder / images[1]], media)
    (folder / 'cdos86.cfg').write_text(config)


def exercise(folder, restricted):
    with PCE(folder, timeout=150, step=0x100000) as vm:
        def text(value):
            for char in value:
                vm.key({' ': 'Space', ':': '+Shift Semicolon -Shift',
                        '.': 'Period'}.get(char, char))

        def command(value):
            text(value)
            vm.key('Return')

        vm.wait('boot', ['A>'], timeout=45)
        command('B:')
        command('FM')
        frame = vm.snapshot('main', ['Select a command', 'CP/M Media'], timeout=25)
        assert ('Backup File(s)' in vm.text(frame)) == (not restricted)
        # Repeatable read-only location navigation; no DOS directory interface.
        vm.key('F9')
        vm.snapshot('users', ['Select user area'])
        vm.key('Esc')
        # Run a real child through the active command processor and return.
        vm.key('F10')
        vm.wait('launch input', ['External command'])
        command('PROBE.CMD')
        vm.snapshot('child', ['FM CHILD OK', 'Press any key'], timeout=30)
        vm.key('Return')
        vm.wait('child returned', ['Action complete'], timeout=25)
        vm.key('Esc')
        if not restricted or folder.name == 'pctmp':
            vm.key('F10')
            vm.wait('DOS launch input', ['External command'])
            command('PROBE.COM')
            vm.snapshot('dos-child', ['FM CHILD OK', 'Press any key'], timeout=30)
            vm.key('Return')
            vm.wait('DOS child returned', ['Action complete'], timeout=25)
            vm.key('Esc')
        # Limit the file selection to an unmistakable fixture, then Type it.
        vm.key('Tab')
        vm.key('Home')
        vm.key('Tab')
        vm.key('Home')
        vm.key('Down')
        vm.key('Down')
        vm.key('Return')
        vm.wait('subset', ['Subset'])
        vm.key('Home')
        for unused in range(12):
            vm.key('Delete')
        command('READTEST.TXT')
        vm.key('F2')
        vm.snapshot('type', ['FM COMPATIBILITY CONTENT'], timeout=20)
        vm.key('Esc')
        vm.key('C')
        vm.wait('copy destination', ['destination', 'Choose a drive'])
        vm.key('Home')
        vm.key('Down')
        vm.key('Return')
        vm.wait('copy user area', ['Choose a user area'])
        vm.key('Home')
        vm.key('Down')
        vm.key('Return')
        vm.snapshot('copy', ['1 completed', '0 failed'], timeout=25)
        vm.key('Esc')
        vm.key('Esc')
        vm.snapshot('exit' , ['B>'], timeout=20)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cdos', type=Path, required=True)
    parser.add_argument('--ccpm', type=Path, required=True)
    args = parser.parse_args()
    folder = Path(tempfile.mkdtemp(prefix='fm-compat-'))
    print('Fixture:', folder, flush=True)
    print('FM SHA256:', hashlib.sha256((FM / 'fm.cmd').read_bytes()).hexdigest(), flush=True)
    (folder / 'probe.c').write_text('#include <stdio.h>\nint main(void) { puts("FM CHILD OK"); return 0; }\n')
    run(['aztec43_acc', '-target=cpm86', '-Osize', '-o', 'probe.cmd', 'probe.c'], folder)
    # CCP/M PC-MODE implements DOS 1, so do not use a DOS 2 C runtime probe.
    (folder / 'probe.asm').write_text(
        'bits 16\norg 100h\nmov dx,message\nmov ah,9\nint 21h\nint 20h\n'
        "message db 'FM CHILD OK',13,10,'$'\n")
    run(['nasm', '-f', 'bin', '-o', 'probe.com', 'probe.asm'], folder)
    for name, root, image in [('cdos', args.cdos, 'cdos86.img'),
                               ('pctmp', args.ccpm, 'ccpm86.img'),
                               ('tmp', args.ccpm, 'ccpmnative.img')]:
        current = folder / name
        current.mkdir()
        (current / 'READTEST.TXT').write_bytes(b'FM COMPATIBILITY CONTENT\r\n\x1a')
        prepare(root.resolve(), image, current, folder / 'probe.cmd')
        print('RUN:', name, flush=True)
        exercise(current, name != 'cdos')
        logical = current / 'checked.img'
        data_image = 'cdostools.img' if name == 'cdos' else 'ccpmtools.img'
        run(['python3', root / 'tools/cpm86twist.py', 'untwist', current / data_image, logical], root / 'media')
        copied = current / 'copied.txt'
        run(['cpmcp', '-f', 'ccpm86-320', logical, '1:READTEST.TXT', copied], root / 'media')
        expected = (current / 'READTEST.TXT').read_bytes()
        assert copied.read_bytes()[:len(expected)] == expected
        print('PASS:', name, 'console, users, child/return, Type, Copy bytes and exit', flush=True)


if __name__ == '__main__':
    main()
