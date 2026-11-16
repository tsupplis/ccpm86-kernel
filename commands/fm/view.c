/* Four passive views and their shared drawing primitives.
 * There are no command labels, fixture names, colours or screen coordinates
 * here. Only border glyphs and item spacing are rendering policy.
 */
#include "fm.h"

/* Paint text and trailing padding once; selection includes the whole field.
 * The adapter receives counted runs, avoiding temporary string construction.
 */
static int field(FM_CONST struct fm_panel *panel, struct fm_display *display, int x, int y,
                 int width, int attribute, FM_CONST char *text)
{
    static char spaces[] =
        "                                                                                ";
    int length;

    if (!display || !display->write || x < 0 || y < 0 || y >= panel->rect.height ||
        x >= panel->rect.width || width < 0)
        return FM_BAD_MODEL;
    if (width > panel->rect.width - x)
        width = panel->rect.width - x;
    for (length = 0; length < width && text[length]; ++length)
        ;
    if (length && (*display->write)(display->context, panel->rect.x + x, panel->rect.y + y,
                                    attribute, text, length))
        return FM_OUTPUT_ERROR;
    if (length < width && (*display->write)(display->context, panel->rect.x + x + length,
                                            panel->rect.y + y, attribute, spaces, width - length))
        return FM_OUTPUT_ERROR;
    return FM_SUCCESS;
}

/* Centre within a field without altering or copying model text. */
static int centred(FM_CONST struct fm_panel *panel, struct fm_display *display, int x, int y,
                   int width, int attribute, FM_CONST char *text)
{
    int length, offset;

    for (length = 0; length < width && text[length]; ++length)
        ;
    offset = (width - length) / 2;
    return field(panel, display, x + offset, y, width - offset, attribute, text);
}

static int clear_panel(FM_CONST struct fm_panel *panel, struct fm_display *display, int inset)
{
    int row, status;

    for (row = inset; row < panel->rect.height - inset; ++row) {
        status = field(panel, display, inset, row, panel->rect.width - 2 * inset,
                       panel->style.normal, "");
        if (status)
            return status;
    }
    return FM_SUCCESS;
}

static int frame(FM_CONST struct fm_panel *panel, struct fm_display *display)
{
    char rule[FM_COLUMNS + 1];
    int column, row, status;

    for (column = 0; column < panel->rect.width; ++column)
        rule[column] = (char)196;
    rule[0] = (char)218;
    rule[column - 1] = (char)191;
    rule[column] = 0;
    status = field(panel, display, 0, 0, panel->rect.width, panel->style.border, rule);
    rule[0] = (char)192;
    rule[column - 1] = (char)217;
    if (!status)
        status = field(panel, display, 0, panel->rect.height - 1, panel->rect.width,
                       panel->style.border, rule);
    for (row = 1; !status && row < panel->rect.height - 1; ++row) {
        status = field(panel, display, 0, row, 1, panel->style.border, "\263");
        if (!status)
            status =
                field(panel, display, panel->rect.width - 1, row, 1, panel->style.border, "\263");
    }
    return status;
}

int fm_device_render(FM_CONST struct fm_device_model *model, struct fm_display *display)
{
    FM_CONST struct fm_panel *panel;
    int status, divider, row;

    if (!fm_device_valid(model))
        return FM_BAD_MODEL;
    panel = &model->panel;
    divider = model->media_width + 1;
    status = clear_panel(panel, display, 1);
    if (!status)
        status = frame(panel, display);
    for (row = 1; !status && row < panel->rect.height - 1; ++row)
        status = field(panel, display, divider, row, 1, panel->style.border, "\263");
    if (!status)
        status = field(panel, display, divider, 0, 1, panel->style.border, "\302");
    if (!status)
        status =
            field(panel, display, divider, panel->rect.height - 1, 1, panel->style.border, "\301");
    if (!status)
        status =
            centred(panel, display, 1, 1, model->media_width, panel->style.normal, model->media);
    if (!status)
        status =
            centred(panel, display, 1, 2, model->media_width, panel->style.normal, model->capacity);
    if (!status)
        status = field(panel, display, divider + 2, 1, panel->rect.width - divider - 3,
                       panel->style.normal, model->location);
    return status;
}

