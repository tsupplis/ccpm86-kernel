# FM — file manager

Four persistent views: **Device, Files, Actions, Help**. `FM` browses CP/M user areas and FAT directories, reads files through Type, copies, renames and deletes selected files, and launches external programs.
FM is MIT licensed; the copied launcher retains its attribution in `LICENSE.md`.

## Runtime compatibility

One unchanged `FM.CMD` runs on CDOS 4.1 and CCP/M-86 3.1 PC consoles,
with either TMP or PCTMP. Both repositories carry identical FM sources and
build rules. No kernel, runtime or other command changes are required.

| Capability | CDOS 4.1 | CCP/M-86 3.1 |
|---|---|---|
| Drives, CP/M users, selection, marks, Type, Copy, Rename/Delete, Print, size/date | Enabled | Enabled |
| DOS filesystem paths, directory browsing and mapping | Enabled | Disabled; CP/M user picker only |
| DREDIX, HELP, native program launch | Enabled | Enabled with TMP or PCTMP |
| DOS executable launch | PCTMP | Requires PCTMP/PC-MODE; unavailable with TMP |
| Backup/Restore | Enabled | Disabled |
| System-file copy/attributes, function-key file loading | Enabled | Enabled |
| Assign keys, serial setup, defaults, memory-disk and scripted boot setup | Existing CDOS protocols | Disabled; incompatible command arguments |
| DCOPY, DFORM, HDMAINT, DSKMAINT, STOP | Launch installed tool | Launch installed tool with its own supported options |

Initialization injects action capabilities. The same policy filters menus and
checks command dispatch, including Repeat and shortcuts. CCP/M never performs
CDOS media-descriptor searches or FAT alias operations; the assembly mapper's
CDOS guard remains intact. The native PC-console gateway and window fields
were checked against both kernels. Serial terminals remain unsupported.

## Current behaviour

| Feature | Real mode |
|---|---|
| Location | Drive Selection opens the shared picker; F9 opens users or directories on the current drive. Enter accepts, Esc cancels |
| Selection | Tab changes pane; arrows/Home/End/PgUp/PgDn navigate; Space/comma/Insert toggle `*` marks; Delete unmarks; F4/Ctrl-X clear. With no marks, the current file uses target colour in Actions, without `*` |
| Type | F2 or Enter in Files; marked files take priority over cursor; F3 repeats |
| Viewer | Space/PgDn/Enter advance; PgUp goes back; Home/End select first/last page; EOF advances to next file; Esc returns with cursor/marks preserved |
| FAT directories | Separate browsable picker; Enter opens a folder; F2 chooses the current directory; F3 selects a drive. Backspace goes to its parent; PgUp/PgDn page through directory names |
| Cancellation | Esc closes overlay without committing a location; main-screen Esc exits |
| Copy | C or Copy menu opens the same drive/user/directory picker; F7/F8 browse saved destinations before accepting. Y confirms replacement; N/Esc cancels; F3 repeats |
| Subset | Actions → Subset of Files accepts an 8.3 mask; `*` restores all files. Accepting a filter clears marks |
| Directory slots | F7/F8 show the saved location and offer Store current / Go / Clear; Esc preserves it. Inside the location picker they open that saved location |
| Size/date | ON/OFF labels name the next action. Details show aligned KiB/MiB sizes and modification dates; unavailable metadata is `--`. Both listings contain files only; cursor and marks are retained |
| Run | F5: selected program with optional arguments; F10: external command line; F3 repeats the action |
| Edit | F6 invokes DREDIX.CMD with the selected filename and optional arguments |
| Help | F1 invokes HELP.CMD with an optional topic |
| Disk utilities | Copy Diskette invokes DCOPY.CMD; Format Diskette invokes DFORM.CMD. Arguments are passed to the utility |
| Limits | 1,024 files per catalog with explicit overflow warning; directory picker pages independently |

