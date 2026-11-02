# WINDOW and WMENU

Shared source for Concurrent CP/M-86 3.1 and Concurrent DOS 4.1. BDOS 12
selects window numbers at runtime: CCP/M uses **0-3**, CDOS uses **1-4**.
Coordinates are one-based.

## WINDOW

```text
WINDOW VIEW
WINDOW TOP N=1
WINDOW FULL N=1
WINDOW CHANGE N=1 PR=4,PC=5,NR=10,NC=40,FG=WHITE,BG=BLUE
WINDOW WRITE N=1 TYPE=SETUP,FILE=A:WSETUP.SUB
```

`CHANGE` accepts geometry, viewport, tracking, display and color settings.
`WRITE` supports `WINDOW`, `CONSOLE` and `SETUP`; SETUP exports all four
windows. Files use 128-byte CP/M records with Ctrl-Z padding.

## WMENU

Run `WMENU`, then press **Ctrl+Alt+0** (top-row zero) or **Ctrl+Alt+Esc**.
Ctrl+keypad-Plus also works. These shortcuts require the corresponding
kernel/XIOS changes.

Use arrows or Space to navigate, Return to select and digits to switch windows.
Esc leaves a submenu or hides the menu; select Abort and Return to unload it.
Color changes repaint existing text without clearing it. Write defaults to
`WINDOWn.TXT` on the current drive.

## Build and check

```sh
make -C commands/window
make -C commands/window check
make check-window
```

`check` runs host parser and XIOS-dispatch tests. `check-window` runs CCP/M
PCTMP/native TMP menu tests or CDOS OEM/menu/color comparisons on disposable
images. The source rewrite is not byte-identical to the OEM programs.
