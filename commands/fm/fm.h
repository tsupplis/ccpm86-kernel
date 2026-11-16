/* FM: model ownership and interface contracts.
 *
 * Lists and text are immutable, borrowed for the application's lifetime.
 * Each view borrows exactly one model owned by the application. Controllers alone change focus,
 * cursor, viewport and marks. Renderers have no application/backend dependency.
 * Native and host builds enforce the same typed interface contracts.
 */
#ifndef FM_H
#define FM_H
#include <string.h>
#define FM_CONST const

#define FM_INPUT_CAPACITY 130 /* Drive prefix + 127-byte FAT path + NUL. */
#define FM_COLUMNS 80
#define FM_ROWS 24
/* Bounded near-data catalog: explicit overflow, never silent truncation. */
#define FM_MAX_FILES 1024
#define FM_NO_SELECTION (-1)
#define FM_TARGET_NONE 0
#define FM_TARGET_ONE 1
#define FM_TARGET_MANY 2
#define FM_NO_TARGETS 4
#define FM_TOO_MANY_TARGETS 5
#define FM_SUCCESS 0
#define FM_BAD_MODEL 1
#define FM_OUTPUT_ERROR 2
#define FM_FILES_FOCUS 0
#define FM_ACTIONS_FOCUS 1
#define FM_KEY_TAB 9
#define FM_KEY_ENTER 13
#define FM_KEY_ESCAPE 27
#define FM_KEY_UP 256
#define FM_KEY_DOWN 257
#define FM_KEY_LEFT 258
#define FM_KEY_RIGHT 259
#define FM_FEATURE_BACKUP 1
#define FM_FEATURE_SETUP_ARGS 2
#define FM_FEATURE_ALL (FM_FEATURE_BACKUP | FM_FEATURE_SETUP_ARGS)
#define FM_MENU_CAPACITY 17
#define FM_CMD_HELP 1
#define FM_CMD_DIRECTORY 2
#define FM_CMD_SUBSET 3
#define FM_CMD_DRIVE 4
#define FM_CMD_TYPE 5
#define FM_CMD_PRINT 6
#define FM_CMD_COPY 7
#define FM_CMD_RENAME 8
#define FM_CMD_DELETE 9
#define FM_CMD_BACKUP 10
#define FM_CMD_EDIT 11
#define FM_CMD_RUN 12
#define FM_CMD_COPY_DISK 13
#define FM_CMD_FORMAT_DISK 14
#define FM_CMD_MEMORY 15
#define FM_CMD_DETAILS 16
#define FM_CMD_SETUP 17
#define FM_CMD_CLEAR 18
#define FM_CMD_BOOKMARK1 19
#define FM_CMD_BOOKMARK2 20
#define FM_CMD_COMMAND 21
#define FM_CMD_OPEN 22
#define FM_CMD_SYSTEM_COPY 23
#define FM_CMD_SYSTEM_ATTR 24
#define FM_CMD_LOCAL_ATTR 25
#define FM_CMD_LOAD_KEYS 26
#define FM_CMD_ASSIGN_KEYS 27
#define FM_CMD_SERIAL 28
#define FM_CMD_DEFAULTS 29
#define FM_CMD_MEMORY_DISK 30
#define FM_CMD_BOOT_DISK 31
#define FM_CMD_HARD_MAINT 32
#define FM_CMD_FLOPPY_MAINT 33
#define FM_KEY_HOME 260
#define FM_KEY_END 261
#define FM_KEY_PGUP 262
#define FM_KEY_PGDN 263
#define FM_KEY_INSERT 264
#define FM_KEY_DELETE 265
#define FM_KEY_F1 270
#define FM_KEY_TICK 300
/* PC-console byte to semantic key; unknown bytes pass through unchanged. */
int fm_key_decode(int);
#define FM_DIALOG_NONE 0
#define FM_DIALOG_INFO 1
#define FM_DIALOG_PICK 2
#define FM_DIALOG_INPUT 3
#define FM_DIALOG_CONFIRM 4
#define FM_DIALOG_PROGRESS 5
#define FM_DIALOG_TEXT 6
#define FM_PURPOSE_SUBSET 3
#define FM_PURPOSE_COMMAND 4
#define FM_PURPOSE_COPY_DESTINATION 6
#define FM_PURPOSE_DIRECTORY_SLOT 7
#define FM_PURPOSE_RENAME 8
#define FM_JOB_PROGRESS 1
#define FM_JOB_DECISION 2
#define FM_JOB_COMPLETE 3
#define FM_JOB_CANCELLED 4
#define FM_JOB_FAILED 5
#define FM_JOB_VIEW 6
#define FM_JOB_INPUT 7
#define FM_JOB_ERROR 3
#define FM_REPLY_NONE 0
#define FM_REPLY_ACCEPT 1
#define FM_REPLY_NEXT 2
#define FM_REPLY_PREVIOUS 3
#define FM_REPLY_FIRST 4
#define FM_REPLY_LAST 5
#define FM_REPLY_TEXT 6