int fm_files_render(const struct fm_files_model *model, struct fm_display *display)
{
    FM_CONST struct fm_panel *panel;
    int status, slot, index, width, rows, x, y, attribute, length, has_marks;

    if (!fm_files_valid(model))
        return FM_BAD_MODEL;
    panel = &model->panel;
    status = clear_panel(panel, display, 1);
    if (!status)
        status = frame(panel, display);
    if (!status)
        status = centred(panel, display, 1, 1, panel->rect.width - 2, panel->style.normal,
                         model->heading);
    if (status)
        return status;
    if (model->count == 0)
        return field(panel, display, 1, 2, panel->rect.width - 2, panel->style.normal,
                     model->empty_text);

    /* Explicit marks override the implicit current-file target, including
     * marks outside the viewport. Focus is a separate presentation state. */
    has_marks = 0;
    for (index = 0; index < model->count; ++index)
        if (fm_marked(model, index)) {
            has_marks = 1;
            break;
        }

    width = (panel->rect.width - 2) / model->columns;
    rows = panel->rect.height - 4;
    for (slot = 0; slot < rows * model->columns; ++slot) {
        index = model->cursor.first + slot;
        if (index >= model->count)
            break;
        x = 1 + (slot % model->columns) * width;
        y = 2 + slot / model->columns;
        attribute = fm_marked(model, index) || (!has_marks && index == model->cursor.selected)
                        ? panel->style.marked
                        : panel->style.normal;
        if (model->focused && index == model->cursor.selected)
            attribute = panel->style.selected;
        /* Reserve one glyph for the keyboard cursor, independent of colour. */
        status = field(panel, display, x, y, 1, panel->style.normal,
                       fm_marked(model, index)                             ? "*"
                       : model->focused && index == model->cursor.selected ? "\020"
                                                                           : " ");
        if (!status)
            status =
                field(panel, display, x + 1, y, width - 2, attribute, model->items[index].name);
        length = strlen(model->items[index].name);
        if (!status && model->items[index].directory && length < width - 2)
            status = field(panel, display, x + 1 + length, y, 1, attribute, "\\");
        if (!status && model->details && width > 16 && model->items[index].detail)
            status =
                field(panel, display, x + 14, y, width - 15, attribute, model->items[index].detail);
        if (status)
            return status;
    }
    if (model->cursor.first > 0) {
        status = field(panel, display, panel->rect.width - 1, 1, 1, panel->style.border, "\036");
        if (status)
            return status;
    }
    if (model->cursor.first + rows * model->columns < model->count)
        return field(panel, display, panel->rect.width - 1, panel->rect.height - 2, 1,
                     panel->style.border, "\037");
    return FM_SUCCESS;
}

int fm_actions_render(const struct fm_actions_model *model, struct fm_display *display)
{
    FM_CONST struct fm_panel *panel;
    int status, slot, index, attribute, inset, rows, width, x, y;

    if (!fm_actions_valid(model))
        return FM_BAD_MODEL;
    panel = &model->panel;
    inset = panel->rect.height == 1 ? 0 : 1;
    rows = panel->rect.height - 2 * inset;
    width = (panel->rect.width - 2 * inset) / model->columns;
    status = clear_panel(panel, display, inset);
    if (!status && inset)
        status = frame(panel, display);
    if (status)
        return status;
    if (model->count == 0)
        return field(panel, display, inset, inset, panel->rect.width - 2 * inset,
                     panel->style.normal, model->empty_text);
    for (slot = 0; slot < rows * model->columns; ++slot) {
        index = model->cursor.first + slot;
        if (index >= model->count)
            break;
        x = inset + (slot % model->columns) * width;
        y = inset + slot / model->columns;
        attribute = model->focused && index == model->cursor.selected ? panel->style.selected
                                                                      : panel->style.normal;
        status = field(panel, display, x, y, 1, panel->style.normal,
                       model->focused && index == model->cursor.selected ? "\020" : " ");
        if (!status)
            status = field(panel, display, x + 1, y, width - 1, attribute,
                           model->items[index].id == model->label_command && model->label
                               ? model->label
                               : model->items[index].label);
        if (status)
            return status;
    }
    if (inset && model->cursor.first > 0) {
        status = field(panel, display, panel->rect.width - 1, 1, 1, panel->style.border, "\036");
        if (status)
            return status;
    }
    if (inset && model->cursor.first + rows * model->columns < model->count)
        return field(panel, display, panel->rect.width - 1, panel->rect.height - 2, 1,
                     panel->style.border, "\037");
    return FM_SUCCESS;
}

int fm_help_render(FM_CONST struct fm_help_model *model, struct fm_display *display)
{
    FM_CONST struct fm_hint *hint;
    int index, row, status;

    if (!fm_help_valid(model))
        return FM_BAD_MODEL;
    /* Status and legend rows cover this borderless panel completely. */
    status = field(&model->panel, display, 0, 0, model->panel.rect.width, model->panel.style.normal,
                   model->status);
    /* The legend has its own configured background, including blank cells. */
    for (row = 1; !status && row < model->panel.rect.height; ++row)
        status = field(&model->panel, display, 0, row, model->panel.rect.width,
                       model->panel.style.accent, "");
    for (index = 0; !status && index < model->count; ++index) {
        hint = &model->hints[index];
        status = field(&model->panel, display, hint->column, hint->row, strlen(hint->text),
                       model->panel.style.accent, hint->text);
    }
    return status;
}

