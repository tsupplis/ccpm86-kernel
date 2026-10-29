#!/usr/bin/env python3
"""Exercise both WMENU wake shortcuts on a disposable PCE disk pair."""

from pathlib import Path
import tempfile, subprocess, shutil

R = Path(__file__).resolve().parent.parent
cdos = (R / "media/cdos86.cfg").exists()
stem = "cdos86" if cdos else "ccpm86"
F = Path(tempfile.mkdtemp(prefix="wmenu-key-"))
print("Diagnostics:", F, flush=True)


def run(*args):
    return subprocess.run(args, cwd=F, check=True, capture_output=True)


shutil.copyfile(R / "media" / (stem + ".img"), F / (stem + ".img"))
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


key("+Ctrl +Alt Esc -Alt -Ctrl")  # Without a manager this must not leak Escape.
for x in "b:wmenu":
    key("+Shift Semicolon -Shift" if x == ":" else "Period" if x == "." else x)
key("Return")
save("installed")
key("+Ctrl +Alt Esc -Alt -Ctrl")
save("new")
key("Esc")
save("idle")
key("+Ctrl KP_Plus -Ctrl")
save("old")
key("Esc")
key("+Ctrl +Alt Esc -Alt -Ctrl")
if cdos:
    for i in range(7):
        key("Space")
    key("Return")
else:
    key("a")
key("Return")
save("exited")
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
for name in ["new", "old"]:
    assert "N=" + ("1" if cdos else "0") in text(name).splitlines()[-1], text(name)
for name in ["idle", "exited"]:
    assert "N=" not in text(name).splitlines()[-1], text(name)
assert [x for x in text("exited").splitlines()[:24] if x][-1] == "A>", text("exited")
print(
    "PASS: "
    + stem
    + " Ctrl+Alt+Esc and Ctrl+keypad-Plus wake WMENU; Esc hides it and Abort exits",
    flush=True,
)
shutil.rmtree(F)
