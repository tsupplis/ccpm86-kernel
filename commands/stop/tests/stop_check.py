#!/usr/bin/env python3
"""Compare STOP CMD behavior using an isolated, synthetic Concurrent system."""

import argparse
from dataclasses import dataclass
from pathlib import Path
import struct
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent
CODE_SEG = 0x2000
DATA_SEG = 0x3000
SYS_SEG = 0x4000
SYS_OFFSET = 0x0100
GROUP_A_SEG = 0x5000
GROUP_B_SEG = 0x6000
MARKER = b"<STOPTEST:"
PROMPT = b"\r\nEnter the Number to delete or RETURN to exit "
HEADING = (
    b"\r\n  Entry   Program   Memory   Window\r\n"
    b"  Number  Name      Used     Number \r\n"
    b"  ------  --------  -------  -------\r\n"
)
POPULATED_REPORT = HEADING + (
    b"    **    ROOT          12K     1\r\n"
    b"    1.    WORKER         2K     2\r\n"
    b"    2.    TEST           1K     4\r\n"
    b"\r\nContiguous Blocks of Free Memory\r\n"
    b"         1000:0          3K\r\n"
    b"         2000:0          1K\r\n"
    b"\r\nFree Shared Code  ==     2K"
    b"\r\nTotal Free Memory ==     6K Bytes\r\n"
)
EMPTY_REPORT = HEADING + (
    b"\r\nContiguous Blocks of Free Memory\r\n"
    b"            None"
    b"\r\nFree Shared Code  ==     0K"
    b"\r\nTotal Free Memory ==     0K Bytes\r\n"
)


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise ValueError(f"expected exactly one occurrence of {old!r}")
    return text.replace(old, new, 1)


def memory_report(extents, shared, total):
    start = POPULATED_REPORT.index(b"\r\nContiguous Blocks of Free Memory\r\n")
    return (
        POPULATED_REPORT[:start]
        + b"\r\nContiguous Blocks of Free Memory\r\n"
        + extents
        + f"\r\nFree Shared Code  =={shared:6}K".encode()
        + f"\r\nTotal Free Memory == {total:5}K Bytes\r\n".encode()
    )


def many_rows_report(count):
    rows = bytearray()
    for index in range(count):
        number = "    **" if index == 0 else f"{index:5}."
        rows.extend(f"{number}    PROC{index:04}{1:8}K{index + 1:6}\r\n".encode())
    return HEADING + rows + EMPTY_REPORT[len(HEADING):]


MANY_ROWS_REPORT = many_rows_report(64)
MAX_ROWS_REPORT = many_rows_report(65)
NO_PROCESSES_REPORT = HEADING + POPULATED_REPORT[
    POPULATED_REPORT.index(b"\r\nContiguous Blocks of Free Memory\r\n"):
]
REPEATED_MEMORY_REPORT = replace_once(
    POPULATED_REPORT, b"ROOT          12K", b"ROOT          20K",
)
CONSOLE_WRAP_REPORT = replace_once(
    POPULATED_REPORT, b"WORKER         2K     2", b"WORKER         2K     0",
)
EMPTY_GROUP_REPORT = memory_report(
    b"         1000:0          3K\r\n         2000:0          1K\r\n", 0, 4,
)
BOTH_SHARED_REPORT = memory_report(
    b"         1000:0          3K\r\n         2000:0          1K\r\n", 3, 7,
)
WRAP_REPORT = memory_report(
    b"         FFF0:0          1K\r\n         2000:0          1K\r\n", 2, 4,
)
FRACTIONAL_REPORT = memory_report(
    b"         1000:0          1K\r\n         2000:0          0K\r\n", 2, 4,
)
OVERFLOW_REPORT = memory_report(
    b"         1000:0          1K\r\n         2000:0          1K\r\n", 2, 4,
)
REORDERED_REPORT = memory_report(
    b"         2000:0          1K\r\n         1000:0          3K\r\n", 2, 6,
)
SHARED_LIMIT_REPORT = HEADING + (
    b"\r\nContiguous Blocks of Free Memory\r\n"
    b"            None"
    b"\r\nFree Shared Code  ==    65K"
    b"\r\nTotal Free Memory ==    65K Bytes\r\n"
)


