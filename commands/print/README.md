# PRINT

Concurrent CP/M-86 3.1 PRINT, rebuilt from the original Digital Research C
sources and shared with Concurrent DOS 4.1. CDOS `PRINTMGR` is a different
utility; there is no CDOS OEM PRINT binary for comparison.

## Use

```text
PRINT [global-options] filespec[local-options] [,filespec[local-options] ...]
PRINT [TAB8] *.TXT
PRINT REPORT.TXT[GROUP0,TAB4]
```

Options are `TABn`, `SYSTEM`, `DIRECTORY` and `GROUPn`; at most two may appear
in each bracketed group. Local options override global settings. Defaults are
tab width 8, directory files and the current user group. Wildcards are supported.
If the printer is busy, choose Wait or Cancel; Space while printing offers
Stop, Next file or Resume.

Unlike the CP/M-86 resident spooler, this version uses Concurrent printer
services and does not support `PRINT OFF`.

## Build and check

```sh
make -C commands/print
make -C commands/print check
```

`check` validates the CMD structure; the rebuild is not byte-identical to the
CCP/M original. A CDOS smoke run completed a file-printing command; printer
operation on CCP/M still needs testing.