/* Word-wrap bounded dialog text. The temporary buffer holds one display line. */
static int message_lines(FM_CONST struct fm_panel *panel, struct fm_display *display,
                         FM_CONST char *text, int width)
{
    char line[FM_COLUMNS + 1];
    int row, length, split, index, status;

    for (row = 1; row <= 2 && *text; ++row) {
        for (length = 0; length < width && text[length]; ++length)
            ;
        split = length;
        if (text[length]) {
            while (split > 0 && text[split] != ' ')
                --split;
            if (!split)
                split = length;
        }
        for (index = 0; index < split; ++index)
            line[index] = text[index];
        line[index] = 0;
        status = field(panel, display, 2, row, width, panel->style.normal, line);
        if (status)
            return status;
        text += split;
        while (*text == ' ')
            ++text;
    }
    return FM_SUCCESS;
}

static int dialog_render(FM_CONST struct fm_dialog *dialog, struct fm_display *display)
{
    FM_CONST struct fm_panel *panel;
    int status, row, index, attribute, width, title_width, start;
    char cursor[2];

    if (!dialog->kind)
        return FM_SUCCESS;
    if (!fm_dialog_valid(dialog))
        return FM_BAD_MODEL;
    panel = &dialog->panel;
    width = panel->rect.width - 4;
    title_width = strlen(dialog->title);
    if (title_width > width)
        title_width = width;
    status = clear_panel(panel, display, 1);
    if (!status)
        status = frame(panel, display);
    if (!status)
        status = field(panel, display, 2, 0, title_width, panel->style.border, dialog->title);
    if (!status)
        status = message_lines(panel, display, dialog->text, width);
    if (dialog->kind == FM_DIALOG_TEXT) {
        for (row = 0; !status && row < dialog->page_count; ++row)
            status = field(panel, display, 2, row + 3, width, panel->style.normal,
                           dialog->page_lines[row]);
    } else if (dialog->kind == FM_DIALOG_PICK) {
        for (row = 3; !status && row < panel->rect.height - 1 - (dialog->footer[0] ? 2 : 0);
             ++row) {
            index = dialog->first + row - 3;
            if (index >= dialog->count)
                break;
            attribute = index == dialog->selected ? panel->style.selected : panel->style.normal;
            status = field(panel, display, 2, row, width, attribute, dialog->options[index]);
        }
        if (!status && !dialog->count)
            status = field(panel, display, 2, 3, width, panel->style.normal, "No subdirectories");
        if (!status && dialog->more_before)
            status =
                field(panel, display, panel->rect.width - 1, 3, 1, panel->style.border, "\036");
        if (!status && dialog->more_after)
            status = field(panel, display, panel->rect.width - 1, panel->rect.height - 4, 1,
                           panel->style.border, "\037");
        for (row = 0; !status && row < 2; ++row)
            if (dialog->footer[row])
                status = field(panel, display, 2, panel->rect.height - 3 + row, width,
                               panel->style.normal, dialog->footer[row]);
    } else if (dialog->kind == FM_DIALOG_INPUT) {
        /* Keep the insertion cursor visible even at the maximum input length. */
        start = dialog->position >= width ? dialog->position - width + 1 : 0;
        if (!status)
            status = field(panel, display, 2, 4, width, panel->style.normal, dialog->input + start);
        cursor[0] = dialog->input[dialog->position] ? dialog->input[dialog->position] : ' ';
        cursor[1] = 0;
        if (!status)
            status = field(panel, display, 2 + dialog->position - start, 4, 1,
                           panel->style.selected, cursor);
    }
    return status;
}

int fm_render(FM_CONST struct fm_model *model, struct fm_display *display)
{
    int status, end_status;

    status = fm_validate(model);
    if (status || !display || !display->begin || !display->write || !display->end)
        return FM_BAD_MODEL;
    if ((*display->begin)(display->context))
        return FM_OUTPUT_ERROR;
    status = fm_device_render(&model->device, display);
    if (!status)
        status = fm_files_render(&model->files, display);
    /* Device and Files share their horizontal edge in the original layout.
     * Reconnect the divider after Files has painted that shared edge. */
    if (!status && model->files.panel.rect.x == model->device.panel.rect.x &&
        model->files.panel.rect.y ==
            model->device.panel.rect.y + model->device.panel.rect.height - 1) {
        status =
            field(&model->files.panel, display, 0, 0, 1, model->files.panel.style.border, "\303");
        if (!status)
            status = field(&model->files.panel, display, model->device.media_width + 1, 0, 1,
                           model->files.panel.style.border, "\301");
        if (!status)
            status = field(&model->files.panel, display, model->files.panel.rect.width - 1, 0, 1,
                           model->files.panel.style.border, "\264");
    }
    if (!status)
        status = fm_actions_render(&model->actions, display);
    if (!status)
        status = fm_help_render(&model->help, display);
    if (!status)
        status = dialog_render(&model->dialog, display);
    end_status = (*display->end)(display->context);
    return status ? status : (end_status ? FM_OUTPUT_ERROR : FM_SUCCESS);
}
