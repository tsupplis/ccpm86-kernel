# SDIR

Built from the CCP/M-86 3.1 PL/M sources; reference binary: CDOS 4.11
`base/sdir.cmd`.

Shared source: `ccpm86-kernel` and `cdos41-kernel` carry identical copies
(see `check_shared_commands`). The DOS-media code is inert on CCP/M 3.1,
including its PC-MODE build, which has no FAT support: it never presents a DOS pseudo-label, never
writes directory-label byte 15, and its block numbers stay below 8000h, so
the FCB disk-map flag tests cannot match.

## Syntax

`SDIR {filespec ...} [options]`: up to ten specs, one option list, prefix
matching as in the historical parser. `SDIR [HELP]` prints the summary.

| Options | Meaning |
| --- | --- |
| FULL / SIZE / SHORT | Detailed listing (default), names/sizes, names only |
| DATE | Detailed listing with timestamps |
| ATTRIBUTES | Archive and user attribute bits |
| DIR / SYS | Non-system / system files (not subdirectories) |
| RO / RW | Read-only / writable entries |
| XFCB / NONXFCB | With / without password or timestamp metadata |
| SORT / NOSORT | Sort by name (default) / search order |
| EXCLUDE | Invert filename matching |
| DRIVE=d / DISK=d | Drive; `ALL` or `(A,B)` lists |
| USER=n / G<n> | User; `ALL` or `(0,1)` lists |
| MESSAGE | Show searched areas even with no match |
| LENGTH=n / FF | Repeat headers every n (>=5) rows; form feeds |
| PAGE / NOPAGE | Pause after 23 rows / no pauses (default) |
| HELP | Option summary |

SHORT and HELP existed in the source but were disabled; both are now active
on both systems. SHORT reuses the buffered FULL/SIZE scan, so SORT/NOSORT and
the filters apply to it.

## DOS directories (CDOS)

BLACK returns FAT directories as entries whose disk-map bytes 16/17 are
equal with bits C0h set (80h = DOS entry, 40h = subdirectory). SDIR keeps a
`subdir` flag in its file record (extended with separate subdirectory/hidden flags; `file$address` uses
`size(file$info)`), prints `<DIR>` in FULL/SIZE and `d:\name` in SHORT, and
counts directories separately. DOS pathnames are not accepted; use CHDIR.

On CDOS FAT media, volume labels are excluded from file rows and totals by the shared
kernel raw-directory adapter, including labels after ordinary files. SDIR
does not display the DOS volume name as a heading.

## Verification

PCE regressions against the OEM binaries (`base/`) run from the CDOS
root; CCP/M uses `make check` (structural) only.
`make check-sdir` (27 paired CP/M/FAT12 cases) and
`tools/sdir_paths_check.py` (7 path cases). The OEM's `<6` year defect is
normalised.

## SYSTEM / HIDDEN selection (shared source)

Default SDIR and `[DIR]` show normal files only, on CP/M and FAT. This is
an intentional change from the old all-files default on both projects.
`[SYS]` selects CP/M SYSTEM, or FAT SYSTEM/HIDDEN; `[DIR SYS]` shows all.
FULL's Type field distinguishes `Dir`, `Sys`, `Hid`, `S+H`. SHORT/SIZE use
the same filtering but retain their compact formats.

The raw FAT metadata record gates disk-map flag decoding; CP/M allocation
bytes never become HIDDEN. HIDDEN lives in a separate file-info byte, not
in name/type attribute bits. Sorting/indexing uses `size(file$info)`.
No BDOS 230 call, disk write or file attribute mutation is involved.
