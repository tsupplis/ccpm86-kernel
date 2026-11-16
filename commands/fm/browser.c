/* Real navigation and action coordination. No native OS calls, output
 * calls or private input loop. Existing pane navigation remains in control.c. */
#include <stdio.h>
#include <stdlib.h>
#include "browser.h"

static void error(struct fm_model *model, FM_CONST char *message)
{
    fm_dialog_info(model, "File browser", message);
}

/* Match an 8.3 mask without recursion or OS calls. '*' fills the remainder
 * of its component, '?' matches one padded character. A bare '*' means *.*. */
static int mask_component(FM_CONST char *name, FM_CONST char *pattern, int width)
{
    int index, actual, wanted;

    for (index = 0; index < width; ++index) {
        if (*pattern == '*')
            return 1;
        actual = *name && *name != '.' ? *name++ : ' ';
        wanted = *pattern && *pattern != '.' ? *pattern++ : ' ';
        if (actual >= 'a' && actual <= 'z')
            actual -= 'a' - 'A';
        if (wanted >= 'a' && wanted <= 'z')
            wanted -= 'a' - 'A';
        if (wanted != '?' && actual != wanted)
            return 0;
    }
    return 1;
}

/* Validate each component before changing the active listing. Directory and
 * drive syntax belongs to location selection, never to a filename subset. */
static int subset_valid(FM_CONST char *pattern)
{
    int index, length, extension, ch;

    length = extension = 0;
    if (!pattern[0])
        return 0;
    for (index = 0; pattern[index]; ++index) {
        ch = pattern[index];
        if (ch == '.') {
            if (extension || !length)
                return 0;
            extension = 1;
            length = 0;
        } else {
            if (ch <= ' ' || ch == ':' || ch == '\\' || ch == '/' || ch == '<' || ch == '>' ||
                ch == '|' || ch == '"')
                return 0;
            if (++length > (extension ? 3 : 8))
                return 0;
        }
    }
    return 1;
}

static int subset_match(FM_CONST char *name, FM_CONST char *pattern)
{
    FM_CONST char *extension, *mask_extension;

    if (!pattern[0] || (pattern[0] == '*' && !pattern[1]))
        return 1;
    extension = name;
    mask_extension = pattern;
    while (*extension && *extension != '.')
        ++extension;
    while (*mask_extension && *mask_extension != '.')
        ++mask_extension;
    if (*extension)
        ++extension;
    if (*mask_extension)
        ++mask_extension;
    return mask_component(name, pattern, 8) && mask_component(extension, mask_extension, 3);
}

/* Preserve the original lexical ordering, including its seven-bit comparison.
 * Sort before publishing borrowed names or accepting marks/action snapshots. */
static int compare_entries(const void *left, const void *right)
{
    const char *a = ((const struct fm_entry *)left)->name;
    const char *b = ((const struct fm_entry *)right)->name;

    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return (*a & 127) - (*b & 127);
}

static void sort_entries(struct fm_browser *browser, int count)
{
    int index;

    qsort(browser->entries, count, sizeof(*browser->entries), compare_entries);
    for (index = 0; index < count; ++index) {
        browser->files[index].name = browser->entries[index].name;
        browser->files[index].directory = browser->entries[index].directory;
        browser->files[index].detail = browser->entries[index].directory ? "<DIR>" : "";
    }
}

/* Catalog capacity doubles from 32 entries, up to the 1,024-entry limit.
 * Doubling bounds allocation churn and old-plus-new storage during growth.
 * One allocation owns both arrays. A failed allocation preserves existing
 * storage; the caller reports its partial listing. No job is active here. */
