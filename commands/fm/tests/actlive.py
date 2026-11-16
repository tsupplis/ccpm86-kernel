"""Print and Free Memory native acceptance; disposable printer capture only."""
import subprocess


def prepare(folder):
    config = folder / "cdos86.cfg"
    # Logical printer 1 is the 378h port; 3BCh overlaps VGA emulation.
    text = config.read_text().replace("stdio:file=/dev/null:flush=1",
                                      "stdio:file=print.out:flush=1")
    config.write_text(text)
    (folder / "stop.c").write_text('#include <stdio.h>\nint main(void) { puts("STOP PROBE RETURNED"); return 0; }\n')
    subprocess.run(["aztec43_acc", "-target=cpm86", "-Osize", "-o", "stop.cmd", "stop.c"],
                   cwd=folder, check=True)
    subprocess.run(["mcopy", "-i", str(folder / "fat.img"), str(folder / "stop.cmd"), "::STOP.CMD"], check=True)


def run(vm):
    def keys(*items):
        for key in items: vm.key(key)

    def text(value):
        for ch in value:
            vm.key({" ": "Space", ".": "Period", "?": "+Shift Slash -Shift"}.get(ch, ch))

    def action(index):
        keys("F9", "Tab", "Home")
        for unused in range(index): vm.key("Down")
        vm.key("Return")

    text("PRINTER 1"); vm.key("Return")
    vm.wait("printer selection", ["Printer Number = 1"])
    text("FMVIEW"); vm.key("Return")
    vm.wait("browser", ["DOCS", "ITEM000.TXT"])
    action(2); keys("Home", "Delete"); text("ITEM00?.TXT"); vm.key("Return")
    keys("F9", "Home", "Right", "Space", "Right", "Space")
    action(5)
    vm.wait("printing complete", ["Action complete", "Selected files printed"], errors=["Action failed"], timeout=30)
    vm.key("Esc")
    # Printed files can be marked again; old completion must not clear new marks.
    keys("F9", "Home", "Right", "Space")
    action(5)
    vm.wait("printing repeated", ["Action complete", "Selected files printed"], errors=["Action failed"], timeout=30)
    vm.key("Esc")
    action(14); vm.wait("STOP arguments", ["STOP arguments"])
    vm.key("Return")
    vm.wait("STOP execution", ["STOP PROBE RETURNED", "Press any key"], timeout=30)
    vm.key("Return"); vm.wait("STOP return", ["Program returned to FM"])
    keys("Esc", "Esc"); vm.wait("shell", ["B>"])
    print("PASS: Print selection/reselection and shared STOP launch/return", flush=True)


def verify(folder):
    expected = b"CATALOG ITEM 000\fCATALOG ITEM 001\fCATALOG ITEM 000\f"
    actual = (folder / "print.out").read_bytes()
    assert actual == expected, (actual, expected)
    print("PASS: exact physical printer output and per-file form feeds", flush=True)
