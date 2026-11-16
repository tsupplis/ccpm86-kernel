/* Browser contract tests: real controllers with a deterministic filesystem.
 * No renderer, emulator, fixture catalog or native services are linked. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "browser.h"

static int allocations, fail_allocate, allocate_calls, fail_on_call;
static void *allocate(void *context, unsigned bytes)
{
    void *memory;
    ++allocate_calls;
    if (fail_allocate || allocate_calls == fail_on_call) return 0;
    memory = malloc(bytes);
    if (memory) ++allocations;
    return memory;
}
static void release(void *context, void *memory)
{
    if (memory) --allocations;
    free(memory);
}
static struct fm_memory memory = {0, allocate, release};
struct disk {
    int next, count, opens, closes, reads, list_error, close_error;
    int drive, user, media, directories, listed, list_closed;
    char opened[13];
};
static int metadata(void *context, const struct fm_location *location,
                    const char *name, struct fm_metadata *output)
{
    output->size_known = output->date_known = 1;
    output->size_low = 128;
    output->size_high = 0;
    output->days = 1;
    output->hour = 12;
    output->minute = 34;
    return FM_FS_OK;
}
static int space(void *context, int drive, unsigned long *kib)
{
    *kib = drive ? 312UL : 5480UL;
    return FM_FS_OK;
}
static int drives(void *context, unsigned *mask)
{ *mask = 3; return FM_FS_OK; }
static int probe(void *context, int drive, int *media)
{ *media = drive ? FM_MEDIA_FAT : FM_MEDIA_CPM; return FM_FS_OK; }
static int list_open(void *context, const struct fm_location *place, int *handle)
{
    struct disk *disk = context;
    disk->media = place->media;
    disk->drive = place->drive; disk->user = place->user;
    disk->next = 0; *handle = 1; ++disk->listed;
    return disk->list_error ? FM_FS_ERROR : FM_FS_OK;
}
static int list_next(void *context, int handle, struct fm_entry *entry)
{
    struct disk *disk = context;
    if (disk->next == disk->count) return FM_FS_END;
    sprintf(entry->name, "FILE%d.TXT", disk->next++);
    entry->directory = disk->media == FM_MEDIA_FAT && (disk->directories || disk->next == 1);
    if (entry->directory) {
        if (disk->directories) sprintf(entry->name, "DIR%02d", disk->next - 1);
        else strcpy(entry->name, "DOCS");
    }
    return FM_FS_OK;
}
static int list_close(void *context, int handle)
{ ++((struct disk *)context)->list_closed; return FM_FS_OK; }
static int file_open(void *context, const struct fm_location *place,
                     const char *name, int *handle)
{
    struct disk *disk = context;
    ++disk->opens; disk->reads = 0; strcpy(disk->opened, name);
    *handle = 2; return FM_FS_OK;
}
static int file_read(void *context, int handle, char *buffer, int capacity, int *count)
{
    struct disk *disk = context;
    assert(capacity >= 7);
    strcpy(buffer, "Hello\r\n"); *count = 7; ++disk->reads;
    return FM_FS_END;
}
static int file_close(void *context, int handle)
{
    struct disk *disk = context;
    ++disk->closes;
    return disk->close_error ? FM_FS_ERROR : FM_FS_OK;
}
static void key(struct fm_browser *browser, struct fm_model *model, int value)
{ int status = fm_browser_key(browser, model, 0, value);
  if (status != FM_SUCCESS && status != FM_JOB_ERROR)
      fprintf(stderr, "key=%d status=%d dialog=%d picking=%d copy=%d files=%d selected=%d\n", value, status, model->dialog.kind, browser->picking, browser->copy_pending, model->files.count, model->files.cursor.selected);
  if (status == FM_BAD_MODEL) fprintf(stderr, "dialog count=%d selected=%d first=%d text=%s\n", model->dialog.count, model->dialog.selected, model->dialog.first, model->dialog.text);
  assert(status == FM_SUCCESS || status == FM_JOB_ERROR); }

/* Copy is cancelled before transfer: any write callback would be a failure. */
static int store_prepare(void *context, const struct fm_location *source,
                         const struct fm_location *destination,
                         const char *name, int *exists)
{ assert(0); return FM_FS_ERROR; }
static int store_begin(void *context)
{ assert(0); return FM_FS_ERROR; }
static int store_write(void *context, const char *bytes, int count)
{ assert(0); return FM_FS_ERROR; }
static int store_commit(void *context, int replace)
{ assert(0); return FM_FS_ERROR; }
static int store_abort(void *context)
{ return FM_FS_OK; }
static const char *store_error(void *context)
{ return "Unexpected transfer"; }
static struct fm_copy_store store = {
    0, store_prepare, store_begin, store_write, store_commit, store_abort, store_error
};

