# BATCH reconstruction

`batch.a86` reconstructs the PC-MODE BATCH executable in `../../base/batch.cmd`
(Digital Research build dated 05/17/84). `batorg.a86` is the invariant copy;
it produces the original 5,632-byte CMD exactly. The maintained `batch.a86`
uses 64 zero bytes at CODE offsets 0041h..0080h in place of the original NOP
patch area. Entry jumps over this area; its size and all later offsets stay
unchanged.

```sh
make -C commands/batch check
```

The check compares whole files: the header, 4,080-byte CODE group, 1,392-byte
DATA group and final record padding. Only the maintained build's 64-byte
zero-filled patch area is allowed to differ. The 200-byte private stack resides in
DATA. ASM86 and GENCMD need no binary post-processing for this executable.
The normal command build includes BATCH; system images install the maintained
build and retain its SYS attribute for system-disk command searches.

The source labels and comments cover:

- PCTmp discovery through parent process descriptors and its far service ABI.
- USER, MEM, 8087, DEL, RENAME, REM, SYS, PAUSE, SUSPEND and PRINTER built-ins.
- Resident/native command lookup and CMD/COM/EXE/BAT extension search.
- Random-record BAT reading, parameter expansion, batch chaining and Ctrl-C.
- Native BDOS wrappers, error tables, overlaid buffers and unused shared helpers.

Executable operations use instruction mnemonics. ASM86 normally forces a
16-bit displacement for CS-relative operands, even `[bx]` or `2[si]`. Four
accesses therefore use a `cs_prefix` CODEMACRO containing only `SEGFIX`,
immediately followed by the normal instruction. This emits the original CS
override without expanding the displacement; it uses no raw opcode bytes.
Keep each prefix adjacent to its instruction. DB declarations contain only
actual strings, initialized storage and padding.

Later Digital Research Concurrent DOS 4.1 `tools41/batch.a86` and its shared
TMP source helped identify the routine roles, alongside this repository's
`kernel/rsp3/pctmp.a86`. The reference binary, rather than that later version,
determines the reconstructed layout and behavior. Original quirks remain,
including unbounded parameter expansion, the 12-byte scan of an 11-byte
separator list, and the PCTmp name comparison's remaining-count test.

This is the Concurrent PC-MODE interpreter, not a standalone CP/M-86 port.
The byte comparison verifies the reconstruction and all original offsets;
it does not constitute an interactive emulator test.
