# FUNCTION reconstruction

`function.a86` reconstructs the Concurrent CP/M-86 3.1 version 2.0 binary in
`../../base/function.cmd`. `funcorg.a86` is its invariant reconstruction; both
sources initially produce the original executable exactly. The earlier CP/M-86
source remains available in Git history.

Build and verify with:

```sh
make -C commands/function check
```

The check compares both complete executables with the reference, including
group descriptors, reserved storage and file padding. The normal command build
also builds FUNCTION, and the maintenance disk takes the maintained executable.
ASM86 and GENCMD preserve the original group lengths. The build only compacts
GENCMD's empty EXTRA descriptor slot so STACK follows DATA in the CMD header.

The two historical versions share their purpose and several routines' roles:

| Area | Earlier CP/M-86 version | Concurrent CP/M-86 version |
| --- | --- | --- |
| Interactive menu | Choose a key; ESC changes sets; Return exits | Same interaction, with three pages |
| Definitions | Ten function keys and ten keypad keys, 20 bytes each | Ten function keys of 20 bytes; forty keypad/Alt/Shift/Ctrl keys of 4 bytes |
| Key storage | Fixed system-data offset obtained through BDOS 49 | Per-console XIOS tables located through SYSDAT and XIOS function 16 |
| Keyboard | IBM BIOS INT 16h | Raw BDOS 6 input, with key expansion disabled |
| Screen | Character output plus BIOS window clearing | Counted BDOS 111 blocks and XIOS escapes |
| File input | Single-record reads through the base-page DMA buffer | Eight-record reads into a separate 1K buffer |
| Editing | Applies each definition through an ESC-colon sequence | Edits local rows and copies the tables back on Return |
| Memory model | One group, entry at 100h | Separate CODE, DATA and STACK groups, entry at zero |

The reconstruction names control-flow targets, data pointers, descriptor fields
and BDOS selectors. Comments cover the editor, hexadecimal escapes, file parser,
XIOS table copies and unused shared-library helpers. Instructions use mnemonics;
`db` declarations describe strings and other data.

Original behavior is preserved, including the exact 1431h version check, the
four-local-console restriction in interactive mode, the file parser's paired
asterisk comments, and the inclusive buffer-boundary comparison in `nextch`.
Byte identity verifies the reconstruction; it is not an emulator interaction test.