@dataclass(frozen=True)
class Case:
    name: str
    input_line: bytes = b""
    tail: bytes = b""
    version: int = 0x1431
    fixture: str = "populated"
    expected_report: bytes = POPULATED_REPORT
    rejected: bool = False
    abort_result: int = 0
    abort_pd: int = 0
    abort_console: int = 0
    abort_name: bytes = b""
    message: bytes = b""


CASES = (
    Case("populated-return"),
    Case("empty-return", fixture="empty", expected_report=EMPTY_REPORT),
    Case("no-processes", fixture="no-processes", expected_report=NO_PROCESSES_REPORT),
    Case("empty-group-count", fixture="empty-group", expected_report=EMPTY_GROUP_REPORT),
    Case("both-shared", fixture="both-shared", expected_report=BOTH_SHARED_REPORT),
    Case("shared-owner-chain", fixture="shared-owner-chain"),
    Case("shared-snapshot-limit", fixture="many-shared", expected_report=SHARED_LIMIT_REPORT),
    Case("wrapping-free-extents", fixture="wrap", expected_report=WRAP_REPORT),
    Case("fractional-memory", fixture="fractional", expected_report=FRACTIONAL_REPORT),
    Case("memory-total-overflow", fixture="overflow", expected_report=OVERFLOW_REPORT),
    Case("free-list-order", fixture="reordered", expected_report=REORDERED_REPORT),
    Case("repeated-memory", fixture="repeated-memory", expected_report=REPEATED_MEMORY_REPORT),
    Case("nonempty-tail", tail=b"X"),
    Case("zero", input_line=b"0"),
    Case("whitespace-only", input_line=b" \t"),
    Case("select-worker", input_line=b"1", abort_pd=0x840,
         abort_console=1, abort_name=b"WORKER  "),
    Case("select-attribute-name", input_line=b"2", abort_pd=0x8C0,
         abort_console=3, abort_name=b"\xd4EST    "),
    Case("leading-space", input_line=b" \t0001", abort_pd=0x840,
         abort_console=1, abort_name=b"WORKER  "),
    Case("maximum-input", input_line=b"0" * 125 + b"1", abort_pd=0x840,
         abort_console=1, abort_name=b"WORKER  "),
    Case("console-wrap", fixture="console-wrap", expected_report=CONSOLE_WRAP_REPORT,
         input_line=b"1", abort_pd=0x840, abort_console=255, abort_name=b"WORKER  "),
    Case("maximum-process-row", fixture="many-rows", expected_report=MANY_ROWS_REPORT,
         input_line=b"63", abort_pd=0x1BD0, abort_console=63, abort_name=b"PROC0063"),
    Case("process-row-overflow", fixture="many-rows", expected_report=MANY_ROWS_REPORT,
         input_line=b"64", message=b"\r\nEntry number too large"),
    Case("process-snapshot-limit", fixture="many-rows-66", expected_report=MAX_ROWS_REPORT,
         input_line=b"65", message=b"\r\nEntry number too large"),
    Case("abort-failure", input_line=b"1", abort_result=1, abort_pd=0x840,
         abort_console=1, abort_name=b"WORKER  ",
         message=b"\r\nThe selected program has not been stopped"),
    Case("nondigit", input_line=b"1x", message=b"\r\nInvalid Entry number."),
    Case("trailing-space", input_line=b"1 ", message=b"\r\nInvalid Entry number."),
    Case("too-large", input_line=b"3", message=b"\r\nEntry number too large"),
    Case("old-version", version=0x142F, rejected=True,
         message=b"Requires Concurrent to run"),
    Case("wrong-family", version=0x1231, rejected=True,
         message=b"Requires Concurrent to run"),
    Case("alternate-family", version=0x1130),
    Case("minimum-version", version=0x1430),
    Case("new-family", version=0xFF30),
)


def cmd_groups(path):
    image = path.read_bytes()
    if len(image) < 128 or image[0x7F] != 0:
        raise ValueError(f"{path}: unsupported CMD header or fixups")
    descriptors = []
    for offset in range(0, 0x48, 9):
        group_type = image[offset]
        if group_type == 0:
            break
        size, base = struct.unpack_from("<HH", image, offset + 1)
        if base:
            raise ValueError(f"{path}: fixed-address CMD groups are unsupported")
        descriptors.append((group_type, size * 16))
    if [group[0] for group in descriptors] != [1, 2]:
        raise ValueError(f"{path}: expected relocatable CODE and DATA groups")
    position = 128
    groups = []
    for _, size in descriptors:
        end = position + size
        if end > len(image) or size > 0xFFFF:
            raise ValueError(f"{path}: truncated or oversized CMD group")
        groups.append(image[position:end])
        position = end
    if any(image[position:]):
        raise ValueError(f"{path}: unsupported nonzero data after CMD groups")
    return groups