static int reserve_catalog(struct fm_browser *browser, int count)
{
    void *storage;
    struct fm_entry *entries;
    int capacity, index;

    if (count <= browser->capacity)
        return FM_FS_OK;
    capacity = browser->capacity ? browser->capacity * 2 : 32;
    while (capacity < count)
        capacity *= 2;
    if (capacity > FM_MAX_FILES)
        return FM_FS_LIMIT;
    storage = (*browser->memory->allocate)(
        browser->memory->context, capacity * (sizeof(struct fm_entry) + sizeof(struct fm_file)));
    if (!storage)
        return FM_FS_ERROR;
    entries = (struct fm_entry *)storage;
    for (index = 0; index < count - 1 && index < browser->capacity; ++index) {
        entries[index] = browser->entries[index];
    }
    if (browser->catalog_storage)
        (*browser->memory->release)(browser->memory->context, browser->catalog_storage);
    browser->catalog_storage = storage;
    browser->capacity = capacity;
    browser->entries = entries;
    browser->files = (struct fm_file *)((char *)storage + capacity * sizeof(struct fm_entry));
    return FM_FS_OK;
}

/* Commit only after the new listing opens. Later read errors report an
 * incomplete catalog. The memory bound is explicit, never silent truncation. */
static int refresh(struct fm_browser *browser, struct fm_model *model,
                   FM_CONST struct fm_location *location)
{
    struct fm_filesystem *fs;
    struct fm_entry extra;
    int handle, result, count, closed;
    unsigned long kib;

    fs = browser->filesystem;
    result = (*fs->list_open)(fs->context, location, &handle);
    if (result != FM_FS_OK) {
        error(model, "Cannot list this location; previous location retained.");
        return result;
    }
    browser->location = *location;
    count = 0;
    model->files.validated_items = 0;
    model->files.validated_count = -1;
    model->files.count = 0;
    model->files.cursor.selected = FM_NO_SELECTION;
    model->files.cursor.first = 0;
    model->files.items = browser->files;
    model->catalog = browser->files;
    model->catalog_count = 0;
    fm_marks_clear(&model->files);
    ++model->generation;
    if (location->media == FM_MEDIA_CPM) {
        snprintf(browser->location_text, sizeof(browser->location_text),
                 "You are here > %c: User %d", 'A' + location->drive, location->user);
        model->device.media = "CP/M Media";
    } else {
        snprintf(browser->location_text, sizeof(browser->location_text), "You are here > %c:%s",
                 'A' + location->drive, location->path);
        model->device.media = "DOS Media";
    }
    model->device.location = browser->location_text;
    strcpy(browser->capacity_text, "Unknown");
    model->device.capacity = browser->capacity_text;
    while (count < FM_MAX_FILES) {
        result = (*fs->list_next)(fs->context, handle, &extra);
        if (result != FM_FS_OK)
            break;
        if (extra.directory || !subset_match(extra.name, browser->subset))
            continue;
        if (reserve_catalog(browser, count + 1)) {
            result = FM_FS_ERROR;
            break;
        }
        strcpy(browser->entries[count].name, extra.name);
        browser->entries[count].directory = extra.directory;
        browser->files[count].directory = browser->entries[count].directory;
        browser->files[count].name = browser->entries[count].name;
        browser->files[count].detail = browser->entries[count].directory ? "<DIR>" : "";
        ++count;
    }
    if (count == FM_MAX_FILES)
        result = (*fs->list_next)(fs->context, handle, &extra);
    closed = (*fs->list_close)(fs->context, handle);
    if (!closed && fs->space && !fs->space(fs->context, location->drive, &kib))
        snprintf(browser->capacity_text, sizeof(browser->capacity_text), "%luk", kib);
    sort_entries(browser, count);
    model->files.items = model->catalog = browser->files;
    fm_type_job_bind(&browser->type_service, &browser->type_job, fs, &browser->location,
                     browser->entries, count);
    if (browser->copy_enabled)
        fm_copy_bind(&browser->copy_service, &browser->copy_job, fs, browser->copy_store,
                     &browser->location, browser->entries);
    model->files.count = model->catalog_count = count;
    model->files.cursor.selected = count ? 0 : FM_NO_SELECTION;
    if (fm_files_publish(&model->files))
        return FM_FS_ERROR;
    if (closed != FM_FS_OK || (result != FM_FS_OK && result != FM_FS_END)) {
        error(model, "Directory read failed; displayed listing may be incomplete.");
        return FM_FS_ERROR;
    }
    if (result == FM_FS_OK) {
        error(model, "Catalog memory limit reached; listing is incomplete.");
        return FM_FS_LIMIT;
    }
    return FM_FS_OK;
}

