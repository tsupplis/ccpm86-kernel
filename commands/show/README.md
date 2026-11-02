# SHOW

Recompiled from the CCP/M-86 3.1 source; reference binary: CDOS SHOW 3.3
`base/show.cmd`. Identifies itself as `CONCURRENT SHOW 4.1N`.

Shared source: `ccpm86-kernel` and `cdos41-kernel` carry identical copies
(see `check_shared_commands`). The DOS-media code is inert on CCP/M 3.1,
which is built without PCMODE: it never presents a DOS pseudo-label, never
writes directory-label byte 15, and its block numbers stay below 8000h, so
the FCB disk-map flag tests cannot match.

## Syntax

`SHOW {d:}{option}`; options may be combined or bracketed.

| Option | Meaning |
| --- | --- |
| SPACE or none | RO/RW status and free space of logged drives |
| DRIVE / DRIVES | BDOS drive characteristics |
| USERS | Current user and file counts per user |
| LABEL | Directory label, protection/stamping flags, label timestamps |
| HELP | Option list |

## Changes from the imported source

- Creation stamping is read from label bit 10h (was an uninitialised local).
- The non-functional OPEN/LOCKED placeholders are removed; any `L` prefix
  selects LABEL.
- USERS and LABEL probe for BLACK's DOS pseudo-label (20h at byte 0, 80h at
  byte 15) and print `This command/option requires CP/M media.` on DOS disks.
- On CDOS only (BDOS version 41h), the implicit SPACE listing stops at M:, as
  in SHOW 3.3. CCP/M keeps listing all logged drives.

## Verification

PCE regressions against the OEM binaries (`base/`) run from the CDOS
root; CCP/M uses `make check` (structural) only.
`make check-show` covers options, DOS rejection, drive selection,
user counts, labels/stamping and free space on CP/M and FAT12.
