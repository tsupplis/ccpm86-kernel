# BACK/REST acceptance — 12 November 2026

Both tests used native FM with original BACK.CMD/REST.CMD and disposable disks.
BACK completed; all four source files were deleted before REST was started.
Host extraction compared the restored content against saved originals.

| File | Bytes | FAT C:\FMTEST | CP/M D: user 15 |
|---|---:|---|---|
| EMPTY.TST | 0 | Identical | Identical |
| TEXT.TST | 26 | Identical | Identical |
| ODD.TST | 513 | Identical | Identical |
| LARGE.TST | 8193 | Identical | Identical |

The test driver required corrections for focus, single-key REST choices and
REST returning to its menu. These were automation failures, not archive failures.
Final comparisons were performed separately after REST returned to its menu;
the retained driver includes the corrected menu exit. No FM code change was
needed. All test emulators are bounded and close on success or exception.

Scope: single-volume backup/restore, exact contents and lengths, FAT subdirectory
and nonzero CP/M user area. Multi-volume splitting, interrupted archives and
media-error recovery are not covered.