int main(void)
{
    struct fm_model model;
    struct fm_browser browser;
    struct disk disk;
    struct fm_filesystem fs;
    int opened;
    memset(&disk, 0, sizeof(disk));
    memset(&browser, 0, sizeof(browser));
    memset(&fs, 0, sizeof(fs));
    fs.space = space;
    fs.context = &disk; fs.drives = drives; fs.probe = probe;
    fs.list_open = list_open; fs.list_next = list_next; fs.list_close = list_close;
    fs.file_open = file_open; fs.file_read = file_read; fs.file_close = file_close;
    disk.count = 2;
    fm_model_init(&model);
    assert(model.catalog_count == 0);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 4));
    assert(model.files.count == 2 && disk.user == 4);
    assert(!strcmp(model.device.capacity, "5480k"));
    key(&browser, &model, FM_KEY_TAB);
    key(&browser, &model, ' ');
    assert(fm_marked(&model.files, 0));
    key(&browser, &model, FM_KEY_F1 + 3);
    assert(!fm_marked(&model.files, 0));

    /* Cancel a location change without losing selection or marks. */
    key(&browser, &model, ' ');
    key(&browser, &model, FM_KEY_F1 + 8);
    key(&browser, &model, FM_KEY_PGDN);
    assert(model.dialog.selected > 0);
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(browser.location.user == 4 && fm_marked(&model.files, 0));
    key(&browser, &model, FM_KEY_F1 + 8);
    key(&browser, &model, FM_KEY_HOME);
    key(&browser, &model, FM_KEY_DOWN);
    key(&browser, &model, FM_KEY_ENTER);
    assert(browser.location.user == 1 && !fm_marked(&model.files, 0));

    /* A marked set overrides the cursor; Type preserves it on return. */
    fm_mark_set(&model.files, 0, 1); fm_mark_set(&model.files, 1, 1);
    model.files.cursor.selected = 1;
    key(&browser, &model, FM_KEY_F1 + 1);
    assert(model.job_active && !strcmp(disk.opened, "FILE0.TXT"));
    assert(!strcmp(model.dialog.page_lines[0], "Hello"));
    key(&browser, &model, FM_KEY_ENTER);
    assert(!strcmp(disk.opened, "FILE1.TXT"));
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(model.files.cursor.selected == 1 && fm_marked(&model.files, 0) && fm_marked(&model.files, 1));
    assert(disk.opens == disk.closes);
    opened = disk.opens;
    key(&browser, &model, FM_KEY_F1 + 2);
    assert(disk.opens == opened + 1);
    key(&browser, &model, FM_KEY_ESCAPE);
    key(&browser, &model, FM_KEY_F1 + 3);
    key(&browser, &model, FM_KEY_ENTER);
    assert(!strcmp(disk.opened, "FILE1.TXT"));
    key(&browser, &model, FM_KEY_ESCAPE);

    /* Changing the catalog invalidates an in-flight viewer request. */
    key(&browser, &model, FM_KEY_F1 + 1);
    ++model.generation;
    key(&browser, &model, FM_KEY_PGDN);
    assert(!model.job_active && !browser.type_job.reader.opened);
    assert(model.dialog.kind == FM_DIALOG_INFO);
    key(&browser, &model, FM_KEY_ESCAPE);

    /* Read/close failure is visible and never leaves an active viewer. */
    disk.close_error = 1;
    key(&browser, &model, FM_KEY_F1 + 1);
    assert(!model.job_active && model.dialog.kind == FM_DIALOG_INFO);
    disk.close_error = 0;
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(!fm_browser_close(&browser));

    /* File catalog excludes directories; browsing candidates never changes it. */
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 1, 0));
    assert(!strcmp(model.device.media, "DOS Media"));
    assert(model.files.count == 1 && !strcmp(model.files.items[0].name, "FILE1.TXT"));
    fm_mark_set(&model.files, 0, 1);
    key(&browser, &model, FM_KEY_F1 + 8);
    assert(browser.picking && !strcmp(model.dialog.title, "Select directory"));
    model.dialog.selected = 0; /* DOCS is a directory, not a menu command. */
    key(&browser, &model, FM_KEY_ENTER);
    assert(!strcmp(browser.picker.candidate.path, "\\DOCS"));
    assert(!strcmp(browser.location.path, "\\") && fm_marked(&model.files, 0));
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(!strcmp(browser.location.path, "\\") && fm_marked(&model.files, 0));
    key(&browser, &model, FM_KEY_F1 + 8);
    model.dialog.selected = 0;
    key(&browser, &model, FM_KEY_ENTER);
    key(&browser, &model, FM_KEY_F1 + 1); /* Choose this directory. */
    assert(!strcmp(browser.location.path, "\\DOCS"));
    assert(model.files.count == 1 && !fm_marked(&model.files, 0));
    key(&browser, &model, FM_KEY_F1 + 1);
    assert(model.job_active && !strcmp(disk.opened, "FILE1.TXT"));
    key(&browser, &model, FM_KEY_ESCAPE);

    /* Copy's destination picker does not replace the source catalog/marks. */
    fm_browser_copy_bind(&browser, &store);
    fm_mark_set(&model.files, 0, 1);
    assert(!browser.actions.execute(browser.actions.context, &model, FM_CMD_COPY));
    assert(browser.copy_pending && browser.picking);
    assert(strstr(model.dialog.title, "Copy FILE1.TXT"));
    assert(strstr(model.help.status, "FILE1.TXT"));
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(!browser.copy_pending && fm_marked(&model.files, 0));
    assert(!strcmp(browser.location.path, "\\DOCS"));
    assert(!browser.actions.execute(browser.actions.context, &model, FM_CMD_COPY));
    key(&browser, &model, FM_KEY_ENTER); /* Drive A. */
    key(&browser, &model, FM_KEY_DOWN);
    key(&browser, &model, FM_KEY_ENTER); /* User 1. */
    assert(model.job_active && model.pending.count == 1);
    assert(browser.copy_job.destination.drive == 0 && browser.copy_job.destination.user == 1);
    assert(browser.location.drive == 1 && fm_marked(&model.files, 0));
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(!model.job_active);
    assert(!fm_browser_close(&browser));

    /* Every directory page closes its cursor, with no fixed catalog limit. */
    disk.count = 27;
    disk.directories = 1;
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 1, 0));
    assert(model.files.count == 0);
    key(&browser, &model, FM_KEY_F1 + 8);
    assert(!strcmp(model.dialog.options[0], "DIR00\\"));
    assert(model.dialog.more_after && !model.dialog.more_before);
    key(&browser, &model, FM_KEY_PGDN);
    assert(!strcmp(model.dialog.options[0], "DIR12\\"));
    key(&browser, &model, FM_KEY_PGDN);
    assert(model.dialog.count == 3 && !strcmp(model.dialog.options[2], "DIR26\\"));
    assert(!model.dialog.more_after && model.dialog.more_before);
    key(&browser, &model, FM_KEY_PGUP);
    assert(!strcmp(model.dialog.options[0], "DIR12\\"));
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(disk.listed == disk.list_closed);
    assert(!fm_browser_close(&browser));
    disk.directories = 0;
    disk.count = 0;
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 1, 0));
    key(&browser, &model, FM_KEY_F1 + 8);
    assert(model.dialog.count == 0 && model.dialog.selected == FM_NO_SELECTION);
    key(&browser, &model, FM_KEY_ENTER);
    assert(browser.picking);
    key(&browser, &model, FM_KEY_F1 + 1);
    assert(!browser.picking && !strcmp(browser.location.path, "\\"));
    assert(!fm_browser_close(&browser));
    /* More than one viewport and more than the former 64-entry limit. */
    disk.count = 200;
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 0));
    assert(model.files.count == 200);
    assert(!strcmp(model.files.items[2].name, "FILE10.TXT"));
    assert(!strcmp(model.files.items[199].name, "FILE99.TXT"));
    key(&browser, &model, FM_KEY_TAB);
    key(&browser, &model, FM_KEY_END);
    assert(model.files.cursor.selected == 199 && model.files.cursor.first > 0);
    key(&browser, &model, ' ');
    key(&browser, &model, FM_KEY_F1 + 1);
    assert(!strcmp(disk.opened, "FILE99.TXT"));
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(fm_marked(&model.files, 199) && model.files.cursor.selected == 199);
    assert(!fm_browser_close(&browser));
    disk.count = FM_MAX_FILES + 1;
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 0));
    assert(model.files.count == FM_MAX_FILES && model.dialog.kind == FM_DIALOG_INFO);

    assert(!fm_browser_close(&browser));
    disk.count = 0;
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 0));
    key(&browser, &model, FM_KEY_F1 + 1);
    assert(model.dialog.kind == FM_DIALOG_INFO && !model.job_active);
    assert(!fm_browser_close(&browser));
    disk.list_error = 1;
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 0));
    assert(model.files.count == 0 && model.dialog.kind == FM_DIALOG_INFO);
    assert(!fm_browser_close(&browser));
    /* Real subset selection uses the standard action interface. A filtered
     * listing remains the same catalog consumed by Type and Copy. */
    disk.list_error = 0;
    disk.count = 12;
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 4));
    assert(!browser.actions.execute(browser.actions.context, &model, FM_CMD_SUBSET));
    strcpy(model.dialog.input, "FILE?.TXT");
    model.dialog.position = strlen(model.dialog.input);
    key(&browser, &model, FM_KEY_ENTER);
    assert(model.files.count == 10 && !strcmp(browser.subset, "FILE?.TXT"));
    assert(!browser.actions.execute(browser.actions.context, &model, FM_CMD_SUBSET));
    strcpy(model.dialog.input, "BAD:MASK");
    model.dialog.position = strlen(model.dialog.input);
    key(&browser, &model, FM_KEY_ENTER);
    assert(browser.subset_pending && model.files.count == 10);
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(!strcmp(browser.subset, "FILE?.TXT") && model.files.count == 10);

    /* Slots capture explicit locations, not a borrowed directory mapping. */
    key(&browser, &model, FM_KEY_F1 + 6);
    key(&browser, &model, FM_KEY_ENTER);
    assert(browser.slot_set[0] && browser.slots[0].user == 4);
    key(&browser, &model, FM_KEY_F1 + 8);
    key(&browser, &model, FM_KEY_HOME);
    key(&browser, &model, FM_KEY_ENTER);
    assert(browser.location.user == 0);
    key(&browser, &model, FM_KEY_F1 + 6);
    key(&browser, &model, FM_KEY_ENTER);
    assert(browser.location.user == 4);
    key(&browser, &model, FM_KEY_F1 + 8);
    key(&browser, &model, FM_KEY_F1 + 6); /* Browse saved bookmark. */
    key(&browser, &model, FM_KEY_ENTER);
    assert(!browser.picking && browser.location.user == 4);
    key(&browser, &model, FM_KEY_F1 + 6);
    key(&browser, &model, FM_KEY_DOWN); /* Clear. */
    key(&browser, &model, FM_KEY_ENTER);
    assert(!browser.slot_set[0]);
    key(&browser, &model, FM_KEY_F1 + 7);
    key(&browser, &model, FM_KEY_ESCAPE);
    assert(!browser.slot_set[1]);
    assert(!fm_browser_close(&browser));

    fs.metadata = metadata;
    fm_model_init(&model);
    disk.count = 20;
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 4));
    assert(!browser.actions.execute(browser.actions.context, &model, FM_CMD_DETAILS));
    key(&browser, &model, FM_KEY_TICK);
    assert(model.files.details && model.files.columns == 1);
    assert(!strcmp(model.actions.label, "Size/Date OFF"));
    assert(strstr(model.files.items[0].detail, "1 KiB  1978-01-01 12:34"));
    key(&browser, &model, FM_KEY_TAB);
    key(&browser, &model, ' ');
    key(&browser, &model, FM_KEY_END);
    assert(strstr(model.files.items[19].detail, "1978-01-01"));
    assert(fm_marked(&model.files, 0));
    browser.mutation_refresh = 1;
    key(&browser, &model, FM_KEY_TICK);
    assert(model.files.cursor.selected == 19);
    assert(!browser.actions.execute(browser.actions.context, &model, FM_CMD_DETAILS));
    assert(!model.files.details && model.files.columns == 4);
    assert(!strcmp(model.actions.label, "Size/Date ON"));
    assert(!fm_browser_close(&browser));
    assert(allocations == 0);
    /* Restricted capabilities filter menus and reject Repeat/direct dispatch.
     * DREDIX, native file operations and user-area navigation remain enabled. */
    fm_model_init(&model);
    fm_features_apply(&model, 0);
    assert(model.actions.count == 16);
    assert(!fm_command_available(&model, FM_CMD_BACKUP));
    assert(fm_command_available(&model, FM_CMD_EDIT));
    assert(fm_command_available(&model, FM_CMD_DIRECTORY));
    fm_setup_menu(&model, 1);
    assert(model.actions.count == 6);
    fm_setup_menu(&model, 0);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 4));
    assert(!browser.actions.execute(browser.actions.context, &model, FM_CMD_BACKUP));
    assert(model.dialog.kind == FM_DIALOG_INFO && !model.job_active);
    key(&browser, &model, FM_KEY_ENTER);
    model.last_command = FM_CMD_BOOT_DISK;
    key(&browser, &model, FM_KEY_F1 + 2);
    assert(model.dialog.kind == FM_DIALOG_INFO && !model.job_active);
    assert(!fm_browser_close(&browser));
    assert(allocations == 0);
    /* Growth failure publishes only valid entries and releases all storage. */
    disk.count = 100;
    fail_on_call = allocate_calls + 2;
    fm_model_init(&model);
    assert(!fm_browser_open(&browser, &model, &fs, &memory, 0, 4));
    assert(model.files.count == 32 && model.dialog.kind == FM_DIALOG_INFO);
    assert(!strcmp(model.files.items[31].name, "FILE9.TXT"));
    assert(!fm_browser_close(&browser));
    assert(allocations == 0);
    fail_on_call = 0;
    fail_allocate = 1;
    assert(fm_browser_open(&browser, &model, &fs, &memory, 0, 4) == FM_FS_ERROR);
    assert(allocations == 0);
    puts("PASS: real browser routing, CP/M selection, cancellation, marks, Type/repeat, cleanup, FAT navigation and large/overflow listings");
    return 0;
}
