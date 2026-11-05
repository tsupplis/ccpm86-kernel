# STOP for CCP/M-86 3.1

Port of the maintained CDOS STOP reconstruction, using standard Concurrent
BDOS 154 (SYSDAT), 157 (P_ABORT) and 143 (termination). Process, memory and
abort-packet layouts match CCP/M-86 3.1; no kernel extension is required.

Run STOP to list processes and free memory. Enter a displayed positive entry
number to request termination, or Return to exit. Entry ** is not selectable.
A nonempty command tail displays the report and exits without prompting.
Protected processes are excluded; the kernel can reject an abort request.
This does not add the CDOS shell's separate STOP name window syntax.

make check reproduces the frozen CDOS reference and compares the maintained
PL/M build against it with synthetic kernel structures and intercepted BDOS.
The reference is retained locally as tests/stop.ref for standalone builds.

make check-live boots disposable CCP/M images, starts PIP on console 0,
selects it through STOP on console 1, verifies its removal and checks that
console 0 returns to a working shell. The test retains diagnostic captures.
Top-level shortcuts are make check-stop and make check-stop-live.

STOP is packaged on ccpmtools.img and ccpmmaint.img. Use B:STOP when
the tools disk is mounted as drive B.