/* Name the effective target even while keyboard focus is in Actions. */
static void selection_status(struct fm_browser *browser, struct fm_model *model)
{
    int index, count = 0;
    const char *name = "";
    const struct fm_command *action;

    for (index = 0; index < model->files.count; ++index)
        if (fm_marked(&model->files, index)) {
            if (!count)
                name = browser->entries[index].name;
            ++count;
        }
    if (!count && model->files.cursor.selected >= 0)
        name = browser->entries[model->files.cursor.selected].name;
    action = model->actions.cursor.selected >= 0
                 ? &model->actions.items[model->actions.cursor.selected]
                 : 0;
    if (model->actions.focused && action && action->targets) {
        if (count)
            snprintf(browser->selection_text, sizeof(browser->selection_text),
                     "%s: %d marked files (*). Tab: files; F4: clear.", action->label, count);
        else
            snprintf(browser->selection_text, sizeof(browser->selection_text),
                     "%s: %s (current file). Tab: select files.", action->label,
                     *name ? name : "none");
    } else if (count)
        snprintf(browser->selection_text, sizeof(browser->selection_text),
                 "%d files marked (*). Tab: switch pane; F4: clear.", count);
    else
        snprintf(browser->selection_text, sizeof(browser->selection_text), "Current file: %s. %s",
                 *name ? name : "none",
                 model->files.focused ? "Space: mark; Tab: actions." : "Tab: select files.");
    model->help.status = browser->selection_text;
}

static void copy_context(struct fm_browser *browser, struct fm_model *model)
{
    const char *name = browser->entries[model->pending.files[0]].name;

    if (model->pending.count == 1)
        snprintf(browser->copy_title, sizeof(browser->copy_title), "Copy %s - destination", name);
    else
        snprintf(browser->copy_title, sizeof(browser->copy_title),
                 "Copy %d marked files - destination", model->pending.count);
    snprintf(browser->selection_text, sizeof(browser->selection_text), "Copy from %c:%s  %s%s",
             'A' + browser->location.drive,
             browser->location.media == FM_MEDIA_FAT ? browser->location.path : "", name,
             model->pending.count > 1 ? " and other marked files (*)" : "");
    if (browser->location.media == FM_MEDIA_CPM)
        snprintf(browser->selection_text, sizeof(browser->selection_text),
                 "Copy from %c: User %d: %s%s", 'A' + browser->location.drive,
                 browser->location.user, name,
                 model->pending.count > 1 ? " and other marked files (*)" : "");
    model->dialog.title = browser->copy_title;
    model->help.status = browser->selection_text;
}

/* Shared picker owns a candidate, never the source catalog or marks. */
static void pick(struct fm_browser *browser, struct fm_model *model, int drives)
{
    fm_dialog_close(model);
    browser->picking = 1;
    if (fm_location_begin(&browser->picker, &model->dialog, browser->filesystem, &browser->location,
                          drives, browser->slots, browser->slot_set) == FM_PICK_ERROR) {
        browser->picking = browser->copy_pending = 0;
        error(model, "Cannot open location picker.");
    }
}

int fm_browser_close(struct fm_browser *browser)
{
    int result;
    if (!browser)
        return FM_FS_ERROR;
    result = (*browser->type_service.cancel)(browser->type_service.context);
    if (browser->copy_enabled && (*browser->copy_service.cancel)(browser->copy_service.context))
        result = FM_FS_ERROR;
    if (browser->catalog_storage)
        (*browser->memory->release)(browser->memory->context, browser->catalog_storage);
    browser->catalog_storage = 0;
    browser->entries = 0;
    browser->files = 0;
    browser->capacity = 0;
    return result;
}

