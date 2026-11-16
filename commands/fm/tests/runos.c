/* Native handoff contract, with all operating-system calls injected at link. */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "runos.h"

static int drive, user, segment, offset_set, paused, closed, opened;
static int launch_error, map_error, open_error;
static char dma[128];

int BDOS(int function, ...)
{
    va_list args;
    int value = 0;
    va_start(args, function);
    if (function == 26) {
        assert(va_arg(args, char *) == dma);
        offset_set = 1;
    } else if (function == 9) {
        assert(strstr(va_arg(args, char *), "return to FM"));
        ++paused;
    } else {
        value = va_arg(args, int);
        if (function == 25) value = drive;
        else if (function == 14) drive = value;
        else if (function == 32) {
            if (value == 255) value = user;
            else user = value;
        } else if (function == 51) segment = value;
        else assert(function == 6 && value == 253);
    }
    va_end(args);
    return value;
}
int pcsysdisk(void) { return 3; }
unsigned spawn_data(void) { return 1234; }
int fm_native_run_location(struct fm_native_fs *fs, const struct fm_location *location,
                           int *target_drive, int *target_user)
{
    *target_drive = 13; *target_user = 4;
    return map_error;
}
void fm_native_close(void) { ++closed; }
int fm_native_open(void) { ++opened; return open_error; }
int p_spawn(char *command)
{
    assert(closed == 1 && !opened);
    assert(drive == 13 && user == 4);
    assert(!strcmp(command, "PROBE.CMD Mixed"));
    /* Emulate the translator leaving a different DMA segment. */
    segment = 999;
    return launch_error;
}
int main(void)
{
    struct fm_native_fs fs;
    struct fm_native_run state;
    struct fm_executor executor;
    struct fm_location location;
    int mode, status;

    memset(&fs, 0, sizeof(fs)); fs.restore_dma = dma;
    memset(&location, 0, sizeof(location));
    fm_native_run_bind(&executor, &state, &fs);
    for (mode = 0; mode < 4; ++mode) {
        closed = opened = paused = offset_set = 0;
        drive = 1; user = 2; segment = 1234;
        launch_error = mode == 1; map_error = mode == 2; open_error = mode == 3;
        status = executor.execute(executor.context, &location, "PROBE.CMD Mixed");
        assert((status != 0) == (mode != 0));
        assert(drive == 1 && user == 2 && segment == 1234);
        assert(closed == !map_error && opened == !map_error);
        assert(offset_set == !map_error);
        assert(paused == (!map_error && !launch_error));
        if (status) assert(executor.error(executor.context)[0]);
    }
    puts("PASS: native launch handoff, directory/user/DMA restoration and failure cleanup");
    return 0;
}
