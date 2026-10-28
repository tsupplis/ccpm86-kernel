#define dataseg DATASEG
#define xdata XDATA
#define xinit XINIT
#define xcall XCALL
#define farword FARWORD
#define farbyte FARBYTE
#define farput FARPUT
#define bdos BDOS
/* Shared Concurrent CP/M-86 window services. Coordinates exposed to users
 * are one-based; XIOS row/column words use zero-based row in the high byte.
 * ABI reference: kernel/con/xbegin.lib and xconout.lib, VS_* definitions.
 */
typedef unsigned short WORD;
struct window {
    int number, prow, pcol, nrows, ncols, vrow, vcol;
    int tracking, display, foreground, background;
};
#ifndef WIN_IMPLEMENTATION
extern WORD xdata;
#endif
extern int xinit(), xcall(), farword(), farbyte(), farput();
extern int w_init(), w_read(), w_apply(), w_top(), w_full(), w_manager();
extern int w_key(), w_status(), w_write(), w_view(), w_command();
extern int bdos(), message(), conchar();
extern char *colors[];

#ifdef HOST_TEST
#include <stdio.h>
#define format sprintf
#else
extern int format();
#endif
