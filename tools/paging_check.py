#!/usr/bin/env python3
"""Exercise optional paging in the built commands and resident DIR in QEMU.

All fixtures and disk writes are confined to temporary image copies.
"""
import json
from pathlib import Path
import select
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent
MEDIA = ROOT / 'media'
PAUSE = 'Press any key to continue...'


def run(*args):
    return subprocess.run(args, cwd=MEDIA, check=True, capture_output=True)


def check():
    with tempfile.TemporaryDirectory(prefix='ccpm-paging-') as td:
        folder = Path(td)
        logical, boot, target = (folder / name for name in ('logical.img', 'boot.img', 'target.img'))
        twist = str(ROOT / 'tools/cpm86twist.py')
        run('python3', twist, 'untwist', str(MEDIA / 'ccpm86.img'), str(logical))
        run('cpmrm', '-f', 'ccpm86-320', str(logical), '0:help.hlp')
        run('cpmcp', '-f', 'ccpm86-320', str(logical), str(ROOT / 'commands/dir/dir.cmd'), '0:')
        run('python3', twist, 'twist', str(logical), str(boot))
        run('python3', twist, 'untwist', str(MEDIA / 'ccpm86base.img'), str(logical))
        data = bytearray(logical.read_bytes())
        data[4096:6144] = b'\xe5' * 2048
        logical.write_bytes(data)
        empty = folder / 'empty'
        empty.write_bytes(b'\x1a')
        for number in range(60):
            run('cpmcp', '-f', 'ccpm86-320', str(logical), str(empty), f'0:f{number:03}.txt')
        for name, content in [
            ('long.txt', b''.join(f'page line {n:02}\r\n'.encode() for n in range(1, 51)) + b'\x1a'),
            ('short.txt', b'short file\r\nsecond line\r\n\x1a'),
            ('exact.txt', b'exact page\r\n' * 23 + b'\x1a'),
            ('wrap.txt', b'wrapped text ' * 160 + b'\r\nwrap end\r\n\x1a')]:
            source = folder / name
            source.write_bytes(content)
            run('cpmcp', '-f', 'ccpm86-320', str(logical), str(source), '0:' + name)
        run('python3', twist, 'twist', str(logical), str(target))
        video = folder / 'video.bin'
        with (folder / 'qemu.log').open('w') as log:
            args = ['qemu-system-i386', '-machine', 'pc', '-accel', 'tcg', '-m', '16',
                    '-display', 'none', '-serial', 'none', '-monitor', 'none', '-qmp', 'stdio',
                    '-net', 'none', '-boot', 'a', '-no-fd-bootchk', '-no-reboot',
                    '-rtc', 'base=2026-10-19T12:34:56,clock=vm']
            for index, image in enumerate((boot, target)):
                args += ['-drive', f'file={image},format=raw,if=floppy,index={index}']
            process = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                       stderr=log, bufsize=0)

            def receive():
                assert select.select([process.stdout], [], [], 10)[0], 'QMP timeout'
                return json.loads(process.stdout.readline())

            def command(name, arguments=None):
                process.stdin.write((json.dumps({'execute': name, 'arguments': arguments or {}}) + '\n').encode())
                while True:
                    reply = receive()
                    if 'error' in reply:
                        raise RuntimeError(reply['error'])
                    if 'return' in reply:
                        return reply['return']

            def hmp(text):
                return command('human-monitor-command', {'command-line': text})

            def screen():
                assert not hmp(f'pmemsave 0xb8000 4000 "{video}"')
                data = video.read_bytes()
                return '\n'.join(''.join(chr(c) if 32 <= c < 127 else ' '
                                         for c in data[row * 160:row * 160 + 160:2])
                                 for row in range(24))

            def prompt(text):
                lines = [line.strip() for line in text.splitlines() if line.strip()]
                return bool(lines) and lines[-1] == 'A>'

            def paused(text):
                lines = [line.rstrip() for line in text.splitlines() if line.strip()]
                return bool(lines) and lines[-1].endswith(PAUSE)

            def wait(predicate):
                end = time.monotonic() + 25
                while time.monotonic() < end:
                    text = screen()
                    if predicate(text):
                        return text
                    assert 'Interrupt from' not in text and 'Disk Error' not in text, text
                    time.sleep(.1)
                raise AssertionError(screen())

            def key(name):
                assert not hmp('sendkey ' + name + ' 50')
                time.sleep(.12)

            def type_command(text):
                keys = {'\n': 'ret', ':': 'shift-semicolon', ' ': 'spc', '.': 'dot',
                        '[': 'bracket_left', ']': 'bracket_right'}
                for char in text:
                    key(keys.get(char, char))

            def pages(text, expected=None, abort=False):
                type_command(text + '\n')
                count = 0
                while True:
                    display = wait(lambda s: prompt(s) or paused(s))
                    if prompt(display):
                        break
                    count += 1
                    assert count <= 6, (text, 'too many pauses', display)
                    # No further page may appear before a key is supplied.
                    time.sleep(.15)
                    assert screen() == display, (text, 'output did not pause')
                    key('ctrl-c' if abort else 'spc')
                    if abort:
                        display = wait(prompt)
                        break
                if expected is not None:
                    assert count == expected, (text, count, expected, display)
                else:
                    assert count > 0, (text, 'did not page', display)
                assert 'Invalid' not in display and 'not found' not in display.lower(), (text, display)
                return display

            try:
                receive()
                command('qmp_capabilities')
                wait(prompt)
                # Compare the utility message with the actual resident shell PAUSE.
                type_command('pause\n')
                wait(paused)
                key('spc')
                wait(prompt)
                pages('type b:long.txt [page]', 2)
                pages('type b:short.txt [page]', 0)
                pages('type b:exact.txt [page]', 0)
                pages('type b:wrap.txt [page]')
                pages('type b:long.txt [page]', 1, abort=True)
                pages('type b:long.txt', 0)
                pages('type b:long.txt [p10]', 4)
                pages('type b:long.txt [p23]', 2)
                pages('type b:short.txt [p01]', 1)
                pages('type b:short.txt [p02]', 0)
                pages('type b:long.txt [p10]', 1, abort=True)
                pages('sdir b: [page]')
                pages('sdir b: [page]', 1, abort=True)
                pages('sdir b: [page nopage]', 0)
                pages('sdir b:', 0)
                pages('dir b: b: [p]')
                pages('dir b: b: [p]', 1, abort=True)
                pages('dir b: b:', 0)  # resident state must reset after abort
                pages('dir.cmd b: b: [p]')
                pages('dir.cmd b: b: [p]', 1, abort=True)
                pages('dir.cmd b: b:', 0)
                for option in ('p', 'p10', '[p]', '[p00]', '[p24]', '[p99]', '[p1]', '[paa]'):
                    type_command('cls\n')
                    wait(prompt)
                    type_command('type b:short.txt ' + option + '\n')
                    display = wait(prompt)
                    assert 'Invalid option' in display, (option, display)
                print('PASS: native PAUSE text; TYPE/SDIR/DIR.RSP/DIR.CMD paging, resume, Ctrl-C, default-off and state reset; TYPE PAGE/P01..P23, short/exact/wrapped pages and rejection of old/out-of-range syntax')
            finally:
                try:
                    command('quit')
                except (BrokenPipeError, RuntimeError, AssertionError):
                    process.kill()
                process.wait(timeout=5)


if __name__ == '__main__':
    check()
