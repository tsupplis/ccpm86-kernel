# WINDOW and WMENU: C rewrite trial

Both programs are built in this directory. This is a new C implementation
based on the original commands' visible interfaces and the Concurrent 3.1
XIOS source, rather than a byte-identical reconstruction.

```sh
make -C commands/window
make -C commands/window check
```

The first command builds `window.cmd` and `wmenu.cmd` with Digital Research
C 1.11, RASM-86 and LINK86. The second runs the portable parser/service tests
with the host C compiler. Compiler and linker output is checked because the
historical tools can return success after reporting errors.

## Modules

| File | Role |
| --- | --- |
| `window.c` | WINDOW command-tail entry point |
| `wmenu.c` | Detached manager, hotkey wait, interactive editing and status line |
| `command.c` | Command/field abbreviations, value parsing and validation |
| `win.c` | Shared window state, XIOS operations, display and file output |
| `win.h` | Shared declarations and coordinate conventions |
| `xios.a86` | Small C ABI bridge for far XIOS calls, far-memory access and BDOS |
| `format.c` | Small-model formatter for decimal/string output; avoids general printf |
| `start.c` | DRC entry and normal process termination |
| `livetest.c` | Real-XIOS GLASS layout/TOP regression; restores original state |
| `hostcheck.c` | Tests against a simulated XIOS data block and service gateway |

The assembly bridge uses the same system:0C03h gateway as the shipped
utilities, obtaining the system segment with BDOS 154 and the process XIOS
context through BDOS 156. Function-16 data pointers are relative to the
system data segment returned by BDOS 154, not the process XIOS context. See `kernel/con/xbegin.lib`, `xchar.lib` and
`xconout.lib` for the register conventions, manager lifecycle and VS fields.
The fixed ABI is intentionally confined to the shared support layer.

## WINDOW

```text
WINDOW VIEW
WINDOW TOP N=0
WINDOW FULL N=1
WINDOW CHANGE N=1 PR=4,PC=5,NR=10,NC=40,VR=1,VC=1,TR=ROW,D=COLOR,FG=WHITE,BG=BLUE
WINDOW WRITE N=1 TYPE=WINDOW,FILE=A:WINDOW1.TXT
WINDOW WRITE N=1 TYPE=CONSOLE,FILE=A:CONSOLE1.TXT
WINDOW WRITE N=1 TYPE=SETUP,FILE=A:WSETUP.SUB
```

Public coordinates are one-based; the shared layer translates to zero-based
XIOS coordinates. CHANGE is parsed and validated before applying it. WRITE
creates/replaces its destination and writes CP/M 128-byte records with Ctrl-Z
padding. Window/console snapshots copy character bytes from the virtual
screen image; SETUP output is a WINDOW CHANGE command.

## WMENU

WMENU registers manager state 1, detaches with BDOS 147 and waits on XIOS
service 17. The existing XIOS manager hotkey activates it. ESC returns it to
idle; Abort followed by RETURN unregisters it and exits.

The trial UI explicitly maps P to Place, S to Size, R to Scroll, T to
Tracking, D to Display, C to Color and W to Write. Digits select a console.
Arrow keys edit geometry, RETURN accepts, and ESC restores saved settings.
Color uses digits 0..7, with TAB switching foreground/background. Write uses
TAB to select Window, Console or Setup output.

## Validation and remaining comparison

Both CMD files compile and link without unresolved symbols. A QEMU smoke
test on Concurrent 3.1 verifies all four WINDOW VIEW rows, CHANGE, the full
GLASS layout/colors/TOP sequence and WMENU installation/detachment.
The tools-disk smoke test uses a temporary bootable copy, leaving the released
data-only disk unchanged. Both reject an
ordinary CP/M environment under emu2 with the corresponding Concurrent-only
message. Host checks cover coordinate conversion, CHANGE fields, numerical
validation, invalid commands, required WRITE arguments and TOP/FULL dispatch.

These are initial trial binaries, not validated drop-in replacements. Live
window edits, resident wake/sleep behavior, video output, screenshot
file contents and detailed error/abbreviation compatibility still require
comparison with the original programs. WMENU's explicit key map is a new UI;
its equivalence to the original has not been established. The host tests
simulate the documented ABI and do not execute the real XIOS or assembly
bridge. In particular, screen clipping/stride and terminal-specific key
behavior must be checked on the target before promoting this rewrite.

The default command build produces both programs. The tools disk selects
`commands/window/window.cmd` and `commands/window/wmenu.cmd`, and `make dist`
packages that disk into the release images/archive. The original binaries
remain in `base/` for comparison. Distribution inclusion does not establish
behavioral equivalence; the comparison limitations above still apply.

The compatibility check uses BDOS 12. It requires Concurrent 8086 identity
1400h in the high byte and BDOS revision 31h or newer in the low byte.
Host regression cases include the real 1431h return, ordinary CP/M 0022h,
0031h without Concurrent identity, and the unsupported 1430h revision.

`make livetest.cmd` builds the target regression program. The QEMU tools-disk
check installs it only in its temporary fixture and runs it automatically.
It exercises all four commands from GLASS, verifies their live geometry and
colors, tests TOP, and restores all windows and the original top console.
