# TYPE

Built from the Concurrent CP/M-86 3.1 PL/M source.

## Syntax

```text
TYPE filespec
TYPE filespec [PAGE]
TYPE filespec [PNN]
```

TYPE displays one text file, ending at Ctrl-Z or BDOS record EOF. Wildcards
are rejected. Passwords may be supplied as `filespec;password` or entered
when prompted. A final line does not need CR/LF; CP/M files have record,
not exact-byte, lengths.

Paging is off by default. `[PAGE]` uses 23 physical screen rows. `[PNN]`
requires exactly two decimal digits from 01 through 23: `[P01]`, `[P10]`,
`[P23]`, for example. Options are case-insensitive and must be trailing.
Unbracketed `P`/`PNN` have been removed. `[P]`, `[P00]`, `[P24]`, `[P99]`,
`[P1]`, malformed brackets and extra arguments are rejected.

Both bracketed forms use the same physical-row counter, including tabs and
wrapping. The prompt is `Press any key to continue...`; input is raw and
non-echoing. Ctrl-C stops output. A pause occurs only when another output
character follows a full page, so EOF at an exact boundary needs no key.
This replaces the old linefeed-only P/PNN mechanism.

`make check-paging` tests the built TYPE on disposable CCP/M QEMU images:
PAGE, numeric limits, case-insensitivity, short/exact/wrapped output,
resume/Ctrl-C, default-off and rejection of old/out-of-range syntax. It also
checks SDIR and both DIR variants. The original parse/open/password behavior
is retained; CDOS-specific adaptations belong to the separate CDOS project.
