/* Backup orchestration. BACK/REST own archive I/O; FM owns the compatible
 * control file and confirmation. Native mapping details stay behind host.
 */
#ifndef FM_BACKUP_H
#define FM_BACKUP_H

#include "copy.h"
#include "run.h"

struct fm_backup_host {
    void *context;
    /* Read-only: resolve a hard disk to its physical root/user zero. */
    int (*prepare)(void *, const struct fm_location *, struct fm_location *root);
    /* Run through the shared executor, restoring physical FAT mappings. */
    int (*execute)(void *, const struct fm_location *root, const char *command);
    const char *(*error)(void *);
};

struct fm_backup_job {
    struct fm_backup_host *host;
    struct fm_copy_store *store;
    const struct fm_location *location;
    const struct fm_entry *entries;
    const struct fm_request *request;
    struct fm_location root;
    int count, active, stage, destination, whole, restore, exists, line, target;
    unsigned decision;
    const char *failure;
    char control[13], command[32], text[180];
};

/* Configure only while idle. Catalog is borrowed until cancel/completion.
 * Invalid input leaves the job unchanged; returns FM_SUCCESS/FM_BAD_MODEL. */
int fm_backup_configure(struct fm_backup_job *, const struct fm_entry *, int);

void fm_backup_bind(struct fm_invoker *, struct fm_backup_job *, struct fm_backup_host *,
                    struct fm_copy_store *, const struct fm_location *);

/* Native composition uses the same launcher and transactional file store. */
struct fm_native_fs;
struct fm_native_backup {
    struct fm_native_fs *filesystem;
    struct fm_executor *executor;
    const char *failure;
};
void fm_native_backup_bind(struct fm_backup_host *, struct fm_native_backup *,
                           struct fm_native_fs *, struct fm_executor *);

#endif
