"""Launch acceptance using browse.py's disposable fixture and one emulator.

Utility names contain the harmless probe, never real formatting/copy programs.
"""
import subprocess

PROBE = r'''#include <stdio.h>
int main(int argc, char **argv)
{
    FILE *file;
    int i;
    file = fopen("RETURN.OK", "w");
    if (!file) { puts("PROBE OPEN FAILED"); return 1; }
    for (i = 1; i < argc; ++i) fprintf(file, "%s\n", argv[i]);
    fclose(file);
    puts("PROBE WROTE FILE");
    return 0;
}
'''


def prepare(root, folder):
    (folder / "probe.c").write_text(PROBE)
    for target, name in (("cpm86", "probe.cmd"), ("dos2", "probe.exe")):
        subprocess.run(["aztec43_acc", "-target=" + target, "-Osize",
                        "-o", name, "probe.c"], cwd=folder, check=True)
    for name in ("PROBE.CMD", "HELP.CMD", "DREDIX.CMD", "DCOPY.CMD", "DFORM.CMD"):
        subprocess.run(["mcopy", "-i", str(folder / "fat.img"),
                        str(folder / "probe.cmd"), "::" + name], check=True)
    subprocess.run(["mcopy", "-i", str(folder / "fat.img"),
                    str(folder / "probe.exe"), "::PROBE.EXE"], check=True)
    subprocess.run(["cpmcp", "-f", "ccpm86-320", str(folder / "logical.img"),
                    str(folder / "probe.cmd"), "4:PROBE.CMD"], cwd=root / "media", check=True)
    subprocess.run(["python3", str(root / "tools/cpm86twist.py"), "twist",
                    str(folder / "logical.img"), str(folder / "cdos86.img")], check=True)


def run(vm):
    def keys(*items):
        for key in items: vm.key(key)

    def text(value):
        for ch in value:
            vm.key({" ": "Space", ".": "Period", ":": "+Shift Semicolon -Shift"}.get(ch, ch))

    def returned():
        vm.wait("child output", ["PROBE WROTE FILE", "Press any key"], timeout=30)
        vm.key("Return")
        vm.wait("child return", ["Action complete"],
                errors=["Action failed", "Unexpected Interrupt"], timeout=30)
        vm.key("Esc")

    def command(value):
        vm.key("F10")
        vm.wait("command input", ["Action input", "External command"])
        text(value); vm.key("Return")

    def action(index):
        keys("F9", "Tab", "Home")
        for unused in range(index): vm.key("Down")
        vm.key("Return")

    text("FMVIEW"); vm.key("Return")
    vm.wait("FM root", ["DOCS", "Select a command"])
    command("PROBE.CMD ROOT"); returned()
    command("PROBE.EXE DOS"); returned()
    # F5/F6 use one selected file; Help and disk wrappers need none.
    action(2); vm.key("Home")
    for unused in range(12): vm.key("Delete")
    text("PROBE.CMD"); keys("Return", "F9", "Home", "Right", "F5")
    vm.wait("Run input", ["Program arguments"])
    vm.key("Return"); returned()
    keys("F9", "Home", "Right", "F6")
    vm.wait("Edit input", ["Editor arguments"])
    vm.key("Return"); returned()
    vm.key("F1"); vm.wait("Help input", ["Help topic"])
    vm.key("Return"); returned()
    for index in (12, 13):
        action(index); vm.wait("disk utility input", ["Action input"])
        vm.key("Return"); returned()
    command("MISSING.CMD"); vm.wait("missing program", ["Action failed"])
    vm.key("Esc")
    # Clear subset before navigating.
    action(2); vm.key("Home")
    for unused in range(12): vm.key("Delete")
    vm.key("+Shift 8 -Shift"); keys("Return", "F9", "Home", "Right", "Right", "Return")
    vm.wait("DOCS", ["NESTED"])
    keys("Home", "Right", "Return")
    vm.wait("nested folder", ["FATREAD.TXT"])
    command("B:PROBE.CMD NESTED"); returned()
    vm.wait("nested refresh", ["RETURN.OK", "FATREAD.TXT"])
    action(3); vm.wait("drives", ["Select drive"])
    keys("Home", "Return"); vm.wait("users", ["Select user area"])
    keys("Down", "Down", "Down", "Down", "Return")
    vm.wait("CPM listing", ["A: User 4", "PROBE.CMD"])
    command("PROBE.CMD CPMUSER4"); returned()
    vm.wait("CPM refresh", ["A: User 4", "RETURN.OK"])
    vm.key("Esc"); vm.wait("shell return", ["B>"])
    print("PASS: CMD/EXE, F5/F6/Help/disk routes, missing program, FAT nested and CP/M user 4 launch/return", flush=True)


def verify(root, folder):
    data = subprocess.check_output(["mtype", "-i", str(folder / "fat.img"), "::DOCS/NESTED/RETURN.OK"])
    assert b"NESTED" in data
    subprocess.run(["python3", str(root / "tools/cpm86twist.py"), "untwist",
                    str(folder / "cdos86.img"), str(folder / "checked.img")], check=True)
    subprocess.run(["cpmcp", "-f", "ccpm86-320", str(folder / "checked.img"),
                    "4:RETURN.OK", str(folder / "returned.txt")], cwd=root / "media", check=True)
    assert b"CPMUSER4" in (folder / "returned.txt").read_bytes()
    print("PASS: persisted child output in correct FAT directory and CP/M user area", flush=True)
