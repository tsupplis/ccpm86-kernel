/* Native execution composition. No native state leaks into action policies. */
#ifndef FM_RUNOS_H
#define FM_RUNOS_H

#include "run.h"
#include "fsnative.h"

struct fm_native_run {
    struct fm_native_fs *filesystem;
    const char *failure;
};

/* Return the configured system drive, zero based, or -1 if unavailable. */
int fm_native_system_drive(void);

void fm_native_run_bind(struct fm_executor *port, struct fm_native_run *state,
                        struct fm_native_fs *filesystem);

#endif