FAT browsing uses one private N:/O:/P: alias; Copy can reserve a second for
the destination. Existing drive mappings remain intact.
If no alias is free, navigation reports an error. Paths are limited to 127
characters. Type respects FAT byte lengths and CP/M text EOF.
Subsets, directory slots, Type and Copy use the filesystem adapter.

## Build and checks

```
make                       # Aztec 4.3 FM.CMD
make check                 # native build + all host contract suites
python3 tests/browse.py      # CDOS-specific FAT/CP/M PCE regression
python3 tests/compat.py --cdos /path/to/cdos41-kernel --ccpm /path/to/ccpm86-kernel
```

`compat.py` uses one FM binary across CDOS, CCP/M PCTMP and CCP/M TMP.
It checks rendering, CP/M user selection, native child launch/return, Type,
Copy with extracted-byte verification and exit. CDOS and CCP/M PCTMP also run
a DOS 1 COM probe. `make check` includes 18 host programs and the architecture
check; these cover disabled-action dispatch and rejected FAT calls on CCP/M.

PCE tests use disposable disks and close their own emulator. The native display
releases the screen mutex before input or BDOS I/O. Host tests inject filesystem,
display and job services without an emulator. Host suites and a compiled dependency check cover the contracts,
including metadata conversion and selection while changing display mode.

[design.md](design.md) defines contracts, invariants, modules, keybindings and the
completed framework phases. CP/M/FAT browsing and Type now use the shared
action lifecycle. Copy, Rename/Delete and external execution use the same lifecycle. Original FM functional
parity is not claimed. The shipped colour configuration is supported; the five menu placements are supported within the bounds below. Alternate
OVPMODE and serial terminals remain outside the current implementation.

Backward paging rereads from the start because the filesystem port is sequential.
Rereading and End advance at most one page per job step; Esc interrupts them.
The viewer shows page and selected-file numbers. Empty files and exact page-sized
files do not create an extra blank page. Cleanup errors remain visible.

## Copy safety and scope

Copy snapshots selected files and transfers binary bytes in bounded steps: at
most eight read/write pairs (1 KiB) before returning to input handling.
Confirmation, close and publication remain separate steps. Transfer progress
text stays stable so each block does not trigger a screen redraw.
An existing target requires Y; N/Esc cancels the operation. Completed files stay;
the current temporary is removed where cleanup succeeds. Recovery files are
retained and named when rollback fails. This is not power-loss atomic.

FAT lengths are preserved. CP/M input has record granularity; FAT-to-CP/M output
pads its last record with Ctrl-Z. Attributes and timestamps are not copied.

Verification: eight host suites pass, including binary/short reads, multiple
files, cancellation, injected write/close failures, rollback and retained recovery.
One disposable-disk PCE run verifies browsing/Type, FAT→CP/M, FAT→FAT and CP/M→FAT
Copy, overwrite acceptance/refusal and self-copy rejection. Extracted outputs
match expected bytes and record padding; no FAT temporary files remain.
`FM_TEST_BOOT=/path/to/image` selects a boot fixture independently of media builds.

## Selection readiness versus original FM

| Area | Status |
|---|---|
| Drive, CP/M user and FAT nested/parent selection | Implemented and tested |
| Focus, cursor, paging, marks and multi-file action targets | Implemented and tested |
| 8.3 subsets and two directory slots | Implemented and tested |
| Cancel/return-state preservation | Implemented and tested |
| Size/date display | Implemented and tested |
| Shipped FM.CFG colour assignments | Implemented; current location, then system drive user 0/root |
| Five menu positions, menu size and margins | Implemented with bounded geometry validation |
| Alternate OVPMODE; serial TERM mappings | Not implemented |
| Alphabetical ordering and PC control-key aliases | Implemented; original disassembly used as reference |
| Rename / Delete | Per-file Rename without overwrite; per-file confirmed Delete of selected files |
| Exact original dialogs and selection behaviour | Full equivalence remains unverified |

