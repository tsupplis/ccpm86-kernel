/* External actions share one job and one injected, synchronous executor.
 * The executor borrows location/command until return and restores its OS/UI
 * context on every return path. Its result is launch status, not child status.
 */
#ifndef FM_RUN_H
#define FM_RUN_H

#include "location.h"

struct fm_executor {
    void *context;
    int (*execute)(void *context, const struct fm_location *location, const char *command);
    const char *(*error)(void *context);
};

struct fm_run_job {
    struct fm_executor *executor;
    const struct fm_location *location;
    const struct fm_request *request;
    const struct fm_entry *entries;
    int count, active, asking, followup;
    unsigned decision;
    const char *prompt;
    char prefix[32], command[128];
};

/* Returns -1 for an unsupported action, otherwise its file selection policy. */
int fm_run_policy(int command);
/* Configure only while idle. Catalog is borrowed until cancel/completion.
 * Invalid input leaves the job unchanged; returns FM_SUCCESS/FM_BAD_MODEL. */
int fm_run_configure(struct fm_run_job *, const struct fm_entry *, int);

void fm_run_bind(struct fm_invoker *port, struct fm_run_job *job, struct fm_executor *executor,
                 const struct fm_location *location);

#endif
