/* Browser controller. Owns catalog and displayed text borrowed by models.
 * Filesystem and action services are injected. The composition root owns this state for the
 * session. */
#ifndef FM_BROWSER_H
#define FM_BROWSER_H
#include "type.h"
#include "locpick.h"
#include "copy.h"
#include "mutate.h"
#include "run.h"
#include "print.h"
#include "backup.h"
#define FM_DETAIL_ROWS (FM_ROWS - 4) /* Upper bound for any valid Files pane. */

/* Allocator is injected; failure returns NULL and never consumes old storage. */
struct fm_memory {
    void *context;
    void *(*allocate)(void *, unsigned);
    void (*release)(void *, void *);
};

struct fm_browser {
    struct fm_filesystem *filesystem;
    /* Injected ports retained by the controller; job state stays private. */
    struct fm_copy_store *copy_store;
    struct fm_mutation_store *mutation_store;
    struct fm_action_handler actions;
    struct fm_location location;
    struct fm_location slots[2];
    int slot_set[2], slot_pending, subset_pending;
    char details[FM_DETAIL_ROWS][FM_DETAIL_CAPACITY]; /* Visible rows only, not 1024 strings. */
    int detail_first;
    unsigned detail_generation;
    char subset[13]; /* CP/M 8.3 mask; files only. */
    struct fm_memory *memory;
    void *catalog_storage;
    int capacity;
    struct fm_entry *entries;
    struct fm_file *files;
    struct fm_type_job type_job;
    struct fm_invoker type_service, copy_service;
    struct fm_invoker *active_service;
    struct fm_copy_job copy_job;
    int copy_enabled, copy_pending;
    struct fm_mutation_job mutation_job;
    struct fm_invoker mutation_service;
    int mutation_enabled, mutation_refresh;
    struct fm_run_job run_job;
    struct fm_invoker run_service;
    int run_enabled;
    struct fm_print_job print_job;
    struct fm_invoker print_service;
    int print_enabled, backup_enabled;
    struct fm_backup_job backup_job;
    struct fm_invoker backup_service;
    int setup_menu, system_drive;
    int picking;
    struct fm_location_picker picker;
    char location_text[FM_PATH_CAPACITY + 24];
    char capacity_text[16];
    char selection_text[80];
    char copy_title[44];
};

/* Open initializes fresh or previously closed state. Displayed entries/text
 * borrow browser storage, so browser must outlive the model using them.
 * Recoverable listing errors appear in the model; invalid setup returns ERROR.
 * Close releases the viewer handle before state is reused or discarded. */
int fm_browser_open(struct fm_browser *, struct fm_model *, struct fm_filesystem *,
                    struct fm_memory *, int, int);
int fm_browser_key(struct fm_browser *, struct fm_model *, struct fm_invoker *, int);
void fm_browser_copy_bind(struct fm_browser *, struct fm_copy_store *);
int fm_browser_close(struct fm_browser *);

void fm_browser_mutation_bind(struct fm_browser *browser, struct fm_mutation_store *store);
void fm_browser_run_bind(struct fm_browser *browser, struct fm_executor *executor);
void fm_browser_print_bind(struct fm_browser *browser, struct fm_printer *printer);
void fm_browser_backup_bind(struct fm_browser *, struct fm_backup_host *);
#endif
