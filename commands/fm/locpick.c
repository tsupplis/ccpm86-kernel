/* Browsable location selection shared by navigation and Copy.
 * Directory pages are rescanned on demand, keeping memory bounded without
 * truncating large directories. The source file catalog is never borrowed.
 */
#include <stdio.h>
#include "locpick.h"

#define DRIVES 1
#define USERS 2
#define FOLDERS 3
#define PAGE_SIZE 12

static void option(struct fm_location_picker *picker, struct fm_dialog *dialog, int operation,
                   const char *label)
{
    int index = dialog->count++;

    picker->operation[index] = operation;
    snprintf(picker->labels[index], sizeof(picker->labels[index]), "%s", label);
    dialog->options[index] = picker->labels[index];
}

static int display(struct fm_location_picker *picker, struct fm_dialog *dialog)
{
    struct fm_filesystem *fs = picker->filesystem;
    struct fm_entry entry;
    unsigned mask;
    int index, handle, result, closed, skipped, shown;
    char label[32];

    dialog->kind = FM_DIALOG_PICK;
    dialog->panel.rect.x = 18;
    dialog->panel.rect.y = 3;
    dialog->panel.rect.width = 44;
    dialog->panel.rect.height = 18;
    dialog->more_before = dialog->more_after = 0;
    dialog->footer[0] = "Enter:open  F2:choose  F3:drives";
    dialog->footer[1] = "Bksp:up PgUp/Dn:page Esc:cancel";
    dialog->first = dialog->selected = dialog->count = 0;
    if (picker->mode == DRIVES) {
        dialog->title = "Select drive";
        strcpy(dialog->text, "Choose a drive to browse");
        dialog->footer[0] = "Enter:open  F7/F8:bookmarks";
        if (fs->drives(fs->context, &mask))
            return FM_PICK_ERROR;
        for (index = 0; index < 16; ++index) {
            if (!(mask & ((unsigned)1 << index)))
                continue;
            snprintf(label, sizeof(label), "Drive %c:", 'A' + index);
            option(picker, dialog, index, label);
        }
        return dialog->count ? FM_PICK_WAIT : FM_PICK_ERROR;
    }
    if (picker->mode == USERS) {
        dialog->title = "Select user area";
        snprintf(dialog->text, sizeof(dialog->text), "%c: Choose a user area",
                 'A' + picker->candidate.drive);
        dialog->footer[0] = "Enter:choose  F3:drives";
        for (index = 0; index < 16; ++index) {
            snprintf(label, sizeof(label), "User %d", index);
            option(picker, dialog, index, label);
        }
        dialog->selected = picker->candidate.user;
        if (dialog->selected >= PAGE_SIZE)
            dialog->first = dialog->selected - PAGE_SIZE + 1;
        return FM_PICK_WAIT;
    }
    dialog->title = "Select directory";
    snprintf(dialog->text, sizeof(dialog->text), "%c:%s", 'A' + picker->candidate.drive,
             picker->candidate.path);
    if (fs->list_open(fs->context, &picker->candidate, &handle))
        return FM_PICK_ERROR;
    skipped = shown = 0;
    while ((result = fs->list_next(fs->context, handle, &entry)) == FM_FS_OK) {
        if (!entry.directory || !strcmp(entry.name, ".") || !strcmp(entry.name, ".."))
            continue;
        if (skipped++ < picker->offset)
            continue;
        if (shown == PAGE_SIZE)
            break;
        snprintf(label, sizeof(label), "%s\\", entry.name);
        option(picker, dialog, 0, label);
        ++shown;
    }
    closed = fs->list_close(fs->context, handle);
    if (closed || (result != FM_FS_OK && result != FM_FS_END))
        return FM_PICK_ERROR;
    dialog->more_before = picker->offset > 0;
    dialog->more_after = result == FM_FS_OK;
    if (!dialog->count)
        dialog->selected = FM_NO_SELECTION;
    return FM_PICK_WAIT;
}

static void parent(struct fm_location *location)
{
    char *slash = strrchr(location->path, '\\');

    if (!slash || slash == location->path)
        location->path[1] = 0;
    else
        *slash = 0;
}

int fm_location_begin(struct fm_location_picker *picker, struct fm_dialog *dialog,
                      struct fm_filesystem *filesystem, const struct fm_location *current,
                      int drives, const struct fm_location *bookmarks, const int *available)
{
    int index;

    picker->filesystem = filesystem;
    picker->candidate = *current;
    picker->offset = 0;
    picker->mode = drives ? DRIVES : current->media == FM_MEDIA_CPM ? USERS : FOLDERS;
    for (index = 0; index < 2; ++index) {
        picker->bookmark_set[index] = available[index];
        if (available[index])
            picker->bookmarks[index] = bookmarks[index];
    }
    return display(picker, dialog);
}

