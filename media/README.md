# Reproducible 320 KB media

Run `make` here, or `make -C media` from the repository root.
Root `make dist` copies the seven distribution images to the root alongside
`y2k-dist.zip` and `disks.zip`.

| Image | Bootable | Contents |
| --- | --- | --- |
| `ccpm86.img` | Yes | PC-MODE/PCTMP kernel and core commands listed below |
| `ccpmnative.img` | Yes | Native TMP kernel and the same core commands |
| `ccpmtools.img` | No | ASM86, DDT86, GENCMD, GENCCPM, CHKCPMFS, WINDOW, WMENU, PRINT, VCMODE, STOP, ERAQ, TOD, PIP; CARDDECK, CLEAR, GLASS and WORDPROC SUB files |
| `ccpmext.img` | No | FM, DREDIX with HELP.EDX and LSN.EDX, PUT, GET, RDR, PIP |
| `ccpmmaint.img` | Yes | PC-MODE/PCTMP kernel, DSKMAINT, DFORM, DCOPY, DCOMP, HDMAINT, INITDIR, CONFIG, FUNCTION, SETUP, SYSTAT, STOP, TOD, PIP |
| `ccpmdev.img` | No | RASM86, LINK86, LIB86, XREF86, SID86, CBAS86, CRUN86, PBASIC, MBASIC.CMD, ASM80, HEXCOM |
| `ccpmdos.img` | No | MSMASM.EXE, MSLIB.EXE, MSLINK.EXE, EXE2BIN.EXE, MBASIC.COM |

Core commands: BATCH, CHSET, DATE, ED, ERA, HELP with HELP.HLP, PCETIME,
PIP, REN, SDIR, SET, SETDEF, SHOW, SUBMIT, SYSDISK and TYPE.
Bootable disks also contain STARTUP.0.

Each image is 327,680 bytes. `PIP.CMD` is PIP4 with the `3.1N` banner.
Empty filesystems are generated on demand with `mk320.py` and `mkfs.cpm`.
The boot sources live in `commands/dskmaint`; media recipes consume their
binaries directly. `media/boottrack.bin` is the generated 4,096-byte reference
used by media checks and CHKCPMFS fixtures. It contains E5 fill, boot code at
000h, the 320K marker at 1FFh and the shared loader at 900h.
Native BASIC is supplied as dev/mbasic.cmd; DOS BASIC as dev/mbasic.com.


Root `make` builds both kernels in one run: `kernel/sysgen/ccpm.sys` (PC-MODE)
and `kernel/sysgen/ccpm-native.sys` (native TMP). The native floppy installs its
kernel as `CCPM.SYS`, as required by the normal boot loader. Both personalities
use the same XIOS implementation and shared kernel fixes. Native XIOS disables
the PC-MODE gateway; this is a source-built alternative, not an archival binary.
Boot either system disk in A: and use the data disks in B:. DOS programs require
the PC-MODE system. Configuration utilities live on maintenance to keep both
system disks within 320 KB.

## Build pipeline

| Step | Operation |
| --- | --- |
| Sources | Build commands, kernel components and all nine RSPs; run GENCCPM |
| Boot | Assemble the boot sector and loader; build and verify the complete 4 KB track |
| Reference check | Require rebuilt SYSDISK and DSKMAINT to match their originals byte-for-byte; run dual-OS DFORM checks |
| Help | Compile `HELP.DAT` into indexed `HELP.HLP` |
| Template | Copy empty `base.img`, then install the source-built boot track |
| Populate | Untwist → cpmtools format `ccpm86-320` → twist back |
| Verify | Extract and compare every packaged file; compare boot tracks with the source-built track and verify sizes |

Recipes are in the Makefiles. `h862bin` from the companion `cpm86-cmdtools`
extracts the assembled boot binaries; `../tools/cpm86twist.py` converts floppy track order.
[Maintained boot sources](../commands/dskmaint/README.md).
Never use cpmtools directly on a physical 320 KB image: head 1 has reversed cylinder order.

## Run and verify

| Command | Action |
| --- | --- |
| `make test` / `make check` | Launch PCE |
| `./ccpm86 vga` | Launch the PCE VGA configuration |
| `make check-media` | Verify all six distributions and their payloads |
| `make check-qemu` | Boot all three system disks; exercise consoles, HELP, SDIR, SYSDISK, SETDEF, DREDIX and windows |

For WINDOW and WMENU, mount `ccpmtools.img` in B: and run `B:WINDOW VIEW`
or `B:WMENU`. DSKMAINT, DFORM, DCOPY, DCOMP, HDMAINT and INITDIR are on the separate bootable maintenance
disk, leaving room for the full HELP on A: and the window tools on B:.

Window layout samples are packaged alongside WINDOW.CMD:

| Sample | Purpose |
| --- | --- |
| `CARDDECK.SUB` | Arrange four overlapping windows |
| `CLEAR.SUB` | Restore all six consoles to full-screen windows |
| `GLASS.SUB` | Arrange mixed-size windows and select console 1 |
| `WORDPROC.SUB` | Set up a large editing window and three small windows |

Select B: before running a sample so its WINDOW commands find WINDOW.CMD.
SUBMIT remains on the main disk:

```text
A>B:
B>A:SUBMIT CARDDECK
B>A:SUBMIT CLEAR
```

For FM, DREDIX, PUT, GET and RDR, mount `ccpmext.img` in B: and run e.g. `B:DREDIX`.
Mount `ccpmdev.img` as a data disk after booting any system disk.
`STARTUP.0` runs `A:TOD`; kernel RTC support supplies startup date/time.
Drive defaults, allocation settings and buffers are in
`../kernel/sysgen/genccpm.in`; patch details are in `../PATCHES.md`.

A manual QEMU session:

```sh
qemu-system-i386 -m 16 -no-fd-bootchk -boot a \
  -drive file=ccpm86.img,format=raw,if=floppy,index=0 \
  -drive file=ccpmtools.img,format=raw,if=floppy,index=1
```

The DRI media byte at offset 511 is `01`, so `-no-fd-bootchk` is required.
Automated QEMU checks use snapshots and leave images unchanged.

Inspect a generated disk:

```sh
python3 ../tools/cpm86twist.py untwist ccpm86.img /tmp/ccpm-logical.img
cpmls -f ccpm86-320 /tmp/ccpm-logical.img
```

After manual cpmtools edits, twist the logical copy back to physical order.
The next `make` replaces generated images.

## Console shortcuts

| Keys | Action |
| --- | --- |
| Ctrl+Alt+top-row 1–4 | Select console 0–3 |
| Ctrl+Alt+Tab | Next console, wrapping from 3 to 0 |
| Ctrl+numeric keypad | Original selection aliases |

All use the normal switch events and honor console ownership/restrictions.
