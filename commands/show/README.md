# SHOW

Recompiled from the CCP/M-86 3.1 source; reference binary: CDOS SHOW 3.3
`base/show.cmd`. Identifies itself as `CONCURRENT SHOW 4.1N`.

Shared source: `ccpm86-kernel` and `cdos41-kernel` carry identical copies
(see `check_shared_commands`). The DOS-media code is inert on CCP/M 3.1,
including its PC-MODE build, which has no FAT support: it never presents a DOS pseudo-label, never
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
- USERS probes the FAT marker and retains its CP/M-media guard. LABEL uses
  the physical-root FAT label name, without CP/M protection/stamp columns.
- On CDOS only (BDOS version 41h), the implicit SPACE listing stops at M:, as
  in SHOW 3.3. CCP/M keeps listing all logged drives.

## Verification

PCE regressions against the OEM binaries (`base/`) run from the CDOS
root; CCP/M uses `make check` (structural) only.
`make check-show` covers options, DOS rejection, drive selection,
user counts, labels/stamping and free space on CP/M and FAT12.

## FAT volume names

SHOW d:LABEL displays the volume name or reports that no label exists,
including when the selected drive is currently in a subdirectory. It reads
the raw 20h metadata record; BDOS 101's synthetic FAT flags are not evidence
that a real label exists. CP/M LABEL output is unchanged. The branch checks
actual FAT media, not the OS version or the presence of PC-MODE.