def verify_startup(reference, candidate):
    if len(reference) < 0x100 or len(candidate) < 0x100:
        raise ValueError("STOP CODE group is too small for the SCD3 startup")
    original = bytearray(reference[:0x100])
    rebuilt = bytearray(candidate[:0x100])
    for start, end in ((9, 11), (14, 16), (0x70, 0x80)):
        rebuilt[start:end] = original[start:end]
    if rebuilt != original:
        offset = next(i for i, (a, b) in enumerate(zip(original, rebuilt)) if a != b)
        raise AssertionError(f"SCD3 startup differs at CODE:{offset:04X}")
    print("PASS SCD3 startup matches release except relocated addresses and serial")


def system_image(fixture):
    many_rows = fixture in ("many-rows", "many-rows-65", "many-rows-66")
    many_shared = fixture == "many-shared"
    image = bytearray(0x5000 if many_rows or many_shared else 0x1000)
    group_a = bytearray(32)
    group_b = bytearray(32)

    def word(offset, value):
        struct.pack_into("<H", image, offset, value)

    def descriptor(offset, next_offset, start, length):
        word(offset, next_offset)
        word(offset + 2, start)
        word(offset + 4, length)

    def owner(offset, next_offset, memory):
        word(offset, next_offset)
        word(offset + 8, memory)

    def process(offset, next_offset, name, memory, console, flags=0):
        word(offset + 2, next_offset)
        image[offset + 6] = flags
        image[offset + 8:offset + 16] = name
        word(offset + 0x16, memory)
        image[offset + 0x20] = console

    if fixture == "empty":
        return image, group_a, group_b

    if many_shared:
        word(SYS_OFFSET + 0x70, 0x1000)
        group_a[0], group_a[9] = 1, 1
        for index in range(66):
            pd_offset = 0x1000 + index * 0x30
            owner_offset = 0x3000 + index * 0x10
            md_offset = 0x4000 + index * 0x10
            next_pd = pd_offset + 0x30 if index < 65 else 0
            process(pd_offset, next_pd, f"SHRD{index:04}".encode(), owner_offset, 0)
            owner(owner_offset, 0, md_offset)
            descriptor(md_offset, 0, GROUP_A_SEG, 0x40)
        return image, group_a, group_b

    if many_rows:
        count = {"many-rows": 64, "many-rows-65": 65, "many-rows-66": 66}[fixture]
        word(SYS_OFFSET + 0x72, 0x1000)
        for index in range(count):
            pd_offset = 0x1000 + index * 0x30
            owner_offset = 0x3000 + index * 0x10
            md_offset = 0x4000 + index * 0x10
            next_pd = pd_offset + 0x30 if index < count - 1 else 0
            process(pd_offset, next_pd, f"PROC{index:04}".encode(), owner_offset, index)
            owner(owner_offset, 0, md_offset)
            descriptor(md_offset, 0, 0x5000 + index * 0x40, 0x40)
        return image, group_a, group_b

    word(SYS_OFFSET + 0x5A, 0x200)
    word(SYS_OFFSET + 0x70, 0x600)
    word(SYS_OFFSET + 0x72, 0x800)
    descriptor(0x200, 0x210, 0x1000, 0x40)
    descriptor(0x210, 0x220, 0x1040, 0x80)
    descriptor(0x220, 0, 0x2000, 0x40)

    group_a[0], group_a[9], group_a[14] = 2, 0, 1
    group_b[0], group_b[9] = 1, 2
    process(0x600, 0x640, b"SHARED1 ", 0xA00, 0)
    process(0x640, 0, b"SHARED2 ", 0xA10, 0)
    owner(0xA00, 0, 0x300)
    owner(0xA10, 0, 0x310)
    descriptor(0x300, 0, 0x5000, 0x80)
    descriptor(0x310, 0, 0x6000, 0x40)

    process(0x800, 0x840, b"ROOT    ", 0xB00, 0)
    process(0x840, 0x880, b"WORKER  ", 0xB30, 1)
    process(0x880, 0x8C0, b"SYSTEM  ", 0xB40, 2, flags=2)
    process(0x8C0, 0x900, b"\xd4EST    ", 0xB50, 3)
    process(0x900, 0, b"NOMEM   ", 0, 4)
    owner(0xB00, 0xB10, 0x320)
    owner(0xB10, 0xB20, 0x320)
    owner(0xB20, 0xB60, 0x330)
    owner(0xB60, 0, 0x330)
    owner(0xB30, 0, 0x340)
    owner(0xB40, 0, 0x350)
    owner(0xB50, 0, 0x360)
    descriptor(0x320, 0, 0x5200, 0x200)
    descriptor(0x330, 0, 0x5300, 0x100)
    descriptor(0x340, 0, 0x5400, 0x80)
    descriptor(0x350, 0, 0x5500, 0x40)
    descriptor(0x360, 0, 0x5600, 0x40)
    if fixture == "no-processes":
        word(SYS_OFFSET + 0x72, 0)
    elif fixture == "empty-group":
        group_a.extend(bytes(0x600 - len(group_a)))
        group_a[0], group_a[1284] = 0, 2
    elif fixture == "both-shared":
        group_b[9] = 1
    elif fixture == "shared-owner-chain":
        owner(0xA00, 0xA20, 0x300)
        owner(0xA20, 0xA30, 0x310)
        owner(0xA30, 0, 0x300)
    elif fixture == "wrap":
        descriptor(0x200, 0x210, 0xFFF0, 0x20)
        descriptor(0x210, 0x220, 0x0010, 0x40)
    elif fixture == "fractional":
        descriptor(0x200, 0x210, 0x1000, 0x20)
        descriptor(0x210, 0x220, 0x1020, 0x40)
        descriptor(0x220, 0, 0x2000, 0x20)
    elif fixture == "overflow":
        descriptor(0x200, 0x210, 0x1000, 0xFFF0)
        descriptor(0x210, 0x220, 0x0FF0, 0x80)
    elif fixture == "reordered":
        descriptor(0x200, 0x210, 0x2000, 0x40)
        descriptor(0x210, 0x220, 0x1000, 0x40)
        descriptor(0x220, 0, 0x1040, 0x80)
    elif fixture == "repeated-memory":
        owner(0xB10, 0xB20, 0x330)
        owner(0xB20, 0xB60, 0x320)
        owner(0xB60, 0, 0x360)
    elif fixture == "console-wrap":
        image[0x840 + 0x20] = 255
    elif fixture != "populated":
        raise ValueError(f"unknown STOP fixture: {fixture}")
    return image, group_a, group_b


