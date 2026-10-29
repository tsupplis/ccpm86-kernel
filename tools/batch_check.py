#!/usr/bin/env python3
"""Exercise maintained BATCH messages and project-specific aliases."""

from pathlib import Path
import tempfile, subprocess, shutil

R = Path(__file__).resolve().parent.parent
cdos = (R / "media/cdos86.cfg").exists()
stem = "cdos86" if cdos else "ccpm86"
F = Path(tempfile.mkdtemp(prefix="batch-live-"))
print("Diagnostics:", F, flush=True)


def run(*a):
    p = subprocess.run(a, cwd=F, capture_output=True)
    if p.returncode:
        raise RuntimeError(a, p.stderr.decode(errors="replace"))
    return p


shutil.copyfile(R / "media/diskdefs", F / "diskdefs")
twist = str(R / "tools/cpm86twist.py")
run("python3", twist, "untwist", str(R / "media" / (stem + ".img")), "a.logical")
run("cpmrm", "-f", "ccpm86-320", "a.logical", "0:batch.cmd")
run(
    "cpmcp",
    "-f",
    "ccpm86-320",
    "a.logical",
    str(R / "commands/batch/batch.cmd"),
    "0:batch.cmd",
)
run("cpmchattr", "-f", "ccpm86-320", "a.logical", "s", "0:batch.cmd")
run("python3", twist, "twist", "a.logical", stem + ".img")
if cdos:
    shutil.copyfile(F / "a.logical", F / "b.logical")
else:
    run("python3", twist, "untwist", str(R / "media/ccpmtools.img"), "b.logical")
lines = (["CLS"] if cdos else []) + ["USER 1", "USER 0", "PAUSE", "DEL B:DROP.TST"]
if not cdos:
    lines += ["RENAME B:NEW.TST=B:KEEP.TST", "ERASE B:KEEP.TST"]
(F / "message.bat").write_bytes(("\r\n".join(lines) + "\r\n").encode() + b"\x1a")
run("cpmcp", "-f", "ccpm86-320", "b.logical", "message.bat", "0:message.bat")
(F / "sentinel").write_bytes(b"KEEP\r\n\x1a")
for n in ["drop.tst", "keep.tst"]:
    run("cpmcp", "-f", "ccpm86-320", "b.logical", "sentinel", "0:" + n)
run("python3", twist, "twist", "b.logical", "b.img")
cfg = (
    (R / "media" / (stem + ".cfg"))
    .read_text()
    .replace('driver = "sdl"', 'driver = "null"')
    .replace('driver      = "sdl:lowpass=5000"', 'driver      = "null"')
    .replace(
        'file     = "' + ("cdostools" if cdos else "ccpmtools") + '.img"',
        'file     = "b.img"',
    )
)
(F / (stem + ".cfg")).write_text(cfg)
c = ["c f000000"]
keys = {":": "+Shift Semicolon -Shift", ".": "Period"}
for ch in "b:message.bat":
    c += ["key " + keys.get(ch, ch), "c 200000"]
c += [
    "key Return",
    "c 10000000",
    'save "pause.bin" bin b800:0000 fa0',
    "key Space",
    "c 10000000",
    'save "done.bin" bin b800:0000 fa0',
    "q",
]
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


def screen(name):
    b = (F / (name + ".bin")).read_bytes()
    s = "\n".join(
        bytes(b[j : j + 160 : 2]).decode("ascii", "replace").rstrip()
        for j in range(0, 3840, 160)
    )
    (F / (name + ".txt")).write_text(s)
    return s


s = screen("pause")
assert "Press any key to continue..." in s, s
assert "User Number = 1" in s and "User Number = 0" in s, s
assert "User numbers only" not in s, s
s = screen("done")
assert [x for x in s.splitlines() if x][-1] == "A>", s
run("python3", twist, "untwist", "b.img", "final.logical")
names = run("cpmls", "-f", "ccpm86-320", "final.logical").stdout.decode().lower()
assert "keep.tst" in names, names
assert ("drop.tst" not in names) if cdos else ("drop.tst" in names), names
print(
    "PASS: "
    + stem
    + " BATCH USER without note, common PAUSE, "
    + ("CLS and DEL->ERA" if cdos else "simplified DOS alias dispatch")
    + "; unrelated file preserved",
    flush=True,
)
shutil.rmtree(F)
