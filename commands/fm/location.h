/* Read-only filesystem port for the FM browser and Type action.
 *
 * Coordinates are explicit: a drive is 0..15, a CP/M user is 0..15, and
 * a FAT path is absolute within its drive. Adapters must restore process
 * drive/user/DMA state before returning. Private owned FAT aliases may persist
 * until process exit; existing mappings must not be replaced. No adapter may draw or read keys.
 *
 * The caller owns all buffers. No pointer into adapter scratch storage may
 * escape a call. A listing is complete only when list_next returns FM_FS_END.
 */
#ifndef FM_LOCATION_H
#define FM_LOCATION_H
#include "fm.h"

#define FM_PATH_CAPACITY 128
#define FM_NAME_CAPACITY 13
#define FM_MEDIA_CPM 0
#define FM_MEDIA_FAT 1
#define FM_FS_OK 0
#define FM_FS_END 1
#define FM_FS_ERROR 2
#define FM_FS_LIMIT 3

struct fm_location {
    int drive;
    int media;
    int user;
    char path[FM_PATH_CAPACITY];
};

struct fm_entry {
    char name[FM_NAME_CAPACITY];
    int directory;
};

/* Cursors and file handles are opaque to controllers. They belong to one
 * adapter context and are valid until close or adapter teardown. Every
 * successful open must be paired with close, including cancellation/errors.
 */
/* Metadata is optional and independent of directory enumeration. Size uses
 * two 16-bit words; dates are CP/M days since 1978-01-01, with decoded time.
 * Unknown values must clear their validity flags, never invent zero dates. */
struct fm_metadata {
    unsigned size_low, size_high, days;
    int size_known, date_known, hour, minute;
};
void fm_metadata_format(FM_CONST struct fm_metadata *, char *);
#define FM_DETAIL_CAPACITY 40

struct fm_filesystem {
    void *context;
    int (*drives)(void *, unsigned *);
    int (*probe)(void *, int, int *);
    int (*list_open)(void *, FM_CONST struct fm_location *, int *);
    int (*list_next)(void *, int, struct fm_entry *);
    int (*list_close)(void *, int);
    int (*file_open)(void *, FM_CONST struct fm_location *, FM_CONST char *, int *);
    int (*file_read)(void *, int, char *, int, int *);
    int (*file_close)(void *, int);
    /* Optional; called only after listing closes and no file handle is active.
     * Fills caller-owned metadata; restores process drive/user/DMA. */
    int (*metadata)(void *, FM_CONST struct fm_location *, FM_CONST char *, struct fm_metadata *);
    /* Optional free space in KiB, for the whole volume (not a user area).
     * Called with no active handle; restores process state on failure too. */
    int (*space)(void *, int drive, unsigned long *kib);
};

/* file_read returns bytes through count: zero with FM_FS_END is normal EOF.
 * Short reads are valid. Errors never imply EOF. Text interpretation (CR/LF,
 * tabs, CP/M Ctrl-Z) belongs to Type, not this byte-reading interface.
 * Paths/names exceeding capacity are rejected, never silently truncated.
 */
#endif
