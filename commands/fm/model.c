/* Shared model validation. Pure, allocation-free and independent of UI output. */
#include "fm.h"

int fm_marked(const struct fm_files_model *files, int index)
{
    if (!files || index < 0 || index >= files->count || index >= FM_MAX_FILES)
        return 0;
    return (files->marks[index / 8] & (1U << (index % 8))) != 0;
}

int fm_mark_set(struct fm_files_model *files, int index, int marked)
{
    unsigned mask;

    if (!files || index < 0 || index >= files->count || index >= FM_MAX_FILES)
        return FM_BAD_MODEL;
    mask = 1U << (index % 8);
    if (marked)
        files->marks[index / 8] |= mask;
    else
        files->marks[index / 8] &= ~mask;
    return FM_SUCCESS;
}

void fm_marks_clear(struct fm_files_model *files)
{
    if (files)
        memset(files->marks, 0, sizeof(files->marks));
}

int fm_panel_valid(FM_CONST struct fm_panel *panel)
{
    FM_CONST struct fm_rect *r;

    if (!panel)
        return 0;
    r = &panel->rect;
    if (panel->style.normal < 0 || panel->style.normal > 255 || panel->style.selected < 0 ||
        panel->style.selected > 255 || panel->style.marked < 0 || panel->style.marked > 255 ||
        panel->style.accent < 0 || panel->style.accent > 255 || panel->style.border < 0 ||
        panel->style.border > 255)
        return 0;
    return r->width > 0 && r->width <= FM_COLUMNS && r->height > 0 && r->height <= FM_ROWS &&
           r->x >= 0 && r->x <= FM_COLUMNS - r->width && r->y >= 0 && r->y <= FM_ROWS - r->height;
}

static int cursor_valid(FM_CONST struct fm_cursor *cursor, int count)
{
    if (count == 0)
        return cursor->selected == FM_NO_SELECTION && cursor->first == 0;
    return cursor->selected >= FM_NO_SELECTION && cursor->selected < count && cursor->first >= 0 &&
           cursor->first < count;
}

int fm_device_valid(FM_CONST struct fm_device_model *model)
{
    return model && fm_panel_valid(&model->panel) && model->panel.rect.height >= 4 &&
           model->media_width > 0 && model->media_width <= model->panel.rect.width - 5 &&
           model->media && model->capacity && model->location;
}

int fm_files_valid(FM_CONST struct fm_files_model *model)
{
    int index;

    if (!model || !fm_panel_valid(&model->panel) || model->panel.rect.height < 5 ||
        model->count < 0 || model->count > FM_MAX_FILES || (model->count && !model->items) ||
        !cursor_valid(&model->cursor, model->count) || model->columns < 1 ||
        model->columns > (model->panel.rect.width - 2) / 2 || !model->heading || !model->empty_text)
        return 0;
    if (model->validated_items == model->items && model->validated_count == model->count)
        return 1;
    for (index = 0; index < model->count; ++index) {
        if (!model->items[index].name)
            return 0;
    }
    return 1;
}

/* Called only after publication has established cursor/layout invariants.
 * Names remain immutable until the owner revokes this certificate. */
int fm_files_publish(struct fm_files_model *model)
{
    if (!model)
        return FM_BAD_MODEL;
    model->validated_items = 0;
    model->validated_count = -1;
    if (!fm_files_valid(model))
        return FM_BAD_MODEL;
    model->validated_items = model->items;
    model->validated_count = model->count;
    return FM_SUCCESS;
}

int fm_actions_valid(FM_CONST struct fm_actions_model *model)
{
    int index;

    if (!model || !fm_panel_valid(&model->panel) ||
        (model->panel.rect.height != 1 && model->panel.rect.height < 3) ||
        model->panel.rect.width < 3 || model->columns < 1 ||
        model->columns > model->panel.rect.width / 2 || model->count < 0 ||
        model->count > FM_MAX_FILES || (model->count && !model->items) ||
        !cursor_valid(&model->cursor, model->count) || !model->empty_text)
        return 0;
    for (index = 0; index < model->count; ++index) {
        if (!model->items[index].label || !model->items[index].help ||
            model->items[index].targets < FM_TARGET_NONE ||
            model->items[index].targets > FM_TARGET_MANY)
            return 0;
    }
    return 1;
}