static int execute(void *context, struct fm_model *model, int command)
{
    struct fm_browser *browser;
    int result, index, media;
    char destination[5];
    browser = (struct fm_browser *)context;
    if (!fm_command_available(model, command)) {
        fm_dialog_info(model, "Unavailable", "This action is not available on this system.");
        return FM_SUCCESS;
    }
    if (command == FM_CMD_SETUP) {
        browser->setup_menu = 1;
        fm_setup_menu(model, 1);
        return FM_SUCCESS;
    }
    if (command == FM_CMD_BACKUP && browser->backup_enabled) {
        result = fm_resolve_targets(&model->files, FM_TARGET_MANY, &model->pending);
        if (result && result != FM_NO_TARGETS)
            return result;
        /* Directory cursors are valid for whole-location backup/restore.
         * Selected-file mode can only use the snapshotted regular files. */
        result = 0;
        for (index = 0; index < model->pending.count; ++index)
            if (!browser->entries[model->pending.files[index]].directory)
                model->pending.files[result++] = model->pending.files[index];
        model->pending.count = result;
        model->pending.command = command;
        model->pending.generation = model->generation;
        model->last_command = command;
        result = fm_backup_configure(&browser->backup_job, browser->entries, model->files.count);
        if (result)
            return result;
        browser->active_service = &browser->backup_service;
        browser->mutation_refresh = 1;
        return fm_job_start(model, browser->active_service);
    }
    if (command == FM_CMD_PRINT && browser->print_enabled) {
        if (fm_resolve_targets(&model->files, FM_TARGET_MANY, &model->pending)) {
            error(model, "No files selected.");
            return FM_SUCCESS;
        }
        for (index = 0; index < model->pending.count; ++index) {
            if (browser->entries[model->pending.files[index]].directory) {
                error(model, "Files only; unmark directory entries.");
                return FM_SUCCESS;
            }
        }
        model->pending.command = command;
        model->pending.generation = model->generation;
        model->last_command = command;
        result = fm_print_configure(&browser->print_job, browser->entries, model->files.count);
        if (result)
            return result;
        browser->active_service = &browser->print_service;
        return fm_job_start(model, browser->active_service);
    }
    if (browser->run_enabled && fm_run_policy(command) >= 0) {
        result = fm_resolve_targets(&model->files, fm_run_policy(command), &model->pending);
        if (result ||
            (model->pending.count && browser->entries[model->pending.files[0]].directory)) {
            error(model, "Select one file; directories cannot be executed or edited.");
            return FM_SUCCESS;
        }
        model->pending.command = command;
        model->pending.generation = model->generation;
        model->last_command = command;
        result = fm_run_configure(&browser->run_job, browser->entries, model->files.count);
        if (result)
            return result;
        browser->active_service = &browser->run_service;
        browser->mutation_refresh = 1;
        return fm_job_start(model, browser->active_service);
    }
    if (command == FM_CMD_SUBSET) {
        fm_dialog_close(model);
        browser->subset_pending = 1;
        model->dialog.purpose = FM_PURPOSE_SUBSET;
        model->dialog.kind = FM_DIALOG_INPUT;
        model->dialog.title = "Subset of files";
        strcpy(model->dialog.text, "8.3 mask, for example *.TXT; * restores all. Esc cancels.");
        strcpy(model->dialog.input, browser->subset);
        model->dialog.position = strlen(model->dialog.input);
        return FM_SUCCESS;
    }
    if (command == FM_CMD_BOOKMARK1 || command == FM_CMD_BOOKMARK2) {
        index = command - FM_CMD_BOOKMARK1;
        browser->slot_pending = index + 1;
        model->dialog.purpose = FM_PURPOSE_DIRECTORY_SLOT;
        fm_dialog_close(model);
        model->dialog.kind = FM_DIALOG_PICK;
        model->dialog.title = index ? "Directory slot 2" : "Directory slot 1";
        model->dialog.options[0] = "Store current location";
        model->dialog.options[1] = "Go to stored location";
        model->dialog.options[2] = "Clear stored location";
        model->dialog.count = browser->slot_set[index] ? 3 : 1;
        model->dialog.selected = browser->slot_set[index] ? 1 : 0;
        model->dialog.first = 0;
        if (!browser->slot_set[index])
            strcpy(model->dialog.text, "Empty bookmark. Enter stores the current location.");
        else if (browser->slots[index].media == FM_MEDIA_CPM)
            snprintf(model->dialog.text, sizeof(model->dialog.text), "%c: User %d",
                     'A' + browser->slots[index].drive, browser->slots[index].user);
        else
            snprintf(model->dialog.text, sizeof(model->dialog.text), "%c:%s",
                     'A' + browser->slots[index].drive, browser->slots[index].path);
        return FM_SUCCESS;
    }
    if (command == FM_CMD_DRIVE || command == FM_CMD_DIRECTORY) {
        pick(browser, model, command == FM_CMD_DRIVE);
        return FM_SUCCESS;
    }
    if (command == FM_CMD_OPEN)
        command = FM_CMD_TYPE;
    if ((command == FM_CMD_RENAME || command == FM_CMD_DELETE || command == FM_CMD_SYSTEM_ATTR ||
         command == FM_CMD_LOCAL_ATTR) &&
        browser->mutation_enabled) {
        result = fm_resolve_targets(&model->files, FM_TARGET_MANY, &model->pending);
        if (result) {
            error(model, "No files selected.");
            return FM_SUCCESS;
        }
        for (index = 0; index < model->pending.count; ++index) {
            if (browser->entries[model->pending.files[index]].directory) {
                error(model, "Files only; unmark directory entries.");
                return FM_SUCCESS;
            }
        }
        model->pending.command = command;
        model->pending.generation = model->generation;
        model->last_command = command;
        result =
            fm_mutation_configure(&browser->mutation_job, browser->entries, model->files.count);
        if (result)
            return result;
        browser->active_service = &browser->mutation_service;
        browser->mutation_refresh = 1;
        return fm_job_start(model, browser->active_service);
    }
    if ((command == FM_CMD_COPY || command == FM_CMD_SYSTEM_COPY) && browser->copy_enabled) {
        if (fm_resolve_targets(&model->files, FM_TARGET_MANY, &model->pending)) {
            error(model, "No files selected.");
            return FM_SUCCESS;
        }
        for (index = 0; index < model->pending.count; ++index)
            if (browser->entries[model->pending.files[index]].directory) {
                error(model, "Copy files only; unmark directory entries.");
                return FM_SUCCESS;
            }
        model->pending.command = command;
        model->pending.generation = model->generation;
        model->last_command = command;
        if (command == FM_CMD_SYSTEM_COPY) {
            if (browser->system_drive < 0 || browser->system_drive > 15 ||
                !browser->mutation_enabled ||
                browser->filesystem->probe(browser->filesystem->context, browser->system_drive,
                                           &media)) {
                error(model, "System disk is unavailable.");
                return FM_SUCCESS;
            }
            snprintf(destination, sizeof(destination), "%c:%s", 'A' + browser->system_drive,
                     media == FM_MEDIA_CPM ? "0" : "\\");
            if (fm_copy_configure(&browser->copy_job, model->files.count, destination,
                                  browser->mutation_store)) {
                error(model, "Cannot prepare system-disk destination.");
                return FM_SUCCESS;
            }
            browser->active_service = &browser->copy_service;
            browser->mutation_refresh = 1;
            return fm_job_start(model, browser->active_service);
        }
        browser->copy_pending = 1;
        pick(browser, model, 1);
        if (browser->picking)
            copy_context(browser, model);
        return FM_SUCCESS;
    }
    if (command == FM_CMD_TYPE) {
        result = fm_resolve_targets(&model->files, FM_TARGET_MANY, &model->pending);
        if (result) {
            error(model, "No files selected.");
        } else {
            model->last_command = command;
            model->pending.command = command;
            model->pending.generation = model->generation;
            browser->active_service = &browser->type_service;
            result = fm_job_start(model, browser->active_service);
            if (!result)
                result =
                    fm_dispatch_to(model, &browser->type_service, &browser->actions, FM_KEY_TICK);
            return result;
        }
        return FM_SUCCESS;
    }
    if (command == FM_CMD_HELP) {
        error(model, "Tab: focus. Space: mark. F2: Type. F4: clear. F9: location. Esc: back.");
    } else if (command == FM_CMD_DETAILS) {
        model->files.details = !model->files.details;
        model->actions.label = model->files.details ? "Size/Date OFF" : "Size/Date ON";
        model->files.columns = model->files.details ? 1 : model->files.normal_columns;
        fm_cursor_move(&model->files.cursor, model->files.count, 0, model->files.columns,
                       model->files.panel.rect.height - 4);
        browser->detail_first = -1;
    } else {
        error(model, "This action is not available in this package.");
    }
    return FM_SUCCESS;
}