def run_case(directory, code, data, case):
    if len(case.tail) > 127 or len(case.input_line) > 126:
        raise ValueError(f"{case.name}: command tail or console input is too long")
    sysdata, group_a, group_b = system_image(case.fixture)
    for name, contents in (
        ("code.bin", code), ("data.bin", data), ("sysdata.bin", sysdata),
        ("group_a.bin", group_a), ("group_b.bin", group_b),
        ("tail.bin", case.tail), ("input.bin", case.input_line),
    ):
        (directory / name).write_bytes(contents)
    definitions = {
        "CODE_SEG": CODE_SEG, "DATA_SEG": DATA_SEG, "SYS_SEG": SYS_SEG,
        "SYS_OFFSET": SYS_OFFSET, "GROUP_A_SEG": GROUP_A_SEG,
        "GROUP_B_SEG": GROUP_B_SEG, "TAIL_LENGTH": len(case.tail),
        "INPUT_LENGTH": len(case.input_line), "VERSION": case.version,
        "ABORT_RESULT": case.abort_result,
    }
    (directory / "case.inc").write_text(
        "".join(f"%define {name} {value}\n" for name, value in definitions.items()),
        encoding="ascii",
    )
    subprocess.run(
        ["nasm", "-f", "bin", str(HERE / "stopcheck.asm"), "-o", "stopcheck.com"],
        cwd=directory, check=True, timeout=10,
    )
    if (directory / "stopcheck.com").stat().st_size > 0xFF00:
        raise ValueError(f"{case.name}: test harness exceeds COM memory")
    result = subprocess.run(
        ["emu2", "stopcheck.com"], cwd=directory, capture_output=True, timeout=10,
    )
    if result.returncode:
        raise RuntimeError(
            f"{case.name}: emulator exited {result.returncode}: "
            f"stdout={result.stdout!r} stderr={result.stderr!r}"
        )
    report, marker, encoded = result.stdout.rpartition(MARKER)
    if not marker or not encoded.endswith(b">"):
        raise RuntimeError(f"{case.name}: missing harness result: {result.stdout!r}")
    try:
        metadata = bytes.fromhex(encoded[:-1].decode("ascii"))
    except (UnicodeDecodeError, ValueError) as error:
        raise RuntimeError(f"{case.name}: malformed harness result {encoded!r}") from error
    if len(metadata) != 17:
        raise RuntimeError(f"{case.name}: unexpected harness result length")
    return report, metadata[0], metadata[1], metadata[2], metadata[3:]