int fm_help_valid(FM_CONST struct fm_help_model *model)
{
    int index;
    FM_CONST struct fm_hint *hint;

    if (!model || !fm_panel_valid(&model->panel) || !model->status || model->count < 0 ||
        model->count > 16 || (model->count && !model->hints))
        return 0;
    for (index = 0; index < model->count; ++index) {
        hint = &model->hints[index];
        if (!hint->text || hint->column < 0 || hint->column >= model->panel.rect.width ||
            hint->row < 1 || hint->row >= model->panel.rect.height)
            return 0;
    }
    return 1;
}

/* Fixed-capacity dialog text must terminate before any strlen or indexing. */
int fm_dialog_valid(FM_CONST struct fm_dialog *dialog)
{
    int index, length;

    if (!dialog)
        return 0;
    if (dialog->kind == FM_DIALOG_NONE)
        return 1;
    if (dialog->kind < FM_DIALOG_INFO || dialog->kind > FM_DIALOG_TEXT ||
        !fm_panel_valid(&dialog->panel) || dialog->panel.rect.width < 12 ||
        dialog->panel.rect.height < 6 || !dialog->title)
        return 0;
    for (length = 0; length < 80 && dialog->text[length]; ++length)
        ;
    if (length == 80)
        return 0;
    if (dialog->kind == FM_DIALOG_TEXT) {
        if (dialog->page_count < 0 || dialog->page_count > 18 ||
            dialog->page_count > dialog->panel.rect.height - 4)
            return 0;
        for (index = 0; index < dialog->page_count; ++index) {
            if (!dialog->page_lines[index])
                return 0;
        }
    }
    if (dialog->kind == FM_DIALOG_INPUT) {
        for (length = 0; length < FM_INPUT_CAPACITY && dialog->input[length]; ++length)
            ;
        if (length == FM_INPUT_CAPACITY || dialog->position < 0 || dialog->position > length)
            return 0;
    }
    if (dialog->kind == FM_DIALOG_PICK) {
        if (dialog->count < 0 || dialog->count > 16 || dialog->first < 0)
            return 0;
        if (!dialog->count) {
            if (dialog->first || dialog->selected != FM_NO_SELECTION)
                return 0;
        } else if (dialog->first >= dialog->count || dialog->selected < dialog->first ||
                   dialog->selected >= dialog->count ||
                   dialog->selected >=
                       dialog->first + dialog->panel.rect.height - 4 - (dialog->footer[0] ? 2 : 0))
            return 0;
        for (index = 0; index < dialog->count; ++index) {
            if (!dialog->options[index])
                return 0;
        }
    }
    return 1;
}

int fm_validate(FM_CONST struct fm_model *model)
{
    if (!model || !fm_device_valid(&model->device) || !fm_files_valid(&model->files) ||
        !fm_actions_valid(&model->actions) || !fm_help_valid(&model->help) ||
        !fm_dialog_valid(&model->dialog) ||
        (model->files.focused != 0 && model->files.focused != 1) ||
        (model->actions.focused != 0 && model->actions.focused != 1) ||
        model->files.focused + model->actions.focused != 1)
        return FM_BAD_MODEL;
    return FM_SUCCESS;
}

/* Controller-only data is validated separately: standalone view models do not
 * need a catalogue or a job service. */