int fm_browser_open(struct fm_browser *browser, struct fm_model *model,
                    struct fm_filesystem *filesystem, struct fm_memory *memory, int drive, int user)
{
    int media;
    if (!browser || !model || !memory || !memory->allocate || !memory->release || !filesystem ||
        !filesystem->drives || !filesystem->probe || !filesystem->list_open ||
        !filesystem->list_next || !filesystem->list_close || !filesystem->file_open ||
        !filesystem->file_read || !filesystem->file_close || drive < 0 || drive > 15 || user < 0 ||
        user > 15)
        return FM_FS_ERROR;
    browser->memory = memory;
    browser->catalog_storage = 0;
    browser->capacity = 0;
    browser->entries = 0;
    browser->files = 0;
    if (reserve_catalog(browser, 1))
        return FM_FS_ERROR;
    browser->entries[0].name[0] = 0;
    browser->slot_set[0] = browser->slot_set[1] = 0;
    browser->slot_pending = browser->subset_pending = 0;
    browser->mutation_enabled = browser->mutation_refresh = 0;
    browser->run_enabled = 0;
    browser->setup_menu = 0;
    browser->system_drive = -1;
    browser->print_enabled = browser->backup_enabled = 0;
    strcpy(browser->subset, "*");
    browser->detail_first = -1;
    browser->filesystem = filesystem;
    browser->actions.context = browser;
    browser->actions.execute = execute;
    browser->actions.accept = 0;
    browser->picking = browser->copy_pending = browser->copy_enabled = 0;
    browser->active_service = &browser->type_service;
    fm_type_job_bind(&browser->type_service, &browser->type_job, filesystem, &browser->location,
                     browser->entries, 0);
    browser->location.drive = drive;
    browser->location.user = user;
    strcpy(browser->location.path, "\\");
    if ((*filesystem->probe)(filesystem->context, drive, &media)) {
        fm_browser_close(browser);
        return FM_FS_ERROR;
    }
    browser->location.media = media;
    refresh(browser, model, &browser->location);
    return FM_FS_OK;
}

