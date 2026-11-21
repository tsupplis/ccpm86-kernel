# CCP/M DSKMAINT and dual-OS DFORM

DSKMAINT is reconstructed from this repository's `base/dskmaint.cmd`:
Digital Research version 1.5, dated 26 January 1984. The copied CP/M-86
sources have been replaced here; the other repository retains that version.

## Historical reconstruction

`make check-dsk` builds the maintained formatter and compares the frozen
`dskmorg.a86` reconstruction with the original.
All 6400 bytes must match, including these descriptors and final padding:

| Group | Stored bytes | Minimum bytes | Maximum bytes |
| --- | ---: | ---: | ---: |
| CODE | 2208 | 2208 | unspecified |
| DATA | 3776 | 3776 | unspecified |
| EXTRA | 0 | 8192 | 1048560 |
| STACK | 256 | 256 | unspecified |

ASM86 emits the four groups. GENCMD's `extra[m200 xffff]` supplies the
EXTRA allocation limits in paragraphs, without a post-build header patch.
DATA uses runtime origin 100h; its preceding base page is included by GENCMD.

The frozen build extracts its original boot images from `base/dskmaint.cmd`.
These are reference-only bytes, not a second maintained loader.

Unlike the CP/M edition, DSKMAINT uses native XIOS. It obtains SYSDAT, UDA,
process and console pointers, requires a local foreground unbuffered console,
sets P_KEEP/NOSWITCH, excludes filesystem access with the MXdisk queue and
releases those restrictions on exit. Its stack IOPB carries drive/count,
logical track, sector, DMA segment and offset. XIOS 15 formats, 11 writes,
and 10 reads/checks. Menu geometry comes from the target disk parameter block.

## Shared boot images

The maintained `bootsect.a86` and `loader.a86` are identical to the
CP/M-86 repository. DSKMAINT embeds their generated includes; the separate
`../dform` build and repository `boot` build consume these same binaries.
The loader selects geometry from the media marker and accepts both CPM.SYS
and CCPM.SYS, retaining each system's entry and extent-search conventions.
DSKMAINT copies the generated image lengths rather than historical constants.
Its format menu still supports 160K/320K; larger-format UI work is separate.

`make check` compares the frozen DSKMORG against the original binary.
DFORM's independent build and simulated-I/O regression live in `../dform`.
