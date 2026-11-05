# SUBMIT

`SUBMIT name [arguments...]` executes a `.SUB` file. `$1` through `$99`
substitute arguments; `$$` produces a literal dollar sign.

Lines may end with CR, CRLF or LF. Leading/trailing spaces and tabs are removed;
interior spacing is preserved. A final command immediately followed by Ctrl-Z
(or physical EOF) is executed even without a newline. Ctrl-Z ends the script;
bytes after it are ignored.

Empty lines, spaces/tabs-only lines, and lines whose first nonblank character is
`;` are ignored without emitting anything, including CR/LF. Comments may extend
across disk records. There is no inline-comment syntax.

## Echo control

Echo starts ON for each invocation of SUBMIT:

```text
; This line is always silent.
$ECHO OFF
DIR
$ECHO ON
SHOW
```

`$ECHO OFF` suppresses SUBMIT's command echoes and prompts; program output and
error messages remain visible. `$ECHO ON` restores normal echo. The names and
ON/OFF operands are case-insensitive. Spaces or tabs may separate the operand.
An invalid or missing operand aborts with an error.

All `$` directives execute silently, including `$GLOBAL`, `$LOCAL`, `$INCLUDE`,
and `$ECHO` itself. The former global/local/include informational messages are
also suppressed. `$INCLUDE` invokes SUBMIT again, so the child begins with echo
ON; the parent's setting remains its own.

## Build and checks

Run `make -C commands/submit check` from the project root. It assembles SUBMIT
with the normal toolchain and runs the resulting machine code under emu2 with
synthetic BDOS file, parse and CLI services. NASM is used only for the temporary
test driver. These tests cover line endings, trimming, blank/comment silence,
record boundaries, EOF and echo/directive sequencing; they do not launch child
programs or test scheduling on a live kernel.