static int browser_key(struct fm_browser *browser, struct fm_model *model,
                       struct fm_invoker *invoker, int key)
{
    int choice, command, result;
    char destination[FM_INPUT_CAPACITY];
    unsigned generation;
    char previous_subset[13];

    if (!browser || !model || fm_validate(model))
        return FM_BAD_MODEL;
    if (model->job_active)
        return fm_dispatch_to(model, browser->active_service, &browser->actions, key);
    if (browser->slot_pending) {
        if (key == FM_KEY_ESCAPE) {
            browser->slot_pending = 0;
        } else if (key == FM_KEY_ENTER) {
            choice = browser->slot_pending - 1;
            browser->slot_pending = 0;
            command = model->dialog.selected;
            fm_dialog_close(model);
            if (!command) {
                browser->slots[choice] = browser->location;
                browser->slot_set[choice] = 1;
            } else if (command == 1) {
                refresh(browser, model, &browser->slots[choice]);
            } else {
                browser->slot_set[choice] = 0;
            }
            return FM_SUCCESS;
        }
        return fm_dispatch_to(model, invoker, &browser->actions, key);
    }
    if (browser->subset_pending) {
        if (key == FM_KEY_ESCAPE) {
            browser->subset_pending = 0;
        } else if (key == FM_KEY_ENTER) {
            choice = strlen(model->dialog.input);
            if (!choice || choice > 12 || !subset_valid(model->dialog.input)) {
                strcpy(model->dialog.text, "Use an 8.3 mask of 1 to 12 characters, or * for all.");
                return FM_SUCCESS;
            }
            strcpy(previous_subset, browser->subset);
            generation = model->generation;
            strcpy(browser->subset, model->dialog.input);
            browser->subset_pending = 0;
            fm_dialog_close(model);
            refresh(browser, model, &browser->location);
            if (model->generation == generation)
                strcpy(browser->subset, previous_subset);
            return FM_SUCCESS;
        }
        return fm_dispatch_to(model, invoker, &browser->actions, key);
    }
    if (browser->picking) {
        result = fm_location_key(&browser->picker, &model->dialog, key);
        if (result == FM_PICK_WAIT)
            return FM_SUCCESS;
        browser->picking = 0;
        fm_dialog_close(model);
        if (result != FM_PICK_ACCEPT) {
            browser->copy_pending = 0;
            if (result == FM_PICK_ERROR)
                error(model, "Cannot read this location.");
            return FM_SUCCESS;
        }
        if (!browser->copy_pending) {
            refresh(browser, model, &browser->picker.candidate);
            return FM_SUCCESS;
        }
        browser->copy_pending = 0;
        if (model->pending.generation != model->generation) {
            error(model, "File list changed; select Copy again.");
            return FM_SUCCESS;
        }
        if (browser->picker.candidate.media == FM_MEDIA_CPM)
            snprintf(destination, sizeof(destination), "%c:%d",
                     'A' + browser->picker.candidate.drive, browser->picker.candidate.user);
        else
            snprintf(destination, sizeof(destination), "%c:%s",
                     'A' + browser->picker.candidate.drive, browser->picker.candidate.path);
        if (fm_copy_configure(&browser->copy_job, model->files.count, destination, 0)) {
            error(model, "Cannot prepare this destination.");
            return FM_SUCCESS;
        }
        browser->active_service = &browser->copy_service;
        return fm_job_start(model, browser->active_service);
    }
    if (model->dialog.kind)
        return fm_dispatch_to(model, invoker, &browser->actions, key);
    if (key == FM_KEY_ESCAPE && browser->setup_menu) {
        browser->setup_menu = 0;
        fm_setup_menu(model, 0);
        return FM_SUCCESS;
    }
    if (key == 8 && browser->location.media == FM_MEDIA_FAT) {
        pick(browser, model, 0);
        return FM_SUCCESS;
    }
    return fm_dispatch_to(model, invoker, &browser->actions, key);
}

