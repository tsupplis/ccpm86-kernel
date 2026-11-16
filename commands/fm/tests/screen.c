/* Native presentation boundary: one locked transfer, redraw only on change. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "fm.h"

static int held, copies, redraws, changed;
static int version = 0x1441;

int BDOS(int function, ...)
{
    return function == 12 ? version : 0;
}
int XINIT(void) { return 0; }
int XSEG(void) { return 1; }
int FARBYTE(unsigned segment, unsigned offset)
{
    return offset == 101 ? 4 : 0;
}
int FARWORD(unsigned segment, unsigned offset)
{
    if (offset == 224) return 0xb800;
    if (offset == 222) return 0x3000;
    return 0;
}
int XCALL(int function, int bx, int cx, int dx)
{
    if (function == 16) return dx == 255 ? 100 : 200;
    assert(function == 20 && !held);
    ++redraws;
    return 0;
}
int FMLOCK(unsigned segment, unsigned window, int release)
{
    assert(segment == 1 && window == 200);
    assert(held == release);
    held = !release;
    return 1;
}
int FMCOPY(unsigned segment, const char *text, const char *attributes, int count)
{
    assert(held && segment == 0x3000 && count == 1920);
    assert(text && attributes);
    ++copies;
    return changed;
}
int main(void)
{
    static struct fm_canvas canvas;

    assert(!fm_native_open());
    changed = 1920;
    assert(!fm_native_present(&canvas));
    assert(copies == 1 && redraws == 1 && !held);
    changed = 0;
    assert(!fm_native_present(&canvas));
    assert(copies == 2 && redraws == 1 && !held);
    changed = 1;
    assert(!fm_native_present(&canvas));
    assert(copies == 3 && redraws == 2 && !held);
    fm_native_close();
    assert(fm_native_present(&canvas) == FM_OUTPUT_ERROR);
    version = 0x1431;
    assert(!fm_native_open());
    changed = 1;
    assert(!fm_native_present(&canvas));
    assert(copies == 4 && redraws == 3 && !held);
    fm_native_close();
    version = 0x0022;
    assert(fm_native_open() == FM_BAD_MODEL);
    puts("PASS: bulk screen transfer, unchanged-frame suppression and lock release");
    return 0;
}
