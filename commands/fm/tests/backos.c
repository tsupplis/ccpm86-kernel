/* Mapping restoration is mandatory even when preparation or launch fails. */
#include <assert.h>
#include <stdio.h>
#include "backup.h"
#include "fsnative.h"

static int enter_error, launch_error, leave_error, entered, launched, left;
int fm_native_backup_location(struct fm_native_fs *fs,
                             const struct fm_location *location,
                             struct fm_location *root)
{
    *root = *location;
    return 0;
}
int fm_native_root_enter(struct fm_native_fs *fs, const struct fm_location *root,
                         struct fm_root_guard *guard)
{
    guard->active = 1;
    ++entered;
    return enter_error;
}
int fm_native_root_leave(struct fm_native_fs *fs, struct fm_root_guard *guard)
{
    assert(guard->active);
    ++left;
    return leave_error;
}
static int execute(void *context, const struct fm_location *root, const char *command)
{
    assert(entered && !left && !strcmp(command, "BACK.CMD CTL=C"));
    ++launched;
    return launch_error;
}
static const char *error(void *context) { return "Launch failed"; }
int main(void)
{
    struct fm_native_backup state;
    struct fm_backup_host port;
    struct fm_executor executor = {0, execute, error};
    struct fm_native_fs fs;
    struct fm_location location;
    int mode, result;

    memset(&fs, 0, sizeof(fs));
    memset(&location, 0, sizeof(location));
    fm_native_backup_bind(&port, &state, &fs, &executor);
    for (mode = 0; mode < 4; ++mode) {
        entered = launched = left = 0;
        enter_error = mode == 1;
        launch_error = mode == 2;
        leave_error = mode == 3;
        result = port.execute(port.context, &location, "BACK.CMD CTL=C");
        assert((result != 0) == (mode != 0));
        assert(entered == 1 && left == 1 && launched == !enter_error);
        if (mode) assert(port.error(port.context)[0]);
    }
    puts("PASS: Backup mapping restoration after preparation, launch and restoration failures");
    return 0;
}
