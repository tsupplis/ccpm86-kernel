# CALLCOMPAT — CCP/M-86 3.1 commands

2026-11-05. Source/API review of all 59 executable artifacts in the distribution
recipe. This is not a cross-version runtime certification.

**3.1**: native CCP/M baseline, including the original IBM PC XIOS and PC-MODE
where required. **Degraded**: works without enhanced behaviour.
**Specific**: no working fallback without the required extension.
DOS markers give the minimum DOS API level; optional higher-version paths do
not raise that minimum. Preserved native binaries are classified as 3.1.

**Adaptive**: deliberately detects the OS version or available services and selects
a supported path; unavailable optional features do not prevent normal use.

| Command | Status | Comment |
| --- | --- | --- |
| ASM80.CMD <!-- commands/asm80/asm80.cmd --> | 3.1 |  |
| ASM86.CMD <!-- commands/asm86/asm86.cmd --> | Adaptive | 3.1 baseline. Version-gated BDOS 108 exit-status reporting; normal operation does not require it. |
| BATCH.CMD <!-- commands/batch/batch.cmd --> | 3.1 |  |
| CBAS86.CMD <!-- dev/cbas86.cmd --> | 3.1 |  |
| CHKCPMFS.CMD <!-- commands/chkcpmfs/chkcpmfs.cmd --> | 3.1 |  |
| CHSET.CMD <!-- commands/chset/chset.cmd --> | 3.1 |  |
| CONFIG.CMD <!-- commands/config/config.cmd --> | 3.1 |  |
| CRUN86.CMD <!-- dev/crun86.cmd --> | 3.1 |  |
| DATE.CMD <!-- commands/date/date.cmd --> | 3.1 |  |
| DCOMP.CMD <!-- commands/disk/dcomp.cmd --> | Specific | Vanilla CP/M-86 1.1, specific CCP/M-86 3.1 and specific CDOS 4.1 only. |
| DCOPY.CMD <!-- commands/disk/dcopy.cmd --> | Specific | Vanilla CP/M-86 1.1, specific CCP/M-86 3.1 and specific CDOS 4.1 only. |
| DDT86.CMD <!-- commands/ddt86/ddt86.cmd --> | 3.1 |  |
| DFORM.CMD <!-- commands/dskmaint/dform.cmd --> | Specific | Vanilla CP/M-86 1.1 and specific CCP/M-86 3.1 only. |
| DREDIX.CMD <!-- commands/dredix/dredix.cmd --> | 3.1 |  |
| DSKMAINT.CMD <!-- commands/dskmaint/dskmaint.cmd --> | 3.1 |  |
| ED.CMD <!-- commands/ed/ed.cmd --> | Adaptive |  |
| ERA.CMD <!-- commands/era/era.cmd --> | 3.1 |  |
| ERAQ.CMD <!-- commands/era/eraq.cmd --> | 3.1 |  |
| EXE2BIN.EXE <!-- dev/exe2bin.exe --> | DOS 1.0 |  |
| FUNCTION.CMD <!-- commands/function/function.cmd --> | 3.1 |  |
| GENCCPM.CMD <!-- commands/genccpm/genccpm.cmd --> | 3.1 |  |
| GENCMD.CMD <!-- commands/gencmd/gencmd.cmd --> | Adaptive | 3.1 baseline. Version-gated BDOS 108 exit-status reporting; normal operation does not require it. |
| GENTAB.CMD <!-- commands/ddt86/gentab.cmd --> | 3.1 |  |
| GET.CMD <!-- commands/putget/get.cmd --> | 3.1 |  |
| HDMAINT.CMD <!-- commands/hdmaint/hdmaint.cmd --> | 3.1 |  |
| HELP.CMD <!-- commands/help/help.cmd --> | 3.1 |  |
| HEXCOM.CMD <!-- commands/hexcom/hexcom.cmd --> | 3.1 |  |
| INITDIR.CMD <!-- commands/initdir/initdir.cmd --> | 3.1 |  |
| LIB86.CMD <!-- dev/lib86.cmd --> | 3.1 |  |
| LINK86.CMD <!-- dev/link86.cmd --> | 3.1 |  |
| MBASIC.CMD <!-- dev/mbasic.cmd --> | 3.1 |  |
| MBASIC.COM <!-- dev/mbasic.com --> | DOS 1.0 |  |
| MSLIB.EXE <!-- dev/mslib.exe --> | DOS 1.0 |  |
| MSLINK.EXE <!-- dev/mslink.exe --> | DOS 1.0 |  |
| MSMASM.EXE <!-- dev/msmasm.exe --> | DOS 1.0 |  |
| PBASIC.CMD <!-- dev/pbasic.cmd --> | 3.1 |  |
| PCETIME.CMD <!-- base/pcetime.cmd --> | 3.1 |  |
| PIP.CMD <!-- commands/pip/pip4.cmd --> | Adaptive |  |
| PRINT.CMD <!-- commands/print/print.cmd --> | 3.1 |  |
| PUT.CMD <!-- commands/putget/put.cmd --> | 3.1 |  |
| RASM86.CMD <!-- dev/rasm86.cmd --> | 3.1 |  |
| RDR.CMD <!-- commands/putget/rdr.cmd --> | 3.1 |  |
| REN.CMD <!-- commands/ren/ren.cmd --> | 3.1 |  |
| SDIR.CMD <!-- commands/sdir/sdir.cmd --> | 3.1 |  |
| SET.CMD <!-- commands/set/set.cmd --> | Degraded | 3.1 baseline. FAT HIDDEN is unavailable without BDOS 230; FAT NAME is unavailable without FAT label-write support. Cross-version behaviour awaits regression. |
| SETDEF.CMD <!-- commands/setdef/setdef.cmd --> | Specific | 3.1 baseline. BDOS 240 packet version 3 missing: command-search settings cannot be queried or changed. |
| SETUP.CMD <!-- commands/setup/setup.cmd --> | 3.1 |  |
| SHOW.CMD <!-- commands/show/show.cmd --> | Degraded | 3.1 baseline. FAT label reporting is unavailable without FAT support and can miss existing labels with older enumeration. Cross-version behaviour awaits regression. |
| SID86.CMD <!-- dev/sid86.cmd --> | 3.1 |  |
| STOP.CMD <!-- commands/stop/stop.cmd --> | 3.1 |  |
| SUBMIT.CMD <!-- commands/submit/submit.cmd --> | 3.1 |  |
| SYSDISK.CMD <!-- commands/sysdisk/sysdisk.cmd --> | 3.1 |  |
| SYSTAT.CMD <!-- commands/systat/systat.cmd --> | 3.1 |  |
| TOD.CMD <!-- commands/tod/tod.cmd --> | 3.1 |  |
| TYPE.CMD <!-- commands/type/type.cmd --> | 3.1 |  |
| VCMODE.CMD <!-- commands/vcmode/vcmode.cmd --> | 3.1 |  |
| WINDOW.CMD <!-- commands/window/window.cmd --> | 3.1 |  |
| WMENU.CMD <!-- commands/window/wmenu.cmd --> | 3.1 |  |
| XREF86.CMD <!-- dev/xref86.cmd --> | 3.1 |  |
