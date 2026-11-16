"""Check compiled module dependencies; no emulator or repository outputs.

This checks direct symbol dependencies, not callback implementations or C type
ownership. Those remain part of the documented manual phase review.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ALLOWED = {
    "browser": {"locpick", "control", "copy", "backup", "mutate", "run", "print", "metadata", "model", "type", "ui"},
    "locpick": {"model"},
    "control": {"model", "ui"},
    "view": {"model"},
    "native": {"keys"},
    "main": {"browser", "canvas", "config", "control", "copy", "fsnative",
             "native", "runos", "backupos", "ui", "view"},
    "backup": {"mutate"}, "backupos": {"fsnative"},
    "model": set(), "canvas": set(), "copy": set(), "type": set(),
    "config": set(), "keys": set(), "metadata": set(), "ui": {"model"},
    "fsnative": {"mutate"}, "mutate": set(), "run": set(), "print": set(),
    "runos": {"fsnative", "native", "spawn"}, "spawn": set(),
}
OS_SYMBOLS = {"BDOS", "XINIT", "XCALL", "FARWORD", "FARBYTE", "FARPUT",
              "FMLOCK", "FMCOPY", "FMLOC", "XDATA", "XSEG"}


def validate(graph):
    for module, dependencies in graph.items():
        unexpected = dependencies - ALLOWED[module]
        if unexpected:
            raise AssertionError(f"{module}: forbidden dependencies {sorted(unexpected)}")
    active, finished = set(), set()

    def visit(module):
        if module in active:
            raise AssertionError(f"dependency cycle through {module}")
        if module in finished:
            return
        active.add(module)
        for dependency in graph[module]:
            visit(dependency)
        active.remove(module)
        finished.add(module)

    for module in graph:
        visit(module)


def main():
    sources = {path.stem: path for path in ROOT.glob("*.c")}
    # Controllers configure jobs through public contracts, never private fields.
    assert not re.search(r"browser->\w+_job\s*\.",
                         sources["browser"].read_text()), "Browser accesses job internals"
    assert sources.keys() == ALLOWED.keys(), "Review dependency policy for changed modules"
    owners, references = {}, {}
    compiler = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="fm-arch-") as folder:
        for module, source in sources.items():
            obj = Path(folder) / (module + ".o")
            subprocess.run(compiler + ["-DHOST_TEST", "-std=c89",
                           "-Werror", "-Wstrict-prototypes", "-Wold-style-definition", "-c", str(source),
                           "-o", str(obj)], check=True, capture_output=True)
            references[module] = set()
            for line in subprocess.check_output(["nm", "-g", str(obj)], text=True).splitlines():
                fields = line.split()
                if len(fields) < 2:
                    continue
                kind, symbol = fields[-2:]
                symbol = symbol.lstrip("_")
                if kind == "U":
                    references[module].add(symbol)
                elif kind in {"T", "D", "B", "R", "S", "C"}:
                    if symbol == "main": continue
                    assert symbol not in owners, f"duplicate definition: {symbol}"
                    owners[symbol] = module
    graph = {}
    for module, symbols in references.items():
        if module not in {"main", "native", "fsnative", "runos", "spawn"}:
            assert not symbols & OS_SYMBOLS, f"{module}: direct OS dependency"
        graph[module] = {owners[symbol] for symbol in symbols
                         if symbol in owners and owners[symbol] != module}
    validate(graph)
    # Prove the policy actually rejects an otherwise linkable view/controller edge.
    bad = {module: set(edges) for module, edges in graph.items()}
    bad["view"].add("control")
    try:
        validate(bad)
    except AssertionError:
        pass
    else:
        raise AssertionError("dependency guard failed to reject a forbidden edge")
    print("PASS: compiled dependency boundaries, portable OS isolation and no direct cycles")


if __name__ == "__main__":
    main()
