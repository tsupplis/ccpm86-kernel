# Active compiler

Aztec 4.3 driver, CP/M-86 small model, `-Osize`, standard runtime only.
C interfaces use full prototypes; touched modules use ANSI definitions. Assembly uses near returns, BP+4 arguments
and trailing-underscore public symbols. The combined native regression passes.
Non-small models are DOS-only in this driver; CP/M overlays remain an option
if later actions require them. No overlays are currently needed.

# Historical DRC trial

# Medium-model trial

The former active build used DR C 1.11 small model. The medium trial was not adopted.

The added CLEARB/C/CLEARM/CLEARS libraries identify themselves as CLEAR 1.32,
dated 4/05/85. The installed compiler is DR C 1.11. A scratch compilation of
all production C modules with -M -x -q1000 succeeded. Object comments identify
DRCm4417 and request CLEARM automatically.

The scratch assembly adapters were converted to far returns, with stack
arguments moved from BP+4 to BP+6 and subsequent arguments shifted by two
bytes. Medium model retains near data pointers; C callback pointers and calls
must follow the compiler's far-code ABI.

Linking against the supplied local CLEARM.L86 fails with unresolved m.init
(from MAIN.OBJ) and _lln (from VIEW/CONTROL/BROWSER/TYPE). LINK86 still writes a
CMD despite these diagnostics; that output is invalid and was not executed.
No replacement stubs or mixed-version runtime modules were introduced.

Needed: DR C 1.32 compiler passes matching these libraries, or a CLEAR 1.11
medium-model library matching the installed compiler. The installed CLEARL
contains similarly named helpers but is not evidence of a compatible medium
runtime; substituting it without validating its data ABI is not appropriate.

Once matched: rebuild every object, validate the far assembly bridges and
callback dispatch, then run the combined native navigation/Type/Copy test.
A subsequent test must cross a 64 KiB code boundary to establish the benefit
rather than merely demonstrating that a smaller medium-model image boots.