/* Optional write capability is bound only by the composition root. Browsing
 * remains usable with a read-only filesystem and no Copy store. */
void fm_browser_copy_bind(struct fm_browser *browser, struct fm_copy_store *store)
{
    if (!browser || !store || !store->prepare || !store->begin || !store->write || !store->commit ||
        !store->abort || !store->error)
        return;
    fm_copy_bind(&browser->copy_service, &browser->copy_job, browser->filesystem, store,
                 &browser->location, browser->entries);
    browser->copy_store = store;
    browser->copy_enabled = 1;
}

/* Catalog mutations must not move an unchanged cursor back to the parent
 * directory. Retain its name, or the nearest surviving position if deleted. */
static void refresh_after_action(struct fm_browser *browser, struct fm_model *model)
{
    char name[FM_NAME_CAPACITY];
    int selected = model->files.cursor.selected;
    int index;

    name[0] = 0;
    if (selected >= 0 && selected < model->files.count)
        strcpy(name, browser->entries[selected].name);
    browser->mutation_refresh = 0;
    if (refresh(browser, model, &browser->location))
        return;
    for (index = 0; index < model->files.count; ++index)
        if (!strcmp(name, browser->entries[index].name)) {
            selected = index;
            break;
        }
    fm_cursor_move(&model->files.cursor, model->files.count, selected, model->files.columns,
                   model->files.panel.rect.height - 4);
}

