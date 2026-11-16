"""Native Rename/Delete acceptance on the combined harness's disposable disks."""
import subprocess


def run(vm):
    def keys(*items):
        for key in items:
            vm.key(key)

    def text(value):
        for ch in value:
            vm.key({'.': 'Period', ' ': 'Space', '?': '+Shift Slash -Shift'}.get(ch, ch))

    def action(index):
        keys('F9', 'Tab', 'Home')
        for unused in range(index):
            vm.key('Down')
        keys('Return')

    def subset(pattern):
        # Called with file focus after a job or initial F9.
        action(2)
        keys('Home')
        for unused in range(12):
            vm.key('Delete')
        text(pattern)
        keys('Return', 'F9', 'Home', 'Right')

    keys('F', 'M', 'V', 'I', 'E', 'W', 'Return')
    vm.wait('root', ['ITEM000.TXT', 'DOCS'])
    keys('F9')
    subset('ITEM00?.TXT')
    keys('Space', 'Right', 'Space')
    action(7)
    for old, new in (('ITEM000.TXT', 'RENAMED.TXT'), ('ITEM001.TXT', 'SECOND.TXT')):
        vm.wait('rename input', ['Action input', old])
        text(new)
        keys('Return')
        vm.wait('rename confirmation', ['Rename', 'Y confirms'])
        keys('Y')
    vm.wait('rename pair complete', ['Action complete', '2 completed'])
    keys('Esc')
    subset('RENAMED.TXT')
    action(8)
    vm.wait('delete confirmation', ['Delete RENAMED.TXT permanently?'])
    keys('N')
    vm.wait('cancelled', ['Action cancelled'])
    keys('Esc')
    vm.wait('retained', ['RENAMED.TXT'])
    keys('F9', 'Home', 'Right')
    action(8)
    vm.wait('delete confirmation', ['Delete RENAMED.TXT permanently?'])
    keys('Y')
    vm.wait('delete complete', ['Action complete', '1 completed'])
    keys('Esc', 'Esc')
    vm.wait('shell', ['B>'])
    print('PASS: native FAT two-file Rename, Delete cancellation and confirmed Delete', flush=True)


def verify(folder):
    names = subprocess.check_output(['mdir', '-b', '-i', str(folder / 'fat.img'), '::']).decode().upper()
    assert 'ITEM000.TXT' not in names and 'RENAMED.TXT' not in names
    assert 'ITEM001.TXT' not in names and 'SECOND.TXT' in names
    assert 'ITEM002.TXT' in names
    print('PASS: persisted mutation results; unrelated files retained', flush=True)