int fm_control_valid(FM_CONST struct fm_model *model)
{
    int index;

    if (!model || model->catalog_count < 0 || model->catalog_count > FM_MAX_FILES ||
        (model->catalog_count && !model->catalog))
        return 0;
    if (model->catalog != model->files.validated_items ||
        model->catalog_count != model->files.validated_count) {
        for (index = 0; index < model->catalog_count; ++index) {
            if (!model->catalog[index].name)
                return 0;
        }
    }
    if (model->dialog.kind == FM_DIALOG_PICK &&
        (model->dialog.purpose != FM_PURPOSE_DIRECTORY_SLOT || model->dialog.count < 1 ||
         model->dialog.count > 3))
        return 0;
    if (model->dialog.kind == FM_DIALOG_INPUT && model->dialog.purpose != FM_PURPOSE_SUBSET &&
        model->dialog.purpose != FM_PURPOSE_COMMAND &&
        model->dialog.purpose != FM_PURPOSE_COPY_DESTINATION &&
        model->dialog.purpose != FM_PURPOSE_RENAME)
        return 0;
    if (model->job_active != 0 && model->job_active != 1)
        return 0;
    if (model->job_active) {
        if (!model->job.token || model->job.total < 0 || model->job.total > FM_MAX_FILES ||
            model->job.failed != 0 || model->job.completed < 0 ||
            model->job.completed > model->job.total)
            return 0;
        if (model->job.state == FM_JOB_DECISION) {
            if (!model->job.decision || model->dialog.kind != FM_DIALOG_CONFIRM)
                return 0;
        } else if (model->job.state == FM_JOB_PROGRESS) {
            if (model->job.decision || model->dialog.kind != FM_DIALOG_PROGRESS)
                return 0;
        } else if (model->job.state == FM_JOB_INPUT) {
            if (!model->job.decision || model->dialog.kind != FM_DIALOG_INPUT)
                return 0;
        } else if (model->job.state == FM_JOB_VIEW) {
            if (model->job.decision || model->dialog.kind != FM_DIALOG_TEXT)
                return 0;
        } else {
            return 0;
        }
    } else if (model->dialog.kind == FM_DIALOG_CONFIRM ||
               model->dialog.kind == FM_DIALOG_PROGRESS) {
        return 0;
    }
    return 1;
}

/* Target policy is independent of any command, dialog, renderer or backend.
 * Count first so a rejected single-file action cannot leave a partial request. */
int fm_resolve_targets(FM_CONST struct fm_files_model *files, int policy,
                       struct fm_request *request)
{
    int index, count;

    if (!request)
        return FM_BAD_MODEL;
    request->count = 0;
    if (!fm_files_valid(files) || policy < FM_TARGET_NONE || policy > FM_TARGET_MANY)
        return FM_BAD_MODEL;
    if (policy == FM_TARGET_NONE)
        return FM_SUCCESS;
    count = 0;
    for (index = 0; index < files->count; ++index) {
        if (fm_marked(files, index))
            ++count;
    }
    if (!count && files->cursor.selected != FM_NO_SELECTION) {
        request->files[0] = files->cursor.selected;
        request->count = 1;
        return FM_SUCCESS;
    }
    if (!count)
        return FM_NO_TARGETS;
    if (policy == FM_TARGET_ONE && count > 1)
        return FM_TOO_MANY_TARGETS;
    for (index = 0; index < files->count; ++index) {
        if (fm_marked(files, index))
            request->files[request->count++] = index;
    }
    return FM_SUCCESS;
}

/* Shared cursor policy; independent of fixtures and filesystem access. */
void fm_cursor_move(struct fm_cursor *cursor, int count, int delta, int columns, int rows)
{
    int next, capacity;

    if (!count)
        return;
    next = cursor->selected == FM_NO_SELECTION ? 0 : cursor->selected + delta;
    if (next < 0)
        next = 0;
    if (next >= count)
        next = count - 1;
    cursor->selected = next;
    capacity = columns * rows;
    if (next < cursor->first)
        cursor->first = (next / columns) * columns;
    if (next >= cursor->first + capacity)
        cursor->first = (next / columns - rows + 1) * columns;
}
