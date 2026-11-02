# WINDOW and WMENU

Both programs use the same C sources as the CDOS 4.1 tools. BDOS 12 selects
the native public window numbering at startup: CCP/M uses N=0..3 and CDOS
uses N=1..4. Both map to XIOS indices 0..3.

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
screen image and omit trailing spaces. SETUP writes commands for all four
windows.

## WMENU

WMENU registers manager state 1, detaches with BDOS 147 and waits on XIOS
service 17. Ctrl+Alt+0 (top-row zero) or Ctrl+Alt+Esc activates it; Ctrl+keypad-Plus remains available. ESC returns it to
idle; Abort followed by RETURN unregisters it and exits.

The highlighted menu uses arrows or Space to select Place, Size, Scroll,
Tracking, Display, Color, Write or Abort; Return opens the selection.
Digits 0..3 select a window. Esc hides the menu without unloading it.

Geometry uses arrows. Tracking and Display use Up/Down or Space. Color uses
Left/Right to choose foreground/background and Up/Down to change it. Esc or
Return leaves these submenus with changes in effect. Write defaults to
`A:WINDOW0.TXT`, permits a replacement filename and output-type selection,
and writes on Return; Esc cancels. Abort followed by Return unregisters the
manager and exits.

Setting VS_ATTRIB alone colors only later output, so a color change (WMENU
Color or WINDOW CHANGE FG/BG) also repaints the attribute bytes of the
virtual-console image (`RECOLOR` in `xios.a86`, under the VS+52 mutex) and
redraws the window. No kernel change is involved.

## Validation and remaining comparison

Both CMD files compile and link without unresolved symbols. Host checks cover
both BDOS 1431h and 1441h, confirming that the shared command parser presents
the OS-native window numbering while addressing the same XIOS indices.
`make check-wmenu` exercises wake/hide/abort and every menu action on disposable
PCTMP media. `python3 tools/wmenu_check.py ccpmnative.img` repeats the menu
check under native TMP. `make check-window` runs both menu checks and builds
the real-XIOS GLASS regression program.

The default command build produces both programs. The tools disk selects
`commands/window/window.cmd` and `commands/window/wmenu.cmd`, and `make dist`
packages that disk into the release images/archive. The original binaries
remain in `base/` for comparison. Distribution inclusion does not establish
behavioral equivalence; the comparison limitations above still apply.

The compatibility check uses BDOS 12. It requires Concurrent 8086 identity
1400h in the high byte and BDOS revision 31h or newer in the low byte.
CCP/M 3.1 (1431h) selects public numbering 0..3; CDOS 4.1 (1441h) selects
1..4. Ordinary CP/M 0022h, 0031h without Concurrent identity, and revision
1430h are rejected.

`make livetest.cmd` builds the target regression program. The QEMU tools-disk
check installs it only in its temporary fixture and runs it automatically.
It exercises all four commands from GLASS, verifies their live geometry and
colors, tests TOP, and restores all windows and the original top console.