def verify_case(case, result):
    report, exit_function, abort_count, read_count, abort_data = result
    expected_exit = 0 if case.rejected else 143
    expected_reads = int(not case.rejected and not case.tail)
    expected_abort = int(bool(case.abort_pd))
    if exit_function != expected_exit or read_count != expected_reads:
        raise AssertionError(f"{case.name}: unexpected termination or input calls: {result!r}")
    if abort_count != expected_abort:
        raise AssertionError(f"{case.name}: unexpected P_ABORT call count: {abort_count}")
    expected_data = bytes(14)
    if expected_abort:
        expected_data = struct.pack(
            "<HHBB8s", case.abort_pd, 0, case.abort_console, 0, case.abort_name,
        )
    if abort_data != expected_data:
        raise AssertionError(
            f"{case.name}: P_ABORT block {abort_data.hex()} != {expected_data.hex()}"
        )
    expected_report = b"" if case.rejected else case.expected_report
    if expected_reads:
        expected_report += PROMPT
    expected_report += case.message
    if report != expected_report:
        raise AssertionError(
            f"{case.name}: report differs\nexpected: {expected_report!r}\nactual: {report!r}"
        )


def verify_safety_differences(directory, reference, candidate):
    empty = Case(
        "empty-list-positive", fixture="empty", expected_report=EMPTY_REPORT,
        input_line=b"1", message=b"\r\nEntry number too large",
    )
    release = run_case(directory, *reference, empty)
    expected_release = (EMPTY_REPORT + PROMPT, 143, 1, 1, bytes(14))
    if release != expected_release:
        raise AssertionError(f"{empty.name}: unexpected reference behavior: {release!r}")
    verify_case(empty, run_case(directory, *candidate, empty))
    print("PASS empty-list-positive (candidate rejects a zero-PD abort)")

    last = Case(
        "65th-process", fixture="many-rows-65", expected_report=MAX_ROWS_REPORT,
        input_line=b"64", abort_pd=0x1C00, abort_console=64, abort_name=b"PROC0064",
    )
    release = run_case(directory, *reference, last)
    release_block = struct.pack("<HHBB8s", 0x1C00, 0x1C00, 64, 0, b"PROC0064")
    expected_release = (MAX_ROWS_REPORT + PROMPT, 143, 1, 1, release_block)
    if release != expected_release:
        raise AssertionError(f"{last.name}: unexpected reference behavior: {release!r}")
    verify_case(last, run_case(directory, *candidate, last))
    print("PASS 65th-process (candidate avoids the reference's abort-block overlap)")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("candidate", type=Path)
    args = parser.parse_args()
    reference = cmd_groups(args.reference)
    candidate = cmd_groups(args.candidate)
    verify_startup(reference[0], candidate[0])
    with tempfile.TemporaryDirectory(prefix="stop-check-") as temp:
        directory = Path(temp)
        for case in CASES:
            expected = run_case(directory, *reference, case)
            verify_case(case, expected)
            actual = run_case(directory, *candidate, case)
            verify_case(case, actual)
            if actual != expected:
                raise AssertionError(
                    f"{case.name}: candidate differs from reference\n"
                    f"reference: {expected!r}\ncandidate: {actual!r}"
                )
            print(f"PASS {case.name}")
        verify_safety_differences(directory, reference, candidate)
    print(f"STOP: {len(CASES)} equivalent and 2 safety-difference cases passed")


if __name__ == "__main__":
    main()
