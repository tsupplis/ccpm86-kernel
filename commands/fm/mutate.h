/* File-name mutations. The adapter owns OS state; the job owns confirmation.
 * prepare is read-only. commit rechecks the prepared file and never overwrites
 * a rename target. Cancellation prevents further mutations, not completed ones.
 */
#ifndef FM_MUTATE_H
#define FM_MUTATE_H

#include "location.h"

struct fm_mutation_store {
    void *context;
    int (*prepare)(void *context, const struct fm_location *location, const char *name,
                   const char *replacement);
    int (*commit)(void *context);
    const char *(*error)(void *context);
    /* Optional attribute operation; preserves unrelated flags. commit uses
     * the same revalidation contract as rename/delete, including RO files. */
    int (*prepare_attributes)(void *, const struct fm_location *, const char *, int system);
};

struct fm_mutation_job {
    struct fm_mutation_store *store;
    const struct fm_location *location;
    const struct fm_entry *entries;
    const struct fm_request *request;
    int count, active, target, prepared, asking;
    unsigned decision;
    char replacement[13], message[80];
};

int fm_mutation_name(const char *name);
/* Configure only while idle. Catalog is borrowed until cancel/completion.
 * Invalid input leaves the job unchanged; returns FM_SUCCESS/FM_BAD_MODEL. */
int fm_mutation_configure(struct fm_mutation_job *, const struct fm_entry *, int);

void fm_mutation_bind(struct fm_invoker *port, struct fm_mutation_job *job,
                      struct fm_mutation_store *store, const struct fm_location *location);

#endif
