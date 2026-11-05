# XIOS source layout

The three `.a86` files are short assembly entry points. Edit the functional
`.lib` files they include; no source generation or flattening is involved.
RASM86 assembles core.obj, console.obj and devices.obj, which LINK86 combines
into one XIOS component with one loaded segment.

| Object | Maintained includes |
| --- | --- |
| Core | xsystem.lib (header, dispatch, interrupts), xservice.lib (supervisor interface), xrtc.lib, xclock.lib (PC-MODE BIOS clock), xinit.lib |
| Console | xkey.lib, xinput.lib (input, status, control blocks), xconout.lib, xpcmode.lib, egvideo.lib |
| Devices | xdiskirq.lib, xdevice.lib (character devices), xfloppy.lib, xdiskraw.lib, xhard.lib |

coreif.inc, consif.inc and devif.inc declare each object's imports and exports.
shared.inc holds common constants, structure fields and instruction macros.
All includes appear directly in the entry files because RASM86 does not support
nested INCLUDE. Keep their order: it determines the linked layout.

Assembly uses consistent label, operation and operand columns, with CRLF line
endings for RASM86. Preserve spaces inside string operands.

The two flavours select PCMODEXIOS and PCMODEENHANCEDKEYS through the existing
pcmode.lib configuration. Native-only forces both off. Build both with
`make -C kernel/con xios.con xiosnat.con` from the project root.

The builder stages sources in kernel/con/.xios/<variant>, retains logs and use
factors, and checks abi.json and fixed entry jumps before publishing CON/SYM/MAP.
SYS-DATA fields and fixed XIOS header offsets remain unchanged. Code and data
share one loaded segment. HARD8087 prevents emulator substitution; the published
header preserves optional-NDP behaviour because XIOS probes hardware itself.