This is a working navigation/selection package, not a claim of 100% original
FM user-experience equivalence. Copy, selected-file Rename and selected-file Delete modify files; Type inspects
files. The native acceptance run copies a marked pair after filtering and
verifies both output files, as well as recalling a stored FAT location.

## Configuration and PC keys

Real mode reads `FM.CFG` from the current location, falling back to the system drive, user 0
(or its FAT root) when it cannot open that file. It accepts the eight colour
assignments in the reference `tests/fm.cfg`, with original colour numbering, case-insensitive
keywords and significant spacing. CR/LF, Ctrl-Z and an unterminated final line
are accepted. Colours outside 0–15 and oversized lines are ignored. A read or
close failure keeps defaults. Invalid layout or alternate OVPMODE produces a warning.

The Files view sorts names alphabetically before selection. PC control aliases:
Ctrl-A/Home, Ctrl-Z/End, Ctrl-C/Tab, Ctrl-K/Up, Ctrl-J/Down and Ctrl-L/Right.
Backspace opens the FAT directory picker; within it, Backspace selects the parent. FM.CFG does not
configure keys; serial TERM.DAT support is a separate outstanding task.

## Layout settings

`MENU POSITION=1..5` selects right, bottom, left, top or compact-bottom Actions.
`MENU SIZE` sets the vertical menu width (plus its three historical framing
columns); `PROMPT SIZE`, `TOP MARGIN`, `BOTTOM MARGIN`, `LEFT MARGIN` and
`RIGHT MARGIN` set the available regions. Horizontal menus scroll as a grid;
Home/End and arrows keep every command reachable. Invalid geometry leaves the
default layout intact. The current four-view design requires at least 67 columns,
eight object rows and three combined prompt/legend rows. Narrow layouts clip
long labels safely. Automatic expanded-bottom menus and alternate OVPMODE
behaviour are not implemented; exact historical interaction parity is not claimed.

## Size budget

Measured with `aztec43_acc -target=cpm86 -Osize`, standard runtime only (no `-lm`):

| Native production image | Bytes |
|---|---:|
| Code (paragraph-aligned CMD group) | 54,528 |
| Initialized data in CMD | 10,704 |
| Minimum data allocation (including runtime reservations) | 40,992 |
| Code remaining below 64 KiB | 11,008 |

`FM.CMD` is the native build target. CDOS packages it on the administration disk;
this compatibility change does not modify either repository's media rules.

Catalog heap storage is additional: 32 entries initially, doubling to 1,024.
Each native entry plus its view descriptor occupies 21 bytes: 672 bytes initially,
2,688 for a 100-file listing, or 21,504 at capacity. Growth briefly holds old and
new blocks; allocator overhead and stack usage are additional. Thus the static
reduction is not a claim that peak RAM has fallen by the same percentage.

The former DRC build used 58,624 code bytes and 21,264 static data bytes.
Historical component measurements and remaining-action estimates are retained
in [model.md](model.md); they are not Aztec size predictions.

The host suites and combined native navigation, metadata, Type and Copy
regression pass with Aztec. Touched C modules use ANSI definitions and bounded runtime formatting. `xios.asm` and
`fsmap.asm` provide the small-model assembly interfaces; overlays are not used.

## Rename and Delete

Rename prompts for an exact 8.3 replacement name for each selected file. It never
replaces an existing target. Delete accepts the marked set, or the current
file when none are marked, and confirms each file separately. Directories,
wildcards and protected files are refused. N/Esc stops the remaining work;
completed changes are not undone. Dismissing the result refreshes the catalog.

The injected mutation store prepares without writing and rechecks the source
before committing. This is not an atomic transaction across concurrent external
writers; final file locking and operation errors remain the kernel's responsibility.
The selection snapshot remains stable until the sequence ends.

