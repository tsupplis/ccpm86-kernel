#!/usr/bin/env python3
"""Exercise WMENU wake shortcuts on a disposable PCE disk pair."""

from pathlib import Path
import tempfile, subprocess, shutil, sys

R = Path(__file__).resolve().parent.parent
cdos = (R / "media/cdos86.cfg").exists()
stem = "cdos86" if cdos else "ccpm86"
# CCP/M: pass ccpmnative.img to boot native TMP instead of PCTMP.
boot = sys.argv[1] if len(sys.argv) > 1 else stem + ".img"
F = Path(tempfile.mkdtemp(prefix="wmenu-key-"))
print("Diagnostics:", F, flush=True)


def run(*args):
    return subprocess.run(args, cwd=F, check=True, capture_output=True)


shutil.copyfile(R / "media" / boot, F / (stem + ".img"))
if cdos:
    run(
        "mformat",
        "-C",
        "-i",
        "fat.img",
        "-t",
        "40",
        "-h",
        "2",
        "-n",
        "8",
        "-c",
        "2",
        "::",
    )
    run("mcopy", "-i", "fat.img", str(R / "commands/window/wmenu.cmd"), "::wmenu.cmd")
else:
    # CCP/M reads its CP/M-format tools floppy, not a FAT fixture.
    shutil.copyfile(R / "media/ccpmtools.img", F / "fat.img")
cfg = (
    (R / "media" / (stem + ".cfg"))
    .read_text()
    .replace('driver = "sdl"', 'driver = "null"')
    .replace('driver      = "sdl:lowpass=5000"', 'driver      = "null"')
    .replace(
        'file     = "' + ("cdostools" if cdos else "ccpmtools") + '.img"',
        'file     = "fat.img"',
    )
)
(F / (stem + ".cfg")).write_text(cfg)
c = ["c f000000"]


def key(k):
    c.extend(["key " + k, "c 200000"])


def save(name):
    c.extend(["c 4000000", f'save "{name}.bin" bin b800:0000 fa0'])


def typed(s):
    for x in s:
        key(
            "+Shift Semicolon -Shift" if x == ":"
            else "Period" if x == "."
            else "Space" if x == " "
            else x
        )
    key("Return")


hotkeys = [
    ("esc", "+Ctrl +Alt Esc -Alt -Ctrl"),
    ("keypad", "+Ctrl KP_Plus -Ctrl"),
    ("zero", "+Ctrl +Alt 0 -Alt -Ctrl"),
]
for name, chord in hotkeys:
    key(chord)  # Without a manager these must not leak into console input.
save("uninstalled")
for x in "b:wmenu":
    key("+Shift Semicolon -Shift" if x == ":" else "Period" if x == "." else x)
key("Return")
save("installed")
for name, chord in hotkeys:
    key(chord)
    save("wake-" + name)
    key("Esc")
    save("idle-" + name)
key(hotkeys[-1][1])
menu_choice = 0


def choose_menu_item(target):
    global menu_choice
    for _ in range((target - menu_choice) % 8):
        key("Space")
    key("Return")
    menu_choice = target


choose_menu_item(7)
key("Return")
save("exited")
for name, chord in hotkeys:
    key(chord)
save("unregistered")
key("0")
save("plain-zero")
if not cdos:
    # Exercise the original highlighted-menu flow on CCP/M.
    key("Return")
    typed("b:wmenu")
    save("reinstalled")
    wake = hotkeys[-1][1]
    key(wake)
    menu_choice = 0
    choose_menu_item(5)
    for k in ["Down", "Down", "Down", "Right", "Down", "Return"]:
        key(k)
    save("colored")
    key("Esc")
    typed("dir")
    save("typed")
    key(wake)
    choose_menu_item(1)
    for k in ["Up", "Left", "Return"]:
        key(k)
    choose_menu_item(0)
    for k in ["Down", "Right", "Return"]:
        key(k)
    save("placed")
    choose_menu_item(2)
    for k in ["Down", "Right"]:
        key(k)
    save("scrolled")
    for k in ["Esc"]:
        key(k)
    choose_menu_item(3)
    key("Space")
    key("Esc")
    choose_menu_item(4)
    key("Space")
    key("Space")
    key("Return")
    choose_menu_item(6)
    key("Return")
    save("written")
    key("Esc")
    typed("b:window view")
    save("view")
c.append("q")
with (F / "pce.log").open("wb") as log:
    subprocess.run(
        ["pce-ibmpc", "-c", stem + ".cfg", "-t", "null", "-g", "vga"],
        cwd=F,
        input=("\n".join(c) + "\n").encode(),
        stdout=log,
        stderr=log,
        timeout=240,
        check=True,
    )
assert "unknown key:" not in (F / "pce.log").read_text(errors="replace")


def text(name):
    b = (F / (name + ".bin")).read_bytes()
    return "\n".join(
        bytes(b[j : j + 160 : 2]).decode("ascii", "replace").rstrip()
        for j in range(0, 4000, 160)
    )


assert "Window Manager installed." in text("installed"), text("installed")
for name, chord in hotkeys:
    wake = text("wake-" + name)
    idle = text("idle-" + name)
    expected_menu = "N=0:  Place Size Scroll Tracking Display Color Write Abort         [ESC]->Exit"
    assert len(expected_menu) <= 80, expected_menu
    assert wake.splitlines()[-1] == expected_menu, wake
    assert "N=" not in idle.splitlines()[-1], idle
if not cdos:
    attrs = set((F / "colored.bin").read_bytes()[1 : 24 * 160 : 2])
    assert attrs == {0x12}, sorted(hex(a) for a in attrs)
    assert "Directory For User" in text("typed").title(), text("typed")
    placed = text("placed").splitlines()
    scrolled = text("scrolled").splitlines()
    row = next(i for i, x in enumerate(placed) if "System Files Exist" in x)
    assert placed[row][1:] == "System Files Exist", placed
    assert scrolled[row][1:] == "ystem Files Exist", scrolled
    assert text("written").splitlines()[-1].startswith("N=0:"), text("written")
    assert b"\0WINDOW0 TXT" in (F / (stem + ".img")).read_bytes()
    assert (
        " 0     02   02   23    79    02   02     No    Color  Green   Blue"
        in text("view").replace("\ufffd", "")
    ), text("view")
for name in ["uninstalled", "exited", "unregistered"]:
    assert "N=" not in text(name).splitlines()[-1], text(name)
    assert [x for x in text(name).splitlines()[:24] if x][-1] == "A>", text(name)
assert [x for x in text("plain-zero").splitlines()[:24] if x][-1] == "A>0", text("plain-zero")
print(
    "PASS: "
    + stem
    + " Ctrl+Alt+0, Ctrl+Alt+Esc and Ctrl+keypad-Plus wake WMENU; Esc hides it, Abort exits,"
    + ("" if cdos else " every displayed command works (" + boot + "),")
    + " uninstalled hotkeys do not leak and plain 0 still works",
    flush=True,
)
shutil.rmtree(F)
