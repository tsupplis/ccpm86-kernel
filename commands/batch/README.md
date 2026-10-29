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

## Review against the historical BATCH source

The `ossrc/cdos-41-oem/D7/batch.a86` file selects BATCH/CMD mode and
includes `tmpmenu.lib`, `tmpdir.lib` and `tmpmenue.lib`. All four files
assemble successfully with RASM-86 1.4a. That build has CODE 2020h and
DATA 0650h, versus 0FF0h and 0570h in our reference; it is not the same
revision and cannot replace the invariant.

The review confirms the shared command/parameter processing lineage:

| Reconstructed routine | Later source counterpart | Assessment |
| --- | --- | --- |
| `find_pctmp` | `M_INIT` | Both follow parent descriptors to PCTmp; later code copies a command into TMP buffers instead of using our far-service handoff. |
| `read_line`, `read_buffer_char` | batch line reader, `movchar` | Same record-based input, Ctrl-Z handling and percent-parameter expansion. |
| `substitute_parameter` | `subparm` | Same parameter role; retain the shipped revision's scan/copy behavior. |
| `init_batch` | batch parameter initialization | Both collapse argument separators; later source scans three delimiters, not our twelve-byte scan. |
| `pctmp_gateway` callers | TMP command-ready buffer handoff | Different ABI between revisions; do not transplant addresses. |

The later `movchar` explicitly checks the command length against 81h and
sets `long_line` on overflow. That protection is not evidence that the
older binary contains it. The reconstruction preserves its actual behavior.
Both revisions have a 64-byte patch reserve; our maintained copy deliberately
zeroes it while the invariant retains the original NOPs. `make check` still
passes for the full original and the documented maintained difference.

The maintained build suppresses the USER media note and uses
`Press any key to continue...` for PAUSE, matching PCTMP. Six NOPs and a
message-padding byte preserve the original layout. `batorg.a86` is frozen;
`make check` constrains the maintained binary to these patches and the
existing zero-filled patch reserve.

DEL/RENAME aliases are bypassed in the maintained build, matching the
simplified PCTMP dispatch. ERA and REN remain explicit external commands.
The old alias bytes remain unreachable to preserve the original layout;
the invariant and strict expected-byte check include this one JMP patch.
