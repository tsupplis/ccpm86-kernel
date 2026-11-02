"""Discard DR C linker's unused zero records after the declared CMD groups.

Only the simple CODE/DATA format without fixups or extra headers is accepted.
Retain complete 128-byte CP/M records; never discard a declared payload byte.
"""
from pathlib import Path
import sys
p = Path(sys.argv[1])
b = p.read_bytes()
assert b[0] == 1 and b[9] == 2 and not any(b[18:128]), 'Unexpected CMD format'
end = 128 + 16 * (int.from_bytes(b[1:3], 'little') + int.from_bytes(b[10:12], 'little'))
end = (end + 127) & ~127
assert len(b) >= end and not any(b[end:]), 'Nonzero data outside declared groups'
p.write_bytes(b[:end])
