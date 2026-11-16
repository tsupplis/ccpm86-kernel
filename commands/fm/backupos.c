/* BackRest launch scope: native mapping guard around the shared executor. */
#include "backup.h"
#include "fsnative.h"

static int prepare(void *context, const struct fm_location *location, struct fm_location *root)
{
    struct fm_native_backup *state = context;
    return fm_native_backup_location(state->filesystem, location, root);
}

static int execute(void *context, const struct fm_location *root, const char *command)
{
    struct fm_native_backup *state = context;
    struct fm_root_guard guard;
    int status, restored;

    state->failure = "Cannot preserve the hard-disk working directory.";
    status = fm_native_root_enter(state->filesystem, root, &guard);
    if (!status) {
        status = state->executor->execute(state->executor->context, root, command);
        if (status)
            state->failure = state->executor->error(state->executor->context);
    }
    restored = fm_native_root_leave(state->filesystem, &guard);
    if (restored)
        state->failure = "BackRest returned, but the original directory could not be restored.";
    return status || restored ? FM_FS_ERROR : FM_FS_OK;
}

static const char *last_error(void *context)
{
    return ((struct fm_native_backup *)context)->failure;
}

void fm_native_backup_bind(struct fm_backup_host *port, struct fm_native_backup *state,
                           struct fm_native_fs *filesystem, struct fm_executor *executor)
{
    state->filesystem = filesystem;
    state->executor = executor;
    state->failure = "Backup failed.";
    port->context = state;
    port->prepare = prepare;
    port->execute = execute;
    port->error = last_error;
}
