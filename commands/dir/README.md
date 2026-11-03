# DIR

Shared PL/M sources in CCP/M and CDOS. Default DIR omits SYSTEM and FAT
HIDDEN; `[S]` selects only that group. FAT detection precedes inspection
of byte 16 bit 20h. The same source includes validated `[Gn]` user parsing
and paging. No BDOS 230 calls or attribute changes occur.

Internal shell DIR remains a separate implementation; its established
`/S`/`[S]` policy is the same. This directory builds the external DIR.CMD.
