/* Native BDOS adapter. The composition root supplies its known DMA
 * buffer; CDOS has no portable get-DMA call. One search or file handle may be
 * active at once. State is caller-owned, never a hidden singleton. One free
 * N:/O:/P: alias is retained for FAT paths and reclaimed at process exit.
 */
#ifndef FM_FSNATIVE_H
#define FM_FSNATIVE_H
#include "location.h"
#include "copy.h"
#include "mutate.h"
struct fm_native_fs {
    int version; /* Captured by native binding; copied to destination state. */
    char *restore_dma;
    char fcb[36];
    char dma[128];
    int active, user, first, position, available;
    int media, effective_drive, alias;
    int identity[4];                        /* Owned alias identity; never overwrite others. */
    unsigned remaining_low, remaining_high; /* FAT byte length, not records. */
};
void fm_native_fs_bind(struct fm_filesystem *, struct fm_native_fs *, char *);
struct fm_native_copy {
    struct fm_native_fs *source;
    struct fm_native_fs destination;
    char name[13], temporary[13], backup[13], original[32], message[80];
    int exists, owned, backed, opened, filled;
    unsigned sequence, bytes_low, bytes_high;
};
void fm_native_copy_bind(struct fm_copy_store *, struct fm_native_copy *, struct fm_native_fs *,
                         char *);

/* Uses the Copy adapter's closed-FCB workspace; actions are serialized. */
struct fm_native_mutation {
    struct fm_native_copy *files;
    int prepared, attributes;
    char replacement[13];
};

void fm_native_mutation_bind(struct fm_mutation_store *port, struct fm_native_mutation *state,
                             struct fm_native_copy *files);

/* Resolve a closed browser location for a child; drive is zero-based. */
int fm_native_run_location(struct fm_native_fs *state, const struct fm_location *location,
                           int *drive, int *user);

/* Scoped physical FAT mapping for historical BACK/REST. The guard is owned
 * by the caller and must be restored even when launch fails. */
struct fm_root_guard {
    int drive, active, sequence;
    char path[FM_PATH_CAPACITY];
};
int fm_native_backup_location(struct fm_native_fs *, const struct fm_location *,
                              struct fm_location *);
int fm_native_root_enter(struct fm_native_fs *, const struct fm_location *, struct fm_root_guard *);
int fm_native_root_leave(struct fm_native_fs *, struct fm_root_guard *);

#endif