struct fm_rect {
    int x, y, width, height;
};

struct fm_style {
    int normal, selected, marked, accent, border;
};

struct fm_panel {
    struct fm_rect rect;
    struct fm_style style;
};

struct fm_cursor {
    int selected; /* Index in the full list, or FM_NO_SELECTION. */
    int first;    /* Index of first visible item. */
};

struct fm_device_model {
    struct fm_panel panel;
    int media_width;
    FM_CONST char *media;
    FM_CONST char *capacity;
    FM_CONST char *location;
};

struct fm_file {
    FM_CONST char *name;
    FM_CONST char *detail; /* Preformatted metadata; renderer does no I/O. */
    int directory;         /* Presentation marker; never inferred from a name. */
};

struct fm_files_model {
    struct fm_panel panel;
    FM_CONST struct fm_file *items;
    int count;
    /* Certificate for immutable item names; revoke before replacing contents.
     * Unpublished standalone models retain full validation on every call. */
    FM_CONST struct fm_file *validated_items;
    int validated_count;
    int columns;
    int normal_columns; /* Layout-selected columns without metadata. */
    struct fm_cursor cursor;
    unsigned char marks[(FM_MAX_FILES + 7) / 8]; /* Private to mark accessors. */
    int focused;
    int details;
    FM_CONST char *heading;
    FM_CONST char *empty_text;
};

struct fm_command {
    int id; /* Stable identifier, never a menu position. */
    FM_CONST char *label;
    FM_CONST char *help;
    int targets; /* NONE, ONE or MANY; independent of menu order. */
};

struct fm_actions_model {
    struct fm_panel panel;
    int columns; /* Grid columns; height 1 is a borderless strip. */
    FM_CONST struct fm_command *items;
    int count;
    struct fm_cursor cursor;
    int focused;
    int label_command; /* Optional per-model label override. */
    const char *label;
    FM_CONST char *empty_text;
};

struct fm_hint {
    int column, row;
    FM_CONST char *text;
};

struct fm_help_model {
    struct fm_panel panel;
    FM_CONST char *status;
    FM_CONST struct fm_hint *hints;
    int count;
};

/* Dialogs overlay the four persistent views; they never own an input loop. */
struct fm_dialog {
    struct fm_panel panel;
    int kind, purpose, selected, first, count, position;
    const char *footer[2]; /* Optional controls, separate from list entries. */
    int more_before, more_after;
    unsigned generation; /* Listing captured when a Copy picker opens. */
    FM_CONST char *title;
    FM_CONST char *options[16];
    int page_count;
    FM_CONST char *page_lines[18];
    char text[80];
    char input[FM_INPUT_CAPACITY];
};

/* The token identifies one invocation; generation identifies its source list.
 * completed is a successful prefix of request.files, failed is the next prefix;
 * the remaining suffix is unprocessed. Counts are retained on cancellation.
 * A decision token is nonzero only while awaiting that specific reply.
 */
/* Borrowed page data stays valid until the next service call. No UI model or
 * coordinates cross the service boundary. The controller owns presentation. */
struct fm_text_page {
    FM_CONST char *title;
    FM_CONST char *hint;
    FM_CONST char *lines[18];
    int count;
};

struct fm_job_result {
    unsigned token, generation, decision;
    int state, completed, failed, total;
    FM_CONST struct fm_text_page *page;
    FM_CONST char
        *prompt; /* Decision text or optional progress/result detail; borrowed until advance. */
};

struct fm_reply {
    unsigned token, decision;
    int answer;
    const char *text; /* Borrowed only during advance; supplied for FM_REPLY_TEXT. */
};

/* Cooperative action invocation boundary. The application owns the request
 * and keeps it immutable until completion or cancellation. Services may borrow
 * it for that interval; they must not retain it afterwards. File indices refer
 * to its listing generation. advance never waits for input. cancel releases
 * resources and ends the borrow. Page data is borrowed until the next call.
 */
struct fm_request {
    int command;
    unsigned token, generation;
    int count;
    int files[FM_MAX_FILES];
    int destination;
};

