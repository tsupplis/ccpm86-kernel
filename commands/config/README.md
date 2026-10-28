# CONFIG for Concurrent CP/M-86

`config.a86` is the maintained disassembly; `conforg.a86` freezes the
original. Run `make check` to assemble both and compare the complete files
with `../../base/config.cmd`, including headers and padding. No code bytes
are patched during the build.

The CP/M-86 disassembly was used as a reference. This is version 1.1, dated 26 January 1984. It supports interactive menus
and command tails such as `CONFIG P3 1200 7 ODD 2`. Printer 3/4 map to
serial ports 0/1. It has separate CODE, DATA and STACK groups, unlike the
CP/M reference, and rejects interactive use from a user terminal.
`header.py` removes GENCMD's empty EXTRA descriptor; it changes no
allocated segment or executable byte.

Byte equality verifies layout and encodings; it does not exercise hardware.

## Disassembly coverage and encoding

All 641 instructions occupy CODE 0000h..0602h, except the embedded copyright
at 0002h..0046h. CODE 0603h..060Fh is zero padding. DATA and STACK contain
no executable code. No instruction has been replaced with opcode DB bytes.

Option records explicitly separate their length word, four display
characters, CODE callback address and parameter bits. This avoids treating
printable bytes of a callback address as part of the option string.
Long help strings wrap at word boundaries; control bytes remain explicit.

CP/M CONFIG also documents command-tail operation (`CONFIG P0 1200 7 ODD 2`)
in its source header. Concurrent CONFIG uses `P3` and `P4` to refer to serial
ports 0 and 1. This is a command-interface difference, not a drive-number
adjustment. No CP/M source or binary was changed for this reconstruction.
