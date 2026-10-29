#!/usr/bin/env python3
"""Keep the invariant original and constrain every maintained byte change."""
from pathlib import Path
import struct
reference=Path('../../base/batch.cmd').read_bytes()
assert Path('batorg.cmd').read_bytes()==reference, 'Invariant differs from original'
expected=bytearray(reference)
expected[0xc1:0x101]=bytes(64)
code_size=struct.unpack_from('<H',reference,1)[0]*16
data_start=128+code_size
note=b'\r\nNote:  User numbers only apply to CP/M media.'
note_offset=reference.index(note)-128
pattern=b'\xba'+struct.pack('<H',note_offset)+b'\xe8'
code=reference[128:data_start]
assert code.count(pattern)==1, 'USER call is not unique'
a=128+code.index(pattern)
expected[a:a+6]=b'\x90'*6
# Bypass the historical DOS alias block, preserving all label addresses.
def instruction(name):
    off=reference.index(name)-128
    pattern=b'\xbf'+struct.pack('<H',off)+b'\xe8'
    assert code.count(pattern)==1
    return code.index(pattern)
a=instruction(b'DEL     ')
z=instruction(b'REM     ')
expected[128+a:128+a+3]=b'\xe9'+struct.pack('<h',z-a-3)
old=b'Strike a key when ready . . .$'
new=b'Press any key to continue...$\x00'
assert len(old)==len(new) and reference.count(old)==1
p=reference.index(old)
expected[p:p+len(old)]=new
assert Path('batch.cmd').read_bytes()==expected, 'Unexpected maintained BATCH byte change'
print('PASS: frozen original; maintained changes limited to patch reserve, USER note call, DOS alias bypass and PAUSE message')
