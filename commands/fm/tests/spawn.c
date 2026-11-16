/* Launcher routing: TMP must never enter the optional PCTMP gateway. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "spawn.h"

static int translator, native_calls, dos_calls, clean_calls, attached;
static int current_user = 4, priority = 100;

void fcbinit(char *name, char *fcb)
{
    const char *dot = strchr(name, '.');
    memset(fcb, 0, 36);
    memset(fcb + 1, ' ', 11);
    if (dot)
        memcpy(fcb + 9, dot + 1, 3);
}
int BDOS(int function, ...)
{
    va_list args;
    int value;
    va_start(args, function);
    if (function == 32) {
        value = va_arg(args, int);
        if (value != 255) current_user = value;
        value = current_user;
    } else {
        assert(function == 15 || function == 16);
        value = 0;
    }
    va_end(args);
    return value;
}
int pcorder(char *order)
{
    if (!translator) return -1;
    memcpy(order, "\1CMDBATCOMEXE", 13);
    return 0;
}
int pcsysdisk(void) { return 1; }
int getpriority(void) { return priority; }
int getppriority(void) { return 100; }
int p_priority(int value) { priority = value; return 0; }
int conattach(void) { ++attached; return 0; }
int p_cli(char *command) { ++native_calls; return 0; }
int ms_p_cli(char *fcb, char *command)
{ assert(translator); ++dos_calls; return 0; }
int pcclean(void) { assert(translator); ++clean_calls; return 0; }

int main(void)
{
    assert(!p_spawn("DREDIX.CMD TEST.TXT"));
    assert(native_calls == 1 && !dos_calls && !clean_calls && priority == 100);
    translator = 1;
    assert(!p_spawn("DREDIX.CMD TEST.TXT"));
    assert(native_calls == 2 && !dos_calls);
    assert(!p_spawn("PROBE.EXE"));
    assert(dos_calls == 1 && clean_calls == 1 && attached == 3);
    assert(current_user == 4 && priority == 100);
    puts("PASS: TMP native launch and PCTMP native/DOS routing, cleanup and priority restoration");
    return 0;
}
