/* Console handoff and process launch. The imported launcher is local to FM.
 * All filesystem handles must be closed before handing the console to a child.
 */
#include <stdio.h>
#include "runos.h"
#include "spawn.h"

int BDOS(int function, ...);

int fm_native_system_drive(void)
{
    return pcsysdisk() - 1;
}

static int execute(void *context, const struct fm_location *location, const char *command)
{
    struct fm_native_run *state = context;
    char tail[128];
    int drive, user, old_drive, old_user, status, reopened;

    state->failure = 0;
    if (!command || strlen(command) >= sizeof(tail) ||
        fm_native_run_location(state->filesystem, location, &drive, &user)) {
        state->failure = "Cannot prepare the selected location for execution.";
        return FM_FS_ERROR;
    }
    strcpy(tail, command);
    old_drive = BDOS(25, 0) & 255;
    old_user = BDOS(32, 255) & 255;
    BDOS(14, drive);
    BDOS(32, user);
    fm_native_close();
    status = p_spawn(tail);
    if (!status) {
        BDOS(9, "\r\nPress any key to return to FM.$");
        BDOS(6, 253);
    }
    BDOS(14, old_drive);
    BDOS(32, old_user);
    BDOS(51, spawn_data());
    BDOS(26, state->filesystem->restore_dma);
    reopened = fm_native_open();
    if (reopened)
        state->failure = "Could not restore the FM console.";
    else if (status)
        state->failure = "Program not found or launch failed.";
    return reopened || status ? FM_FS_ERROR : FM_SUCCESS;
}

static const char *last_error(void *context)
{
    struct fm_native_run *state = context;
    return state->failure ? state->failure : "Execution failed.";
}

void fm_native_run_bind(struct fm_executor *port, struct fm_native_run *state,
                        struct fm_native_fs *filesystem)
{
    state->filesystem = filesystem;
    state->failure = 0;
    port->context = state;
    port->execute = execute;
    port->error = last_error;
}
