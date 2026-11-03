# SET

Recompiled from the CCP/M-86 3.1 source; reference binary: CDOS SET 3.1
`base/set.cmd`. The version string is `CONCURRENT SET 4.1N`.

Shared source: `ccpm86-kernel` and `cdos41-kernel` carry identical copies
(see `check_shared_commands`). The DOS-media code is inert on CCP/M 3.1,
including its PC-MODE build, which has no FAT support: it never presents a DOS pseudo-label, never
writes directory-label byte 15, and its block numbers stay below 8000h, so
the FCB disk-map flag tests cannot match.

## Command line

```
SET filespec [RO|RW,DIR|SYS,ARCHIVE=ON|OFF,F1=ON|OFF,...,F4=ON|OFF]
SET filespec [HIDDEN=ON|OFF]
SET filespec[;password] [PASSWORD=newpassword,PROTECT=READ|WRITE|DELETE|NONE]
SET d: [RO|RW]
SET d: [NAME=label.typ]
SET d: [PROTECT=ON|OFF]
SET d: [PASSWORD=password]
SET d: [DEFAULT=password]
SET d: [CREATE=ON|OFF,ACCESS=ON|OFF,UPDATE=ON|OFF]
SET [HELP]
```

Drive prefixes are optional. Commas separate options; the existing parser
also accepts its historical abbreviated keywords and multiple references.
A bare filespec or drive is not a query: SET requires an option. SHOW and
SDIR provide inspection. Use `[PASSWORD=,PROTECT=OFF]` to remove a label
password and disable drive protection, or file `[PASSWORD=,PROTECT=NONE]`
to remove a file password/protection. The historical parser requires a comma
after the empty password; `[PASSWORD=]` alone is rejected by both builds. Enable drive password protection before
assigning file/label passwords or setting a default password.

READ protects reading, writing, renaming and deletion; WRITE protects
writing, renaming and deletion; DELETE protects renaming and deletion. The
default password is copied to the calling process. Creation and access
stamping are mutually exclusive; stamping needs an INITDIR-formatted
directory.

## DOS media (CDOS)

File attributes work on DOS media through the kernel mapping (CP/M
ARCHIVE=ON clears DOS's archive-needed bit). XFCB passwords and
stamping need CP/M media: SET probes BLACK's DOS pseudo-label (private FCB and
buffer, DMA offset restored) and prints `This command/option requires CP/M
media.` instead of prompting for a password or failing in INITDIR.

## Verification

PCE regressions against the OEM binaries (`base/`) run from the CDOS
root; CCP/M uses `make check` (structural) only.
`make check-set` compares attributes, wildcards, DOS directories,
media guards, labels, password modes and drive RO/RW on CP/M/FAT12 pairs.
OEM SET 3.1 SHA-256:
`e4ec4cdefafe7cd1793c3bc2fbbe5429046e87df8fd53ca720d72c5633616123`.

## FAT case metadata policy (CDOS)

The CDOS kernel treats FAT byte 12 bits 3/4 as lowercase-name and
lowercase-extension flags. SET leaves those bits untouched: only legacy
F1-F3 map to FAT, while F1-F4 remain available on CP/M disks. FAT F4 requests
therefore have no effect. This intentional kernel change supersedes OEM
behavior for the ambiguous FAT F4/case bit; see `../../patches.md` and
`make check-fat-case` at the project root.

## FAT volume names

On actual FAT media, `[NAME=WORK]` creates/changes an eleven-character
volume label; `[NAME="MY DISK"]` permits spaces and `[NAME=]` deletes it.
The parser treats it as a volume string rather than an 8.3 filename.
Only this NAME path uses the FAT BDOS 100 extension (extent zero).
Password/stamping options retain their CP/M-media guard, including when
combined with NAME; rejected combinations do not commit the pending name.
The physical root is used without changing the current subdirectory.
Ordinary files sharing the label's name are protected. CP/M NAME syntax and
all CP/M password/stamp behavior are unchanged.

## FAT HIDDEN attribute

`SET filespec [HIDDEN=ON|OFF]` changes only FAT HIDDEN. It can be combined
with the existing file attributes and applied to wildcard selections.
It first checks actual FAT media, then BDOS 230 capabilities: successful
return, signature `4146h` and HIDDEN support. Missing/mismatched services
are rejected before any changes. CCP/M/CP/M disks reject HIDDEN without
calling 230; existing options retain their behavior. Both projects share
`set.plm` and `scd.a86`; the latter supplies the caller data segment for
BDOS 230's explicit far FCB pointer.

The new service supports GET and masked SET; its complete packet contract
is in the CDOS `api.md`; the CCP/M API table marks 230 unsupported.
BDOS 30 is unchanged. Listing visibility and ERA/RM policy are separate work and are not changed by this option.
