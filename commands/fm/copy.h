/* Copy transaction and action contracts. No UI or native calls in the action.
 * prepare pins destination coordinates and checks self-copy/target protection.
 * begin creates an owned temporary; write consumes all bytes or fails.
 * commit publishes only a closed complete file; abort removes only owned work.
 * An existing destination survives cancellation and pre-commit failure.
 */
#ifndef FM_COPY_H
#define FM_COPY_H
#include "location.h"
#include "mutate.h"
/* All callbacks return FM_FS_OK or FM_FS_ERROR. prepare sets exists only on
 * success. A NULL source requests generated-file publication, not a copy.
 * Names and coordinates are borrowed for the duration of the call.
 * write consumes the complete block or fails; it must not publish partial data.
 * commit may leave a named recovery backup if rollback fails. abort is
 * idempotent and must preserve that backup. error returns context-owned text
 * valid until the next store call. No callback draws or reads keyboard input.
 */
struct fm_copy_store {
    void *context;
    int (*prepare)(void *context, FM_CONST struct fm_location *source,
                   FM_CONST struct fm_location *destination, FM_CONST char *name, int *exists);
    int (*begin)(void *context);
    int (*write)(void *context, FM_CONST char *bytes, int count);
    int (*commit)(void *context, int replace);
    int (*abort)(void *context);
    FM_CONST char *(*error)(void *context);
};

/* Caller-owned storage. Fields are private to copy.c after binding; controllers
 * configure through fm_copy_configure and execute through fm_invoker only.
 * The catalog/location are borrowed for the entire invocation; the destination
 * is copied; ordered target indices are borrowed. No native handle escapes the action.
 */
struct fm_copy_job {
    struct fm_filesystem *source;
    struct fm_copy_store *store;
    struct fm_mutation_store *attributes; /* Optional system-copy finalization. */
    FM_CONST struct fm_location *location;
    FM_CONST struct fm_entry *entries;
    struct fm_location destination;
    FM_CONST struct fm_request *request; /* Borrowed until cancel/completion. */
    int count, active, target, stage, opened, handle, replace;
    int buffered, source_end; /* Pending block; EOF may accompany final bytes. */
    unsigned decision;
    char buffer[128], message[80];
};
void fm_copy_bind(struct fm_invoker *port, struct fm_copy_job *job, struct fm_filesystem *source,
                  struct fm_copy_store *store, FM_CONST struct fm_location *location,
                  FM_CONST struct fm_entry *entries);
int fm_copy_destination(struct fm_filesystem *, FM_CONST char *, struct fm_location *);
/* Optional attributes are applied only after successful system-copy publication.
 * Configure while idle. Returns FM_BAD_MODEL for invalid input/active jobs.
 * All bound services and the borrowed source catalog outlive the invocation. */
int fm_copy_configure(struct fm_copy_job *job, int count, FM_CONST char *destination,
                      struct fm_mutation_store *attributes);
#endif
