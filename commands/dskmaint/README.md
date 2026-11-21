# CCP/M DSKMAINT

The maintained version supports 160K, 320K, 720K, 1200K and 1440K CP/M
floppies through native XIOS, on both PCTMP and TMP. F1 verifies, F3 copies,
F4–F8 select the five formats, and F9 exits. Copy requires matching geometry.
The three-column format menu follows the CP/M-86 edition.

## Reconstruction and addresses

`dskmorg.a86` preserves Digital Research version 1.5 (26 January 1984).
`make check` requires its complete 6,400-byte CMD to match
`../../base/dskmaint.cmd`, including headers, all four groups and padding.
Only the frozen build extracts its historical boot bytes from that reference.

`dskmaint.a86` uses symbolic code/data pointers, menu records and computed
string lengths. Base-page, SYSDAT, UDA, process and console offsets are named
interface constants. Changing embedded boot-image lengths cannot move a menu
away from its pointers. The maintained binary intentionally differs from the
original; the frozen reconstruction does not.

## Native disk operations

Legacy media uses XIOS 15/11/10. Larger formats require the extended capability
response before format confirmation, then use 29/36/35 with explicit geometry
and caller-built CHRN sector IDs. Formatting writes the shared boot code and
loader, then reads every track for controller verification. Copy transfers the
whole disk, including reserved tracks and the final partial allocation track.

Track buffers occupy 16K-aligned slots in loader-allocated EXTRA memory; the
32K minimum guarantees at least one slot without a 64K DMA crossing. DSKMAINT
holds MXdisk, P_KEEP and the local console's NOSWITCH restriction. I/O errors
return to the utility instead of permitting an interactive Ignore. Changed
media is invalidated before releasing MXdisk; BDOS login is reset afterward.

`bootsect.a86` and `loader.a86` are the maintained shared sources. Their generated
includes are embedded here; DFORM and media consume the same boot binaries.
The sources remain identical to the CP/M-86 copies, with independent builds.

## Validation

`make check-live` requires both local kernels already built. It runs the
existing PCE installation against disposable disks under `private/dskmaint-native`:
five formats × two console personalities × format/copy cases. Each copy case
also invokes standalone verification. Checks include menu readiness, automatic
post-format verification, whole-image equality, unchanged source media and
return to the command prompt. Two additional cases check format cancellation
and refusal to copy between unlike formats, with both disks unchanged.
No user disk is used.

For a targeted run: `python3 -B tests/live.py --variants pctmp --formats 720`.