struct fm_model {
    unsigned features; /* Injected at startup; never inferred by generic UI code. */
    struct fm_command menu_items[FM_MENU_CAPACITY];
    struct fm_device_model device;
    struct fm_files_model files;
    struct fm_actions_model actions;
    struct fm_help_model help;
    unsigned generation;
    int exit_requested;
    struct fm_dialog dialog;
    FM_CONST struct fm_file *catalog;
    int catalog_count;
    struct fm_request pending; /* Target snapshot; only parameters change before invoke. */
    int job_active;
    unsigned next_job_token;
    struct fm_job_result job;
    int last_command;
    char status[80];
};

/* The display borrows each counted text run only during write(). It returns
 * FM_SUCCESS or FM_OUTPUT_ERROR. Coordinates and length are validated/clipped
 * by the renderer. begin/end delimit a complete frame; end also follows errors.
 */
struct fm_display {
    void *context;
    int (*begin)(void *);
    int (*write)(void *, int, int, int, FM_CONST char *, int);
    int (*end)(void *);
};

struct fm_invoker {
    void *context;
    int (*invoke)(void *, FM_CONST struct fm_request *);
    int (*advance)(void *, FM_CONST struct fm_reply *, struct fm_job_result *);
    int (*cancel)(void *);
    FM_CONST char *(*error)(void *); /* Optional cleanup detail. */
    int interactive;                 /* Viewer completion/cancel returns directly to browser. */
};

/* Pure checks shared by controllers and views; no rendering dependency. */
int fm_panel_valid(FM_CONST struct fm_panel *);
int fm_device_valid(FM_CONST struct fm_device_model *);
/* Marks are bounded by the current catalog. Invalid reads return false;
 * invalid writes fail without mutation. Clear also resets unused capacity. */
int fm_marked(const struct fm_files_model *, int);
int fm_mark_set(struct fm_files_model *, int, int);
void fm_marks_clear(struct fm_files_model *);
int fm_files_publish(struct fm_files_model *);
int fm_files_valid(FM_CONST struct fm_files_model *);
int fm_actions_valid(FM_CONST struct fm_actions_model *);
int fm_help_valid(FM_CONST struct fm_help_model *);
int fm_dialog_valid(FM_CONST struct fm_dialog *);

int fm_device_render(FM_CONST struct fm_device_model *, struct fm_display *);
int fm_files_render(FM_CONST struct fm_files_model *, struct fm_display *);
int fm_actions_render(FM_CONST struct fm_actions_model *, struct fm_display *);
int fm_help_render(FM_CONST struct fm_help_model *, struct fm_display *);
int fm_render(FM_CONST struct fm_model *, struct fm_display *);
int fm_validate(FM_CONST struct fm_model *);
int fm_control_valid(FM_CONST struct fm_model *);
/* Resolves marks before cursor, in listing order, without changing the model.
 * On rejection request.count is zero; other request fields are untouched. */
int fm_resolve_targets(FM_CONST struct fm_files_model *, int, struct fm_request *);
/* Controller extension, not a backend service. execute borrows the mutable
 * model for this call and returns an FM_* result. Context outlives dispatch.
 * Shared routing handles pane movement/marks before invoking named actions.
 */
struct fm_action_handler {
    void *context;
    int (*execute)(void *, struct fm_model *, int);
    /* Optional: accept a completed non-job picker/input. Owns commit policy. */
    int (*accept)(void *, struct fm_model *, struct fm_invoker *);
};
int fm_dispatch_to(struct fm_model *, struct fm_invoker *, struct fm_action_handler *, int);
void fm_model_init(struct fm_model *);
int fm_command_available(const struct fm_model *model, int command);
/* Startup only, after model initialization and before opening a browser.
 * Rebuilds the main action menu; features describe policy, not OS identity. */
void fm_features_apply(struct fm_model *model, unsigned features);
void fm_setup_menu(struct fm_model *model, int enabled);
int fm_job_start(struct fm_model *, struct fm_invoker *);

/* Display adapter storage; views depend only on fm_display. */
struct fm_canvas {
    char text[FM_ROWS][FM_COLUMNS];
    char attribute[FM_ROWS][FM_COLUMNS];
};
void fm_canvas_bind(struct fm_display *, struct fm_canvas *);
int fm_native_open(void);
int fm_native_present(FM_CONST struct fm_canvas *);
int fm_native_read(void);
void fm_native_close(void);

/* Shared presentation/cursor helpers; no action policy or filesystem access. */
void fm_dialog_close(struct fm_model *);
void fm_dialog_info(struct fm_model *, FM_CONST char *, FM_CONST char *);
void fm_cursor_move(struct fm_cursor *, int, int, int, int);

#endif
