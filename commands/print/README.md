# PRINT for Concurrent CP/M-86

PRINT is recompiled from the original Digital Research C sources in
`ossrc/ccpmv31/D6`, using `SQPRINT.C` and the helper modules listed in the
original `SQLINK.INP`. The local C sources, headers, `cpmif.a86` and
`sqlink.inp` are unchanged copies of those originals.

## Build

```sh
make -C commands/print
make -C commands/print clean all
```

The Makefile uses `drccpm_cc`, `pcdev_rasm86` and `drccpm_link`.
It follows the historical submission file: assemble CPMIF, compile SQPRINT
and its helpers, then link with `sqlink[input]`. The input file retains the
original module order and avoids the DOS command-tail length limit.
`MAIN.C` supplies the C entry point; `CPMIF.A86` implements the BDOS bridge
and preserves the full AX/CX results for the utility helpers.

## Use

```text
PRINT [global-options] filespec[local-options] [,filespec[local-options] ...]
PRINT REPORT.TXT
PRINT [TAB8] *.TXT
PRINT REPORT.TXT[GROUP0,TAB4]
```

Options are TABn, SYSTEM, DIRECTORY and GROUPn. Defaults are tab width 8,
directory files and the current user group. Local options override global
settings. The original parser accepts up to two options per bracketed group.
Wildcards are supported. If the printer is occupied, PRINT offers Wait or
Cancel. SPACE during printing offers Stop, Next file or Resume.

## Relationship to CP/M-86 PRINT

The CP/M-86 V1.1 PRINT is a resident assembly spooler which hooks interrupts
and accesses kernel internals. This Concurrent version is a different C
implementation: it acquires/releases the printer through Concurrent CP/M
services and processes files within its own process. It checks for Concurrent
CP/M-86 3.1. It does not implement the older spooler's `PRINT OFF` interface.
The programs share a purpose, but the code does not establish that this is
an incremental revision of the CP/M-86 implementation.

## Validation

The current DRC 1.11 / LINK86 2.02 tools compile and link all modules without
undefined symbols. This is a source rebuild, not a byte-identical reconstruction
of `../../base/print.cmd`: compiler/runtime and linked layout differ. The
rebuilt CMD has 6,480 bytes of allocated CODE and 1,088 bytes of DATA; the
shipped image has 6,752 and 928 respectively. Both request up to 64 KiB DATA.
Printer operation still needs testing under Concurrent CP/M-86.
