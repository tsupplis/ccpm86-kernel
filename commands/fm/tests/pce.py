"""Bounded, observable PCE monitor interaction; no bulk blind cycle waits."""

from pathlib import Path
import subprocess, time, sys


class PCE:
    def __init__(self, folder, command=None, timeout=180, step=0x40000):
        self.folder = Path(folder)
        self.started = time.monotonic()
        self.deadline = self.started + timeout
        self.step = step
        self.polls = 0
        self.cycles = 0
        self.last = None
        self.log = (self.folder / "pce.log").open("wb")
        self.process = subprocess.Popen(
            command or ["pce-ibmpc", "-c", "cdos86.cfg", "-t", "null", "-g", "vga"],
            cwd=self.folder,
            stdin=subprocess.PIPE,
            stdout=self.log,
            stderr=self.log,
        )

    @staticmethod
    def text(frame):
        return "\n".join(
            bytes(frame[i : i + 160 : 2]).decode("cp437").rstrip()
            for i in range(0, 3840, 160)
        )

    def check(self, deadline, what):
        status = self.process.poll()
        if status is not None:
            raise RuntimeError(
                "PCE exited with status %s while %s; see %s"
                % (status, what, self.folder / "pce.log")
            )
        if time.monotonic() >= min(deadline, self.deadline):
            raise TimeoutError(
                "PCE timeout while %s; last frame %s" % (what, self.last)
            )

    def send(self, text):
        self.check(self.deadline, "sending monitor command")
        try:
            self.process.stdin.write((text + "\n").encode())
            self.process.stdin.flush()
        except BrokenPipeError:
            raise RuntimeError(
                "PCE monitor pipe closed; see " + str(self.folder / "pce.log")
            )

    def frame(self, deadline):
        self.check(deadline, "advancing guest")
        self.polls += 1
        self.cycles += self.step
        vga = self.folder / ("poll-%05d.vga" % self.polls)
        bda = self.folder / ("poll-%05d.bda" % self.polls)
        self.last = vga
        self.send(
            'c %x\nsave "%s" bin b800:0000 fa0\nsave "%s" bin 0040:001a 4'
            % (self.step, vga.name, bda.name)
        )
        # A complete BDA dump follows VGA in the monitor stream and is the
        # command-completion barrier. Partial saves are never accepted.
        while True:
            if vga.exists() and bda.exists():
                image = vga.read_bytes()
                keyboard = bda.read_bytes()
                if len(image) == 4000 and len(keyboard) == 4:
                    return image, keyboard
            self.check(deadline, "waiting for monitor snapshot")
            time.sleep(0.005)

    def wait(self, label, expected=(), errors=(), timeout=10, stable=2):
        deadline = min(self.deadline, time.monotonic() + timeout)
        previous = None
        quiet = 0
        while True:
            frame, keyboard = self.frame(deadline)
            text = self.text(frame)
            ready = all(word in text for word in expected)
            for error in errors:
                if error in text:
                    raise AssertionError(
                        "%s: unexpected %r; frame %s" % (label, error, self.last)
                    )
            empty = keyboard[:2] == keyboard[2:]
            quiet = (
                quiet + 1 if empty and frame[:3840] == previous else 1 if empty else 0
            )
            previous = frame[:3840]
            if ready and quiet >= stable:
                return frame
            self.check(deadline, "waiting for " + label)

    def key(self, key):
        self.send("key " + key)
        # Idle/no-change keys are valid: consume the BIOS queue and observe
        # two identical subsequent displays, rather than requiring text change.
        return self.wait("key " + key, stable=3)

    def snapshot(self, name, expected=(), errors=(), timeout=10):
        frame = self.wait(name, expected, errors, timeout)
        (self.folder / (name + ".bin")).write_bytes(frame)
        (self.folder / (name + ".txt")).write_text(self.text(frame))
        return frame

    def close(self):
        self.send("q")
        self.process.stdin.close()
        try:
            self.process.wait(
                timeout=max(0.01, min(10, self.deadline - time.monotonic()))
            )
        except subprocess.TimeoutExpired:
            self.abort()
            raise TimeoutError("PCE did not exit after q")
        self.log.close()
        if self.process.returncode:
            raise RuntimeError("PCE failed on exit: " + str(self.process.returncode))
        self.elapsed = time.monotonic() - self.started

    def abort(self):
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait(timeout=5)
        if self.process.stdin and not self.process.stdin.closed:
            self.process.stdin.close()
        self.log.close()

    def __enter__(self):
        return self

    def __exit__(self, kind, value, trace):
        if kind:
            self.abort()
        else:
            self.close()


def self_test():
    """Real fake subprocesses prove early readiness, death and wall deadlines."""
    import tempfile, unittest

    fake = r"""
import sys,re
from pathlib import Path
mode=sys.argv[1]
if mode=='dead':sys.exit(7)
for line in sys.stdin:
 if line.strip()=='q':break
 if mode=='silent':continue
 m=re.match(r'save "([^"]+)" bin (.*)',line)
 if not m:continue
 if '0040:' in m[2]:data=b'\x1e\x00'*2
 else:
  data=bytearray(b' \x07'*2000)
  for i,c in enumerate(b'READY'):data[i*2]=c
 Path(m[1]).write_bytes(data)
"""

    class Tests(unittest.TestCase):
        def fixture(self, mode):
            tmp = tempfile.TemporaryDirectory()
            self.addCleanup(tmp.cleanup)
            p = Path(tmp.name) / "fake.py"
            p.write_text(fake)
            vm = PCE(tmp.name, [sys.executable, str(p), mode], timeout=2)
            self.addCleanup(vm.abort)
            return vm

        def test_early_ready(self):
            vm = self.fixture("ready")
            frame = vm.wait("ready", ["READY"], stable=1)
            self.assertIn("READY", vm.text(frame))
            self.assertEqual(vm.polls, 1)

        def test_no_change_key(self):
            vm = self.fixture("ready")
            vm.key("Home")
            self.assertEqual(vm.polls, 3)

        def test_timeout(self):
            vm = self.fixture("silent")
            start = time.monotonic()
            with self.assertRaises(TimeoutError):
                vm.wait("never", ["READY"], timeout=0.05)
            self.assertLess(time.monotonic() - start, 0.5)

        def test_process_death(self):
            vm = self.fixture("dead")
            with self.assertRaisesRegex(RuntimeError, "status 7"):
                vm.wait("dead", ["READY"])

        def test_error_is_immediate(self):
            vm = self.fixture("ready")
            with self.assertRaises(AssertionError):
                vm.wait("error", errors=["READY"])
            self.assertEqual(vm.polls, 1)

    result = unittest.TextTestRunner(verbosity=2).run(
        unittest.defaultTestLoader.loadTestsFromTestCase(Tests)
    )
    if not result.wasSuccessful():
        raise SystemExit(1)


if __name__ == "__main__":
    self_test()