`make check` includes mutation lifecycle tests. Native mutation acceptance:
`FM_TEST_MUTATIONS=1 python3 tests/browse.py` (disposable disks only).

## External programs

All launch actions use `run.c`; the table there contains their only differences.
`runos.c` owns console handoff and location restoration. Local `spawn.c` and
`spawnos.asm` supply the copied util.lib launch implementation; no sibling
repository or util.lib is required to build or run FM.

CMD uses P_CLI; COM/EXE uses the PCTMP DOS launch service. Omitted extensions
follow PCTMP ORDER. BAT dispatches through BATCH. Executable names are 8.3,
optionally drive-qualified; select a FAT directory in FM rather than entering a
full pathname as the executable token. The complete command is limited to 127
characters. Arguments retain their case; the target program's tail policy applies.

The child inherits the selected CP/M user area or mapped FAT directory. FM waits
for console ownership, preserves child output until a keypress, restores drive,
user and both DMA coordinates, and refreshes the catalog after result dismissal.
A successful launch is not a claim that the child returned exit code zero.
Esc cancels before launch; once running, the child owns keyboard interaction.
Edit uses an external DREDIX, not a built-in editor. Disk operations and their
confirmations belong to DCOPY/DFORM; FM adds no separate formatting logic.

`FM_TEST_RUN=1 python3 tests/browse.py` uses one disposable emulator session.
The utility routes use harmless probe programs, so this test cannot format a disk.

Verified: CMD and DOS EXE launch/return, F5/F6/F1 and both disk-action routes,
missing-command errors, nested FAT working directories and CP/M user 4. Persisted
child output was checked after emulator exit. Disk-action routes used probes,
not destructive utilities. `make check` also covers DMA restoration and failures.

## Print and Free Memory

Print uses the current logical printer and the marked files, or the current file
when none are marked. It prints seven-bit text, expands tabs at eight-column
stops, stops at Ctrl-Z/FFh or EOF, and sends a form feed after each opened file.
Esc cancels between records; completed files are unmarked. Errors stop the job;
open files and printer ownership are released on completion or cancellation.
Physical printer waits remain the operating system's responsibility.

Free Memory invokes `STOP.CMD` through the shared launcher, matching the original
FM action. It does not add a separate memory manager.

`make check` covers printer conversion, selection, errors and cancellation.
`FM_TEST_ACTIONS=1 python3 tests/browse.py` checks actual printer bytes and the
STOP launch route in one disposable emulator session.

Backup and Set Up System are implemented. Exact historical dialog parity and
alternate OVPMODE/serial-terminal operation are not claimed.

## Set Up System and Backup

Set Up System replaces the Actions menu; Esc returns to the main menu.
File targets use the same cursor/marks snapshot as Copy. After an action, a file
that still exists retains the cursor instead of jumping to the parent directory.

| Setup action | Implementation |
|---|---|
| Copy to Sys Disk | Transactional Copy to configured system drive, user 0/FAT root; then SYS/RO attributes |
| System / Local Attributes | Confirm each file; set SYS/RO or DIR/RW, preserving other attributes |
| Load Func Keys | FUNCTION.CMD with the selected file |
| Assign Func Keys | FUNCTION.CMD, followed by SETUP.CMD /PARAM |
| Serial Ports | SETPORT.CMD, followed by SETUP.CMD /PARAM |
| System Defaults / Memory Disk | SETUP.CMD with optional arguments / SETUP.CMD /MDISK |
| Boot From H D | DOS or CP/M choice selects HDMAINT's historical function-key sequence |
| Hard / Diskette Maint | HDMAINT.CMD / DSKMAINT.CMD |

Backup accepts `1`/`2` for selected files to A:/B:, `3`/`4` for all files in
the current location, or `R` for REST with an existing control file. Hard disks
only. FM confirms before publishing `CONTROLx.BR` at physical root/user 0 and
launching BACK/REST with `CTL=x`. It preserves the physical FAT working directory.
BACK's destination erasure and REST's possible replacement are stated before launch.
Archive I/O belongs to the external utilities; returning to FM does not establish
that an archive completed successfully. Their own report is authoritative.

