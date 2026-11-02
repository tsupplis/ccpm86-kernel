# TYPE

Recompiled from the CCP/M-86 3.1 PL/M source.

Shared source: `ccpm86-kernel` and `cdos41-kernel` carry identical copies
(see `check_shared_commands`). The DOS-media code is inert on CCP/M 3.1,
which is built without PCMODE: it never presents a DOS pseudo-label, never
writes directory-label byte 15, and its block numbers stay below 8000h, so
the FCB disk-map flag tests cannot match.

## Syntax

```text
TYPE filespec[;password]
TYPE filespec [PAGE]
TYPE filespec [PNN]
```

- One file; wildcards are rejected. A missing password is prompted once
  (Backspace, Ctrl-X, Ctrl-C), then the open is retried once.
- Text ends at the first Ctrl-Z or the last record. The final line needs no
  CR/LF. There is deliberately no explicit close (historical behaviour).
- `[PAGE]` pauses after 23 physical rows (tabs and wrapping counted);
  `[PNN]` selects 01..23 rows, exactly two digits. Options are trailing and
  case-insensitive. The prompt reads a key via BDOS 6/FDh; Ctrl-C stops.
  The old unbracketed `P`/`PNN` forms are rejected.

## DOS exact EOF

CP/M files are record based: without Ctrl-Z, the whole last 128-byte record
is shown. On CDOS, BLACK marks DOS FCBs with equal flag bytes 16/17 (bit 80h)
and the exact 32-bit size in bytes 28..31; TYPE then stops at the real DOS
EOF instead of printing the zero padding.

## Verification

PCE regressions against the OEM binaries (`base/`) run from the CDOS
root; CCP/M uses `make check` (structural) only.
`make check-type` covers empty/unterminated files, embedded Ctrl-Z,
record boundaries, passwords, paging/Ctrl-C and an unaligned 65537-byte DOS
file.
