"""Combined UI regression on disposable disks; exits its own emulator."""

from pathlib import Path
import tempfile, shutil, subprocess, re, importlib.util, time, os, sys
from datetime import datetime

R = Path(__file__).resolve().parents[3]
F = Path(tempfile.mkdtemp(prefix="fm-user-"))
# A saved pristine fixture can isolate this run from concurrent media rebuilds.
boot_image = Path(os.environ.get("FM_TEST_BOOT", str(R / "media/cdos86.img")))
shutil.copyfile(boot_image, F / "cdos86.img")
subprocess.run(
    [
        "python3",
        str(R / "tools/cpm86twist.py"),
        "untwist",
        str(F / "cdos86.img"),
        str(F / "logical.img"),
    ],
    check=True,
)
(F / "READTEST.TXT").write_bytes(b"REAL FILE CONTENT\r\nSECOND LINE\r\n\x1a")
subprocess.run(
    [
        "cpmcp",
        "-f",
        "ccpm86-320",
        str(F / "logical.img"),
        str(F / "READTEST.TXT"),
        "4:READTEST.TXT",
    ],
    check=True,
    cwd=R / "media",
)
# Native layout variants live on A:, keeping the FAT catalog test unchanged.
for name, position in (("LEFT", 3), ("TOP", 4), ("STRIP", 5)):
    template = F / (name + ".CFG")
    template.write_text("MENU POSITION=%d\r\nOBJECT FG COLOR=15\r\n"
                        "OBJECT BG COLOR=2\r\nCOMMAND FG COLOR=7\r\n"
                        "COMMAND BG COLOR=1\r\n" % position)
    subprocess.run(["cpmcp", "-f", "ccpm86-320", str(F / "logical.img"),
                    str(template), "0:" + name + ".CFG"], check=True, cwd=R / "media")
subprocess.run(
    [
        "python3",
        str(R / "tools/cpm86twist.py"),
        "twist",
        str(F / "logical.img"),
        str(F / "cdos86.img"),
    ],
    check=True,
)
subprocess.run(
    [
        "mformat",
        "-C",
        "-i",
        str(F / "fat.img"),
        "-t",
        "40",
        "-h",
        "2",
        "-n",
        "8",
        "-c",
        "2",
        "::",
    ],
    check=True,
)
subprocess.run(["mmd", "-i", str(F / "fat.img"), "::DOCS", "::DOCS/NESTED"], check=True)
subprocess.run(
    [
        "mcopy",
        "-i",
        str(F / "fat.img"),
        str(R / "commands/fm/fm.cmd"),
        "::FMVIEW.CMD",
    ],
    check=True,
)
# Exercise historical colour numbers with distinguishable panel backgrounds.
(F / "FM.CFG").write_bytes(
    b"OBJECT FG COLOR=15\r\nOBJECT BG COLOR=2\r\n"
    b"COMMAND FG COLOR=7\r\nCOMMAND BG COLOR=1\r\n"
    b"KEY FG COLOR=14\r\nKEY BG COLOR=4\r\n\x1a"
)
subprocess.run(
    ["mcopy", "-i", str(F / "fat.img"), str(F / "FM.CFG"), "::FM.CFG"],
    check=True,
)
(F / "FATREAD.TXT").write_bytes(
    b"FAT NESTED CONTENT\r\nEXACT END\r\n"
    + b"".join(("VIEW LINE %02d\r\n" % n).encode() for n in range(50))
)
stamp = datetime(2024, 2, 29, 12, 34).timestamp()
os.utime(F / "FATREAD.TXT", (stamp, stamp))
subprocess.run(
    [
        "mcopy",
        "-m",
        "-i",
        str(F / "fat.img"),
        str(F / "FATREAD.TXT"),
        "::DOCS/NESTED/FATREAD.TXT",
    ],
    check=True,
)
for number in range(100):
    path = F / ("ITEM%03d.TXT" % number)
    path.write_bytes(("CATALOG ITEM %03d" % number).encode())
