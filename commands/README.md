# Command builds

Each directory builds independently with the installed cross-development tools.
The parent Makefile visits them in `SUBDIRS` order and stops on failure. There
are no build dependencies on another kernel project.

## Makefile conventions

- Declare only the toolchain variables used by the recipes, followed by the
  output list (`TOOLS`) and any module, include or reference lists.
- Keep `all` as the default target. Declare action targets `.PHONY` and use
  `.DELETE_ON_ERROR` so a failed recipe cannot leave a usable-looking output.
- `all` builds local tools. `check` validates CMD structure or runs the stronger
  comparison/runtime checks documented in that directory. `clean` removes
  generated files, using extension wildcards where appropriate; it preserves
  sources, invariant reconstructions, reference binaries and editor data.
- Track included declarations and Makefile changes. Compiler recipes use `$<`
  for the source, rather than passing all prerequisites to the compiler.
- Preserve tool-specific compiler flags, link order, group layout and memory
  allocation. Explain exceptional header processing beside its recipe.
- Keep system composition in `kernel/sysgen` and disk packaging in `media`.
  Command directories must not duplicate those recipes.

Intel PL/M builds compile and link objects, locate the segments, convert them
to H86, then run GENCMD. Allocation values derived from locator maps remain
lazily expanded so the maps exist before they are read. DRI C, PL/I and Aztec
C builds retain their own linkers and runtime conventions.

Assembly reconstructions with frozen originals compare complete CMD files,
including headers and padding. Maintained versions with intentional patches
use their dedicated checks. A `cmdinfo` check alone verifies structure, not
functional equivalence.

Run `make -C commands check` for all command checks. This includes existing
emulator-based checks such as INITDIR; it is broader than the root `make check`.
