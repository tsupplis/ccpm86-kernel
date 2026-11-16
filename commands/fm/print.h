/* Direct printing: a portable job over injected files and a printer port.
 * Printer callbacks return FM_FS_OK/ERROR. attach must not wait for ownership;
 * detach is called exactly once after a successful attach, even on cancellation.
 * put may wait for the physical printer. It never reads console input.
 */
#ifndef FM_PRINT_H
#define FM_PRINT_H

#include "location.h"

struct fm_printer {
    void *context;
    int (*attach)(void *context);
    int (*put)(void *context, int byte);
    int (*detach)(void *context);
};

struct fm_print_job {
    struct fm_filesystem *filesystem;
    struct fm_printer *printer;
    const struct fm_location *location;
    const struct fm_entry *entries;
    const struct fm_request *request;
    int count, active, target, attached, opened, handle, column;
    const char *failure;
    char buffer[128], message[80];
};

/* Configure only while idle. Catalog is borrowed until cancel/completion.
 * Invalid input leaves the job unchanged; returns FM_SUCCESS/FM_BAD_MODEL. */
int fm_print_configure(struct fm_print_job *, const struct fm_entry *, int);

void fm_print_bind(struct fm_invoker *port, struct fm_print_job *job,
                   struct fm_filesystem *filesystem, struct fm_printer *printer,
                   const struct fm_location *location);
void fm_native_printer_bind(struct fm_printer *printer);

#endif