subprocess.run(
    ["mcopy", "-i", str(F / "fat.img")]
    + [str(F / ("ITEM%03d.TXT" % n)) for n in range(100)]
    + ["::"],
    check=True,
)
cfg = (R / "media/cdos86.cfg").read_text().replace("cdostools.img", "fat.img")
cfg = re.sub(
    r'(?m)^path\s*=\s*"([^"\n]+)"',
    lambda m: 'path = "' + str((R / "media" / m[1]).resolve()) + '"',
    cfg,
)
(F / "cdos86.cfg").write_text(cfg)
spec = importlib.util.spec_from_file_location("pce", R / "commands/fm/tests/pce.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
if os.environ.get("FM_TEST_ACTIONS"):
    import actlive
    actlive.prepare(F)
if os.environ.get("FM_TEST_RUN"):
    import runlive
    runlive.prepare(R, F)
vm = m.PCE(F, ["pce-ibmpc", "-c", "cdos86.cfg", "-t", "null", "-g", "vga"], timeout=420)
print("PCE PID", vm.process.pid, "folder", F, flush=True)


def copy_to(destination):
    vm.key("C")
    vm.wait("copy destination", ["Copy destination"])
    for character in destination:
        vm.key(
            "+Shift Semicolon -Shift"
            if character == ":"
            else "Backslash" if character == chr(92) else character
        )
    vm.key("Return")


def check_layouts():
    for name, x, y, expected_attr in (("LEFT", 20, 0, 0x4f),
                                      ("TOP", 0, 3, 0x4f),
                                      ("STRIP", 79, 20, 0x17)):
        for character in "A:PIP B:FM.CFG=A:" + name + ".CFG":
            vm.key({":": "+Shift Semicolon -Shift", " ": "Space",
                    ".": "Period", "=": "Equal"}.get(character, character))
        vm.key("Return")
        # Earlier prompts remain on screen while PIP runs. Require the final
        # nonblank line to be the new prompt before typing another command.
        deadline = time.monotonic() + 15
        ready = 0
        while ready < 2:
            frame, keyboard = vm.frame(deadline)
            ready = ready + 1 if vm.text(frame).rstrip().endswith("B>") and keyboard[:2] == keyboard[2:] else 0
        for character in "FMVIEW":
            vm.key("Space" if character == " " else character)
        vm.key("Return")
        vm.wait(name + " layout", ["B:", "Select a command"])
        frame, unused = vm.frame(time.monotonic() + 5)
        assert frame[(y * 80 + x) * 2 + 1] == expected_attr, name
        vm.key("End")
        vm.key("Return")
        vm.wait("last command reachable", ["This action is not available"])
        vm.key("Esc")
        vm.key("F9")
        vm.key("End")
        vm.key("F2")
        vm.wait("layout file selection", ["CATALOG ITEM 099"])
        vm.key("Esc")
        vm.key("Esc")
        vm.wait("layout shell", ["B>"])
    print("PASS: native left, top and compact menus, last command and file reachability", flush=True)


try:
    vm.wait("boot", ["A>"], timeout=45)
    for key in ["B", "+Shift Semicolon -Shift", "Return"]:
        vm.key(key)
    if os.environ.get("FM_TEST_ACTIONS"):
        actlive.run(vm)
        vm.close()
        actlive.verify(F)
        sys.exit(0)
    if os.environ.get("FM_TEST_RUN"):
        runlive.run(vm)
        vm.close()
        runlive.verify(R, F)
        sys.exit(0)
    if os.environ.get("FM_TEST_MUTATIONS"):
        import mutlive
        mutlive.run(vm)
        vm.close()
        mutlive.verify(F)
        sys.exit(0)
    if os.environ.get("FM_TEST_LAYOUT_ONLY"):
        check_layouts()
        vm.close()
        sys.exit(0)
    for key in ["F", "M", "V", "I", "E", "W", "Space", "R", "E", "A", "L", "Return"]:
        vm.key(key)
    vm.wait(
        "FAT root",
        ["B:", "DOCS", "ITEM000.TXT"],
        errors=["Cannot list", "FM framework error:"],
    )
    frame, unused = vm.frame(time.monotonic() + 5)
    assert frame[1] == 0x4f, "configured object colour"
    assert frame[60 * 2 + 1] == 0x17, "configured command colour"
    assert frame[(22 * 80 + 79) * 2 + 1] == 0x2e, "whole legend background"
    # Filter through the real Actions pane, retaining directory navigation.
    vm.key("Home")
    vm.key("Down")
    vm.key("Down")
    vm.key("Return")
    vm.wait("subset input", ["Subset of files"])
    vm.key("Home")
    vm.key("Delete")
    for key in [
        "I",
        "T",
        "E",
        "M",
        "0",
        "0",
        "+Shift Slash -Shift",
        "Period",
        "T",
        "X",
        "T",
    ]:
        vm.key(key)
    vm.key("Return")
    vm.wait("filtered catalog", ["DOCS", "ITEM000.TXT", "ITEM009.TXT"])
    vm.key("F7")
    vm.key("Return")
    vm.key("F9")
    vm.key("Home")
    vm.key("Right")
    vm.key("Space")
    vm.key("Right")
    vm.key("Space")
    copy_to("A:7")
    vm.wait(
        "marked pair copied",
        ["Action complete", "2 completed"],
        errors=["Action failed"],
    )
    vm.key("Esc")
    vm.key("F4")
    vm.key("Tab")
    vm.key("Home")
    vm.key("Down")
    vm.key("Down")
    vm.key("Return")
    vm.key("Home")
    for unused in range(12):
        vm.key("Delete")
    vm.key("+Shift 8 -Shift")
    vm.key("Return")
    vm.key("F9")
    vm.key("Home")
    vm.key("Return")
    vm.wait(
        "FAT directory",
        ["B:\\DOCS", "NESTED"],
        errors=["Cannot list", "FM framework error:"],
    )
    vm.key("Right")
    vm.key("Return")
    vm.wait("FAT nested directory", ["NESTED", "FATREAD.TXT"], errors=["Cannot list"])
    vm.key("F7")
    vm.key("Return")
    vm.wait("slot recalled", ["ITEM000.TXT", "DOCS"])
    vm.key("F9")
    vm.key("Home")
    vm.key("Return")
    vm.key("Right")
    vm.key("Return")
    vm.wait("nested after slot recall", ["NESTED", "FATREAD.TXT"])
    # Metadata is read through the native adapter, including a known FAT stamp.
    vm.key("Tab")
    vm.key("End")
    vm.key("Up")
    vm.key("Return")
    vm.wait("FAT size and date", ["1 KiB", "2024-02-29 12:34"])
    vm.key("Return")
    vm.key("F9")
    vm.key("Right")
    vm.key("F2")
    vm.wait(
        "FAT Type",
        ["FAT NESTED CONTENT", "EXACT END"],
        errors=["Cannot open", "FM framework error:"],
    )
    vm.key("PageDown")
    vm.wait(
        "next page",
        ["page 2", "VIEW LINE 16"],
        errors=["Action failed", "FM framework error:"],
    )
    vm.key("PageUp")
    vm.wait("previous page", ["page 1", "FAT NESTED CONTENT"])
    vm.key("End")
    vm.wait("last page", ["page 3", "VIEW LINE 49", "EOF."], errors=["Action failed"])
    vm.key("Home")
    vm.wait("first page", ["page 1", "FAT NESTED CONTENT"])
    vm.key("Esc")
    copy_to("A:6")
    vm.wait(
        "FAT to CPM Copy",
        ["Action complete", "1 completed"],
        errors=["Action failed"],
        timeout=20,
    )
    vm.key("Esc")
    copy_to(r"B:\DOCS")
    vm.wait(
        "FAT to FAT Copy",
        ["Action complete", "1 completed"],
        errors=["Action failed"],
        timeout=20,
    )
    vm.key("Esc")
    copy_to(r"B:\DOCS")
    vm.wait("overwrite question", ["Replace FATREAD.TXT?"])
    vm.key("N")
    vm.wait("declined overwrite", ["Action cancelled"])
    vm.key("Esc")
    copy_to(r"B:\DOCS")
    vm.wait("overwrite question again", ["Replace FATREAD.TXT?"])
    vm.key("Y")
    vm.wait(
        "overwrite committed",
        ["Action complete", "1 completed"],
        errors=["Action failed"],
        timeout=20,
    )
    vm.key("Esc")
    copy_to(r"B:\DOCS\NESTED")
    vm.wait("self-copy rejected", ["Action failed", "same", "directory."])
    vm.key("Esc")
    vm.key("Backspace")
    vm.key("Backspace")
    vm.wait("FAT root restored", ["DOCS", "ITEM000.TXT"])
    vm.key("End")
    vm.key("Space")
    vm.key("F2")
    vm.wait(
        "large catalog Type",
        ["CATALOG ITEM 099"],
        errors=["Cannot open", "FM framework error:"],
    )
    vm.key("Esc")
    vm.key("F4")
    vm.key("Tab")
    vm.key("Home")
    for key in ["Down", "Down", "Down", "Return"]:
        vm.key(key)
    vm.wait("drives", ["Select drive"])
    vm.key("Home")
    vm.key("Return")
    vm.wait("users", ["Select user area"])
    for key in ["Down", "Down", "Down", "Down", "Return"]:
        vm.key(key)
    vm.wait("listing", ["A: User 4", "READTEST.TXT"], errors=["FM framework error:"])
    vm.key("End")
    vm.key("Up")
    vm.key("Return")
    vm.wait("CPM size", ["READTEST.TXT", "1 KiB"])
    vm.key("Return")
    vm.key("F2")
    vm.wait(
        "type", ["REAL FILE CONTENT", "SECOND LINE"], errors=["FM framework error:"]
    )
    vm.key("Esc")
    deadline = time.monotonic() + 10
    while True:
        frame, _ = vm.frame(deadline)
        text = vm.text(frame)
        if "REAL FILE CONTENT" not in text and "READTEST.TXT" in text:
            break
    copy_to(r"B:\DOCS")
    vm.wait(
        "CPM to FAT Copy",
        ["Action complete", "1 completed"],
        errors=["Action failed"],
        timeout=20,
    )
    vm.key("Esc")
    vm.key("Esc")
    vm.wait("shell", ["B>"])
    check_layouts()

    print(
        "PASS: FAT nested/parent navigation, exact-byte Type, 100-file catalog selection, CP/M user 4 Type, viewer navigation, shared Copy lifecycle and exit",
        flush=True,
    )
    vm.close()
    # Inspect persisted output after orderly emulator exit; screen success is not enough.
    subprocess.run(
        [
            "python3",
            str(R / "tools/cpm86twist.py"),
            "untwist",
            str(F / "cdos86.img"),
            str(F / "checked.img"),
        ],
        check=True,
    )
    subprocess.run(
        [
            "cpmcp",
            "-f",
            "ccpm86-320",
            str(F / "checked.img"),
            "6:FATREAD.TXT",
            str(F / "copied-cpm.dat"),
        ],
        check=True,
        cwd=R / "media",
    )
    subprocess.run(
        [
            "cpmcp",
            "-f",
            "ccpm86-320",
            str(F / "checked.img"),
            "4:READTEST.TXT",
            str(F / "source-cpm.dat"),
        ],
        check=True,
        cwd=R / "media",
    )
    subprocess.run(
        [
            "mcopy",
            "-i",
            str(F / "fat.img"),
            "::DOCS/FATREAD.TXT",
            str(F / "copied-fat.dat"),
        ],
        check=True,
    )
    subprocess.run(
        [
            "mcopy",
            "-i",
            str(F / "fat.img"),
            "::DOCS/READTEST.TXT",
            str(F / "copied-from-cpm.dat"),
        ],
        check=True,
    )
    for number in (0, 1):
        name = "ITEM%03d.TXT" % number
        output = F / ("pair%d.dat" % number)
        subprocess.run(
            [
                "cpmcp",
                "-f",
                "ccpm86-320",
                str(F / "checked.img"),
                "7:" + name,
                str(output),
            ],
            check=True,
            cwd=R / "media",
        )
        expected_pair = (F / name).read_bytes()
        assert output.read_bytes() == expected_pair + bytes([26]) * (
            -len(expected_pair) % 128
        )
    expected = (F / "FATREAD.TXT").read_bytes()
    assert (F / "copied-fat.dat").read_bytes() == expected
    assert (F / "copied-cpm.dat").read_bytes() == expected + bytes([26]) * (
        -len(expected) % 128
    )
    # cpmcp extracts the byte-count metadata; BDOS reads complete records.
    # Locate this unique fixture payload in the pristine image and compare the
    # actual record bytes, including padding, rather than inventing a pad value.
    source = (F / "source-cpm.dat").read_bytes()
    raw = (F / "logical.img").read_bytes()
    assert raw.count(source) == 1
    start = raw.index(source)
    records = (len(source) + 127) // 128 * 128
    assert (F / "copied-from-cpm.dat").read_bytes() == raw[start : start + records]
    names = subprocess.check_output(
        ["mdir", "-b", "-i", str(F / "fat.img"), "::DOCS"]
    ).decode()
    assert ".$$$" not in names
    print(
        "PASS: persisted FAT/FAT, FAT/CPM and CPM/FAT bytes; overwrite, decline, self-copy rejection and no temporary residue",
        flush=True,
    )
except Exception:
    if vm.process.poll() is None:
        vm.send("r")
        vm.send('save "failure.bin" bin 0000:0000 a0000')
        vm.frame(time.monotonic() + 2)
    vm.abort()
    raise