int fm_location_key(struct fm_location_picker *picker, struct fm_dialog *dialog, int key)
{
    struct fm_location before;
    struct fm_cursor cursor;
    int operation, result, length, old_mode, old_offset, bookmark;

    before = picker->candidate;
    if (key == FM_KEY_ESCAPE)
        return FM_PICK_CANCEL;
    old_mode = picker->mode;
    old_offset = picker->offset;
    if (key == FM_KEY_F1 + 6 || key == FM_KEY_F1 + 7) {
        bookmark = key - FM_KEY_F1 - 6;
        if (!picker->bookmark_set[bookmark]) {
            strcpy(dialog->text, "Bookmark is empty. Select a drive or directory.");
            return FM_PICK_WAIT;
        }
        picker->candidate = picker->bookmarks[bookmark];
        picker->mode = picker->candidate.media == FM_MEDIA_CPM ? USERS : FOLDERS;
        picker->offset = 0;
        return display(picker, dialog);
    }
    if (key == FM_KEY_F1 + 1 && picker->mode == FOLDERS)
        return FM_PICK_ACCEPT;
    if (key == FM_KEY_F1 + 2) {
        picker->mode = DRIVES;
        picker->offset = 0;
    } else if (picker->mode == FOLDERS && (key == FM_KEY_PGUP || key == FM_KEY_PGDN)) {
        if (key == FM_KEY_PGUP && picker->offset)
            picker->offset -= PAGE_SIZE;
        else if (key == FM_KEY_PGDN && dialog->more_after)
            picker->offset += PAGE_SIZE;
        else
            return FM_PICK_WAIT;
    } else if (key == 8) {
        if (picker->mode == FOLDERS && picker->candidate.path[1])
            parent(&picker->candidate);
        else
            picker->mode = DRIVES;
        picker->offset = 0;
    } else if (key == FM_KEY_ENTER) {
        if (dialog->selected < 0 || dialog->selected >= dialog->count)
            return FM_PICK_WAIT;
        operation = picker->operation[dialog->selected];
        if (picker->mode == DRIVES) {
            if (picker->filesystem->probe(picker->filesystem->context, operation,
                                          &picker->candidate.media)) {
                picker->candidate = before;
                strcpy(dialog->text, "Drive unavailable. Choose another drive.");
                return FM_PICK_WAIT;
            }
            picker->candidate.drive = operation;
            picker->candidate.user = 0;
            strcpy(picker->candidate.path, "\\");
            picker->mode = picker->candidate.media == FM_MEDIA_CPM ? USERS : FOLDERS;
            picker->offset = 0;
        } else if (picker->mode == USERS) {
            picker->candidate.user = operation;
            return FM_PICK_ACCEPT;
        } else {
            length = strlen(picker->candidate.path);
            if (length + strlen(picker->labels[dialog->selected]) + 2 >
                sizeof(picker->candidate.path)) {
                strcpy(dialog->text, "Directory path is too long.");
                return FM_PICK_WAIT;
            }
            if (length > 1)
                strcat(picker->candidate.path, "\\");
            strcat(picker->candidate.path, picker->labels[dialog->selected]);
            picker->candidate.path[strlen(picker->candidate.path) - 1] = 0;
            picker->offset = 0;
        }
    } else {
        cursor.selected = dialog->selected;
        cursor.first = dialog->first;
        operation = 0;
        if (key == FM_KEY_UP)
            operation = -1;
        if (key == FM_KEY_DOWN)
            operation = 1;
        if (key == FM_KEY_HOME)
            operation = -dialog->count;
        if (key == FM_KEY_END)
            operation = dialog->count;
        if (key == FM_KEY_PGUP)
            operation = -(dialog->panel.rect.height - 6);
        if (key == FM_KEY_PGDN)
            operation = dialog->panel.rect.height - 6;
        fm_cursor_move(&cursor, dialog->count, operation, 1, dialog->panel.rect.height - 6);
        dialog->selected = cursor.selected;
        dialog->first = cursor.first;
        return FM_PICK_WAIT;
    }
    result = display(picker, dialog);
    if (result == FM_PICK_ERROR) {
        picker->candidate = before;
        picker->mode = old_mode;
        picker->offset = old_offset;
        /* Restore the previous page where possible; no caller state commits. */
        if (display(picker, dialog) == FM_PICK_ERROR)
            return FM_PICK_ERROR;
        strcpy(dialog->text, "Cannot read that location. Previous location retained.");
    }
    return FM_PICK_WAIT;
}
