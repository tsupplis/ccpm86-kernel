/* Read-only text action. No rendering or keyboard dependency.
 * The caller owns state and injects the filesystem. At most one handle is open.
 * begin borrows the port until close; location/name are consumed during open.
 * next replaces the page; close is idempotent and required on cancellation.
 */
#ifndef FM_TYPE_H
#define FM_TYPE_H
#include "location.h"
#define FM_TEXT_ROWS 18
#define FM_TEXT_COLUMNS 76
struct fm_type {
    struct fm_filesystem *filesystem;
    int handle, opened, eof, status;
    int buffered, position, skip_lf, pending_spaces, stream_end, wrapped;
    char buffer[128];
    char lines[FM_TEXT_ROWS][FM_TEXT_COLUMNS + 1];
    int rows;
};
int fm_type_begin(struct fm_type *, struct fm_filesystem *, FM_CONST struct fm_location *,
                  FM_CONST char *);
/* END without new content preserves the last page; begin resets rows. */
int fm_type_next(struct fm_type *);
int fm_type_close(struct fm_type *);
/* Type implements the shared invocation port. Entries/location are borrowed
 * from a stable catalog until cancellation/completion; request indices are
 * borrowed unchanged until completion. Each advance reads at most one page, so replay is
 * cancellable.
 */
struct fm_type_job {
    struct fm_filesystem *filesystem;
    FM_CONST struct fm_location *location;
    FM_CONST struct fm_entry *entries;
    int count, active, target, page_number, goal, loading, ending;
    FM_CONST struct fm_request *request; /* Borrowed until cancel/completion. */
    struct fm_type reader;
    struct fm_text_page page;
    char title[80];
};
void fm_type_job_bind(struct fm_invoker *, struct fm_type_job *, struct fm_filesystem *,
                      FM_CONST struct fm_location *, FM_CONST struct fm_entry *, int);
#endif