/* Refresh only the visible metadata page after controller transitions.
 * The view borrows formatted strings and performs no filesystem queries.
 * A job owns the filesystem while active; refresh waits until it finishes. */
int fm_browser_key(struct fm_browser *browser, struct fm_model *model, struct fm_invoker *invoker,
                   int key)
{
    struct fm_metadata metadata;
    int result, row, index, was_printing;

    /* Keep outcome text visible; refresh the changed catalog when dismissed. */
    if (browser->mutation_refresh && !model->job_active && !model->dialog.kind) {
        refresh_after_action(browser, model);
    }
    was_printing = model->job_active && browser->active_service == &browser->print_service;
    result = browser_key(browser, model, invoker, key);
    /* Printing has no catalog mutation. Clear only successfully printed marks. */
    if (was_printing && model->pending.command == FM_CMD_PRINT &&
        model->pending.generation == model->generation) {
        for (index = 0; index < model->job.completed; ++index)
            fm_mark_set(&model->files, model->pending.files[index], 0);
    }
    if (browser->mutation_refresh && !model->job_active && !model->dialog.kind) {
        refresh_after_action(browser, model);
    }
    if (browser->picking && browser->copy_pending)
        copy_context(browser, model);
    else if (!model->job_active && !model->dialog.kind)
        selection_status(browser, model);
    if (result || model->job_active || model->dialog.kind || !model->files.details)
        return result;
    if (browser->detail_first == model->files.cursor.first &&
        browser->detail_generation == model->generation)
        return result;
    for (row = 0; row < model->files.panel.rect.height - 4 && row < FM_DETAIL_ROWS; ++row) {
        index = model->files.cursor.first + row;
        if (index >= model->files.count)
            break;
        strcpy(browser->details[row], "--");
        if (browser->entries[index].directory) {
            strcpy(browser->details[row], "<DIR>");
        } else if (browser->filesystem->metadata) {
            metadata.size_known = metadata.date_known = 0;
            if (!(*browser->filesystem->metadata)(browser->filesystem->context, &browser->location,
                                                  browser->entries[index].name, &metadata))
                fm_metadata_format(&metadata, browser->details[row]);
        }
        browser->files[index].detail = browser->details[row];
    }
    browser->detail_first = model->files.cursor.first;
    browser->detail_generation = model->generation;
    return result;
}

void fm_browser_mutation_bind(struct fm_browser *browser, struct fm_mutation_store *store)
{
    if (!browser || !store || !store->prepare || !store->commit || !store->error)
        return;
    fm_mutation_bind(&browser->mutation_service, &browser->mutation_job, store, &browser->location);
    browser->mutation_store = store;
    browser->mutation_enabled = 1;
}

void fm_browser_run_bind(struct fm_browser *browser, struct fm_executor *executor)
{
    fm_run_bind(&browser->run_service, &browser->run_job, executor, &browser->location);
    browser->run_enabled = 1;
}

void fm_browser_print_bind(struct fm_browser *browser, struct fm_printer *printer)
{
    if (!printer || !printer->attach || !printer->put || !printer->detach)
        return;
    fm_print_bind(&browser->print_service, &browser->print_job, browser->filesystem, printer,
                  &browser->location);
    browser->print_enabled = 1;
}

void fm_browser_backup_bind(struct fm_browser *browser, struct fm_backup_host *host)
{
    if (!browser || !browser->copy_enabled || !host || !host->prepare || !host->execute ||
        !host->error)
        return;
    fm_backup_bind(&browser->backup_service, &browser->backup_job, host, browser->copy_store,
                   &browser->location);
    browser->backup_enabled = 1;
}
