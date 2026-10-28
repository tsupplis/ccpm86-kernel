# SETUP for Concurrent CP/M-86

`setup.a86` is the maintained disassembly; `setuporg.a86` freezes the
original. Run `make check` to assemble both and compare the complete files
with `../../base/setup.cmd`, including headers and padding. No code bytes
are patched during the build.

The CP/M-86 disassembly was used as a reference. The Concurrent version retains the memory-disk, serial/function-key saving,
floppy timing, verification and storage-selection routines. It checks the
Concurrent OS and console context and uses the Concurrent XIOS interfaces.
CP/M startup-command, FIDDS and musical-error menus are absent. The
configuration sector begins at 0908h with signature DDB2h.

Byte equality verifies layout and encodings; it does not exercise hardware.

## Disassembly coverage and encoding

The executable region 0100h..0907h contains 767 instructions. Its only
non-instruction ranges are the copyright (0102h..0125h), console error
(0176h..01ACh), OS error (01D8h..0202h), and menu dispatch tables at
0150h, 0224h, 027Eh, 03EAh, 043Eh and 04B5h. All other DB storage is the
loader page or data beginning at 0908h. There are no opcode-DB exceptions.

The image shares CODE and DATA. Absolute symbolic aliases deliberately
prevent ASM86 from inserting CS overrides for code-resident variables.
The complete-file comparison protects those original addresses. Interior
banner addresses retain aliases without breaking the displayed sentence
into artificial string fragments. Menu text is grouped on logical lines;
control bytes and zero terminators remain explicit.

Usage: `SETUP` opens the function-key menus; it has no command-tail parser.
CP/M reference menus for startup commands, FIDDS and musical error messages
are not present in this Concurrent image. Both references remain separate.
