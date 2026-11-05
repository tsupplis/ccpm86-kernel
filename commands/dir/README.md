# DIR

Shared PL/M sources in CCP/M and CDOS. Default DIR omits SYSTEM and FAT
HIDDEN; `[S]` selects only that group. FAT detection precedes inspection
of byte 16 bit 20h. No BDOS 230 calls or attribute changes occur.

## Options

`DIR {filespec ...} [options]` and explicit `DIR.CMD` accept:

| Option | Meaning |
|---|---|
| P | Pause after 23 physical screen rows; Ctrl-C stops |
| S | Show system files only; CDOS FAT also includes hidden files |
| Gn | Select CP/M user n, 0..15 |

Letters can be compact, space/comma separated, or in separate bracket groups.
`[SP]`, `[PSG0]`, `[G0SP]`, `[S,P,G0]` and `[S] [P] [G0]` are valid.
The final closing bracket can be omitted, as in `[SP` or `[PG0`.
Missing/invalid user numbers, unknown letters and malformed tokens are rejected.

CCP/M DIR.RSP uses the same option decoder and system-only selection policy.
CDOS shell/BATCH DIR has its own parser and additional `/L`, `/W`, `/S`, `/P`
and `/Gn` spellings; it also accepts compact bracket flags after a user number.
This directory builds the standalone native DIR.CMD.

## Verification

`python3 -B tools/option_combinations_check.py` from either project root boots
its own kernel and commands in PCE with disposable CP/M disks. It checks compact
and separated combinations, option order, user/system selection, rejection of
invalid input, paging and reset after Ctrl-C. The same run checks SDIR keyword
combinations and TYPE's password-tail plus PAGE/PNN syntax. TYPE has one paging
setting; two simultaneous paging options are deliberately rejected.