Checks cover control-file contents, CP/M/FAT rules, cancellation, stale replies,
write/commit failure, mapping cleanup, all launch policies and attribute flags.
A disposable-disk native run verifies System/Local Attributes, system-disk Copy,
Backup control publication and launch/return from a mapped FAT directory. The
launch probe is deliberately harmless. A separate run of the original BACK
accepted the generated control file and reached its destination-disk prompt.
Native round trips now pass for FAT `C:\FMTEST` and CP/M `D:` user 15:
BACK wrote the archive, the source files were deleted, REST recovered them,
and host extraction matched the originals byte-for-byte. Each case covered
an empty file, 26-byte text, and 513-/8,193-byte binary files. Tests used disposable
disk copies and the original BACK/REST utilities; no FM implementation change
was needed. This verifies single-volume recovery, not multi-volume splitting.

## Navigation display

The active Files or Actions selection has an arrow as well as a colour highlight.
The Device pane shows free KiB, as in the original FM, for CP/M and FAT volumes;
an unavailable measurement displays `Unknown`.

Presentation uses one locked bulk transfer to the virtual console and redraws
only changed frames. It adds no shadow screen buffer. In the same PCE fixture,
six action movements took 23,789,568 cycles instead of 53,346,304 (2.24× faster);
two page-down movements took 8,847,360 instead of 17,891,328 (2.02× faster).
These compare the preceding FMVIEW build, not the historical FM.

## UI acceptance — 13 November 2026

`make check` passes, including directory paging, source/mark preservation during
Copy selection, bookmark clearing, label state and normal-colour arrow gutters.
The combined `tests/uilive.py` PCE pass verified listings, details, location
selection, cancellation, Copy picker and bookmark store/return/clear. It uses
`FM_TEST_FIXTURE` and disposable disk copies. The first attempt used an incorrect
PCE Escape key name; the corrected combined run passed.

Copy names its effective source in the dialog title and status. With no marks,
it uses the current file, shown in target colour when Actions has focus. With marks, `*` identifies
each target and the dialog shows their count. Picker controls sit below the
directory-only list; scroll indicators appear only when another page exists.

## Optimization — 13 November 2026

Copy skips complete frame construction on idle ticks when its progress text is
unchanged; input, decisions, errors and completion still repaint. Catalog names
are validated on publication and remain immutable until refresh; cursor and
layout bounds are checked on every use. Type preserves its last page at EOF
without a second 1,386-byte page buffer. Production definitions and native
adapter declarations use ANSI prototypes, enforced by the architecture check.

Fields paint their text and trailing padding once; bordered panels clear only
interiors, and Help paints each background row once. A 30-frame comparison
across all five layouts (focus, marks, details, dialogs and empty listings)
produced identical characters/attributes with 79,313 rather than 122,795 painted
cells (35.4% fewer view writes by cell count, not an elapsed-time benchmark).
Marks use bounded bitset accessors, saving another 896 bytes of static RAM.
Together these changes reduce static RAM by 2,278 bytes; paragraph-aligned code
is 53,568 bytes, up 560 bytes from the pre-optimization build.

Validation: `make check` passes, including mark-byte boundaries and the final
catalog slot. One combined PCE run passed navigation, exact-page Type EOF,
Copy with byte-for-byte destination verification and exit; its emulator closed.

Copy batching measurement: the same 16,388-byte FAT-to-FAT copy in disposable
PCE disks took 93,323,264 emulated cycles versus 1,017,380,864 before batching
and stable progress text (10.9× fewer cycles). Both outputs matched the source
exactly. This is one emulator workload, not a physical-disk throughput claim.
Host checks cover the eight-pair bound with short reads, cancellation before
publication and existing failure/rollback cases.
