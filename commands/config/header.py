"""Remove the empty EXTRA descriptor emitted before STACK by GENCMD."""
from pathlib import Path
import sys
p = Path(sys.argv[1])
b = bytearray(p.read_bytes())
assert b[0] == 1 and b[9] == 2
assert b[18:27] == bytes(9), "Expected empty EXTRA descriptor"
assert b[27:36] == bytes.fromhex("041000000010000000"), "Unexpected STACK"
assert b[36:45] == bytes(9)
b[18:27] = b[27:36]
b[27:36] = bytes(9)
p.write_bytes(b)
