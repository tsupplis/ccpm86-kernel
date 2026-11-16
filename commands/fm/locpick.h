/* Location picker controller. Borrows filesystem and dialog; owns only a
 * candidate location and one bounded page of labels. No live catalog, marks
 * or action request is changed. Every directory cursor closes before return.
 */
#ifndef FM_LOCPICK_H
#define FM_LOCPICK_H

#include "location.h"

#define FM_PICK_WAIT 0
#define FM_PICK_ACCEPT 1
#define FM_PICK_CANCEL 2
#define FM_PICK_ERROR 3

struct fm_location_picker {
    struct fm_filesystem *filesystem;
    struct fm_location candidate;
    int mode, offset;
    int operation[16];
    char labels[16][32];
    struct fm_location bookmarks[2];
    int bookmark_set[2];
};

/* drives selects the drive chooser; otherwise open candidate's users/folders.
 * Bookmarks are snapshots, independent of subsequent changes by the caller. */
int fm_location_begin(struct fm_location_picker *picker, struct fm_dialog *dialog,
                      struct fm_filesystem *filesystem, const struct fm_location *current,
                      int drives, const struct fm_location *bookmarks, const int *available);
int fm_location_key(struct fm_location_picker *picker, struct fm_dialog *dialog, int key);

#endif
