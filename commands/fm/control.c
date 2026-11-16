/* Input controller. No rendering, console access or filesystem operations. */
#include <stdio.h>
#include "fm.h"

/* Both parameterised jobs and viewers start from the same immutable target
 * snapshot. The model retains the immutable request until the job ends. */
int fm_job_start(struct fm_model *model, struct fm_invoker *invoker)
{
    struct fm_request *request;
    int status, index;
    if (!model || !invoker || !invoker->invoke || !invoker->advance || !invoker->cancel ||
        model->job_active || model->pending.generation != model->generation ||
        model->pending.count < 0 || model->pending.count > FM_MAX_FILES)
        return FM_BAD_MODEL;
    request = &model->pending;
    for (index = 0; index < request->count; ++index)
        if (request->files[index] < 0 || request->files[index] >= model->files.count ||
            (index && request->files[index] <= request->files[index - 1]))
            return FM_BAD_MODEL;
    if (!++model->next_job_token)
        ++model->next_job_token;
    request->token = model->next_job_token;
    status = (*invoker->invoke)(invoker->context, request);
    if (status) {
        (*invoker->cancel)(invoker->context);
        fm_dialog_info(model, "Action failed", "The action could not start. Selection retained.");
        return FM_JOB_ERROR;
    }
    model->job_active = 1;
    model->job.token = request->token;
    model->job.generation = request->generation;
    model->job.decision = 0;
    model->job.state = FM_JOB_PROGRESS;
    model->job.completed = model->job.failed = 0;
    model->job.total = request->count;
    model->job.page = 0;
    model->dialog.kind = FM_DIALOG_PROGRESS;
    model->dialog.title = "Action in progress";
    strcpy(model->dialog.text, "Working. Esc cancels.");
    return FM_SUCCESS;
}

/* Keep the last valid progress snapshot when an adapter fails. Cancellation
 * releases the adapter before the controller exposes a terminal outcome. */
static int job_error(struct fm_model *model, struct fm_invoker *invoker)
{
    (*invoker->cancel)(invoker->context);
    model->job_active = 0;
    model->job.state = FM_JOB_FAILED;
    model->job.decision = 0;
    fm_dialog_info(model, "Action failed",
                   invoker->error ? (*invoker->error)(invoker->context)
                                  : "Service or cleanup failed. Progress and selection retained.");
    return FM_JOB_ERROR;
}

static int job_step(struct fm_model *model, struct fm_invoker *invoker, int answer)
{
    struct fm_job_result result;
    struct fm_reply reply;
    int status, row;

    if (model->generation != model->job.generation)
        return job_error(model, invoker);
    reply.token = model->job.token;
    reply.decision =
        (answer == FM_REPLY_ACCEPT || answer == FM_REPLY_TEXT) ? model->job.decision : 0;
    reply.answer = answer;
    reply.text = answer == FM_REPLY_TEXT ? model->dialog.input : 0;
    /* An adapter reporting success must fill every result field. Initializing
     * an invalid result makes incomplete adapters fail closed without reading
     * indeterminate stack data. */
    result.page = 0;
    result.prompt = 0;
    result.token = result.generation = result.decision = 0;
    result.state = result.completed = result.failed = result.total = -1;
    status = (*invoker->advance)(invoker->context, &reply, &result);
    if (status)
        return job_error(model, invoker);
    if (result.token != model->job.token || result.generation != model->job.generation ||
        result.total != model->job.total || result.completed < model->job.completed ||
        result.failed < 0 || result.completed > result.total ||
        result.failed > result.total - result.completed || result.state < FM_JOB_PROGRESS ||
        result.state > FM_JOB_INPUT ||
        ((result.state == FM_JOB_DECISION || result.state == FM_JOB_INPUT)
             ? !result.decision
             : result.decision != 0) ||
        (answer == FM_REPLY_ACCEPT && result.state == FM_JOB_DECISION &&
         result.decision == reply.decision) ||
        (result.state == FM_JOB_COMPLETE && result.completed + result.failed != result.total) ||
        ((result.state == FM_JOB_PROGRESS || result.state == FM_JOB_DECISION ||
          result.state == FM_JOB_VIEW || result.state == FM_JOB_INPUT) &&
         result.failed))
        return job_error(model, invoker);
    if (result.state == FM_JOB_VIEW) {
        if (!invoker->interactive || !result.page || !result.page->title || !result.page->hint ||
            result.page->count < 0 || result.page->count > 18)
            return job_error(model, invoker);
        for (row = 0; row < result.page->count; ++row)
            if (!result.page->lines[row])
                return job_error(model, invoker);
    }
    if ((result.state == FM_JOB_DECISION || result.state == FM_JOB_INPUT) && !result.prompt)
        return job_error(model, invoker);
    model->job = result;
    if (result.state == FM_JOB_VIEW) {
        model->dialog.kind = FM_DIALOG_TEXT;
        model->dialog.panel.rect.x = model->dialog.panel.rect.y = 0;
        model->dialog.panel.rect.width = 80;
        model->dialog.panel.rect.height = 24;
        model->dialog.title = result.page->title;
        sprintf(model->dialog.text, "%.79s", result.page->hint);
        model->dialog.page_count = result.page->count;
        for (row = 0; row < result.page->count; ++row)
            model->dialog.page_lines[row] = result.page->lines[row];
    } else if (result.state == FM_JOB_INPUT) {
        fm_dialog_close(model);
        model->dialog.kind = FM_DIALOG_INPUT;
        model->dialog.purpose = FM_PURPOSE_COMMAND;
        model->dialog.title = "Action input";
        snprintf(model->dialog.text, sizeof(model->dialog.text), "%s", result.prompt);
        model->dialog.input[0] = 0;
        model->dialog.position = 0;
    } else if (result.state == FM_JOB_DECISION) {
        model->dialog.kind = FM_DIALOG_CONFIRM;
        model->dialog.title = "Confirm action";
        sprintf(model->dialog.text, "%.79s", result.prompt);
    } else if (result.state == FM_JOB_PROGRESS) {
        fm_dialog_close(model);
        model->dialog.kind = FM_DIALOG_PROGRESS;
        model->dialog.title = "Action in progress";
        if (result.prompt)
            sprintf(model->dialog.text, "%.79s", result.prompt);
        else
            sprintf(model->dialog.text, "Processed %d of %d files. Esc cancels.", result.completed,
                    result.total);
    } else {
        if ((*invoker->cancel)(invoker->context))
            return job_error(model, invoker);
        model->job_active = 0;
        if (invoker->interactive && result.state == FM_JOB_COMPLETE) {
            fm_dialog_close(model);
            return FM_SUCCESS;
        }
        if (result.prompt)
            snprintf(model->status, sizeof(model->status), "%s", result.prompt);
        else
            snprintf(model->status, sizeof(model->status),
                     "%d completed, %d failed, %d unprocessed.", result.completed, result.failed,
                     result.total - result.completed - result.failed);
        model->help.status = model->status;
        fm_dialog_info(model,
                       result.state == FM_JOB_COMPLETE    ? "Action complete"
                       : result.state == FM_JOB_CANCELLED ? "Action cancelled"
                                                          : "Action failed",
                       result.prompt ? result.prompt : model->status);
    }
    return FM_SUCCESS;
}

/* Shared bounded input editor; job replies borrow the text for one step. */
static int input_key(struct fm_model *model, struct fm_invoker *invoker,
                     struct fm_action_handler *handler, int key)
{
    struct fm_dialog *dialog = &model->dialog;
    int length, position, index;

    length = strlen(dialog->input);
    position = dialog->position;
    if (key == FM_KEY_LEFT && position > 0)
        --dialog->position;
    if (key == FM_KEY_RIGHT && position < length)
        ++dialog->position;
    if (key == FM_KEY_HOME)
        dialog->position = 0;
    if (key == FM_KEY_END)
        dialog->position = length;
    if (key == 8 && position > 0) {
        for (index = position - 1; index < length; ++index)
            dialog->input[index] = dialog->input[index + 1];
        --dialog->position;
    } else if (key == FM_KEY_DELETE && position < length) {
        for (index = position; index < length; ++index)
            dialog->input[index] = dialog->input[index + 1];
    } else if (key >= 32 && key < 127 && length < FM_INPUT_CAPACITY - 1) {
        for (index = length + 1; index > position; --index)
            dialog->input[index] = dialog->input[index - 1];
        dialog->input[position] = key;
        ++dialog->position;
    } else if (key == FM_KEY_ENTER) {
        if (!length && !model->job_active) {
            strcpy(dialog->text, "Enter a value, or Esc to cancel.");
            return FM_SUCCESS;
        }
        if (model->job_active)
            return job_step(model, invoker, FM_REPLY_TEXT);
        if (handler && handler->accept)
            return (*handler->accept)(handler->context, model, invoker);
        fm_dialog_close(model);
    }
    return FM_SUCCESS;
}

static int dialog_key(struct fm_model *model, struct fm_invoker *invoker,
                      struct fm_action_handler *handler, int key)
{
    struct fm_dialog *dialog;

    dialog = &model->dialog;
    if (model->job_active) {
        if (key == FM_KEY_ESCAPE ||
            (dialog->kind == FM_DIALOG_CONFIRM && (key == 'n' || key == 'N'))) {
            if ((*invoker->cancel)(invoker->context))
                return job_error(model, invoker);
            model->job_active = 0;
            model->job.state = FM_JOB_CANCELLED;
            model->job.decision = 0;
            if (invoker->interactive) {
                fm_dialog_close(model);
                return FM_SUCCESS;
            }
            sprintf(model->status, "Cancelled: %d completed, %d unprocessed.", model->job.completed,
                    model->job.total - model->job.completed);
            fm_dialog_info(model, "Action cancelled", model->status);
            return FM_SUCCESS;
        }
        if (dialog->kind == FM_DIALOG_INPUT)
            return input_key(model, invoker, handler, key);
        if (dialog->kind == FM_DIALOG_TEXT) {
            if (key == ' ' || key == FM_KEY_PGDN || key == FM_KEY_ENTER)
                return job_step(model, invoker, FM_REPLY_NEXT);
            if (key == FM_KEY_PGUP)
                return job_step(model, invoker, FM_REPLY_PREVIOUS);
            if (key == FM_KEY_HOME)
                return job_step(model, invoker, FM_REPLY_FIRST);
            if (key == FM_KEY_END)
                return job_step(model, invoker, FM_REPLY_LAST);
            return FM_SUCCESS;
        }
        if (dialog->kind == FM_DIALOG_CONFIRM)
            return key == 'y' || key == 'Y' ? job_step(model, invoker, FM_REPLY_ACCEPT)
                                            : FM_SUCCESS;
        return key == FM_KEY_TICK || key == ' ' ? job_step(model, invoker, FM_REPLY_NONE)
                                                : FM_SUCCESS;
    }
    if (key == FM_KEY_ESCAPE || (dialog->kind == FM_DIALOG_INFO && key == FM_KEY_ENTER)) {
        fm_dialog_close(model);
        return FM_SUCCESS;
    }
    if (dialog->kind == FM_DIALOG_PICK) {
        if (key == FM_KEY_UP && dialog->selected > 0)
            --dialog->selected;
        if (key == FM_KEY_DOWN && dialog->selected + 1 < dialog->count)
            ++dialog->selected;
        if (key == FM_KEY_HOME)
            dialog->selected = 0;
        if (key == FM_KEY_END)
            dialog->selected = dialog->count - 1;
        if (key == FM_KEY_PGUP)
            dialog->selected -= dialog->panel.rect.height - 4;
        if (key == FM_KEY_PGDN)
            dialog->selected += dialog->panel.rect.height - 4;
        if (dialog->selected < 0)
            dialog->selected = 0;
        if (dialog->selected >= dialog->count)
            dialog->selected = dialog->count - 1;
        if (dialog->selected < dialog->first)
            dialog->first = dialog->selected;
        if (dialog->selected >= dialog->first + dialog->panel.rect.height - 4)
            dialog->first = dialog->selected - dialog->panel.rect.height + 5;
        if (key != FM_KEY_ENTER)
            return FM_SUCCESS;
        if (handler && handler->accept)
            return (*handler->accept)(handler->context, model, invoker);
        fm_dialog_close(model);
    } else if (dialog->kind == FM_DIALOG_INPUT) {
        return input_key(model, invoker, handler, key);
    }
    return FM_SUCCESS;
}

/* These bindings share the same command IDs as the injected action list. */
static FM_CONST int function_commands[] = {FM_CMD_HELP,      FM_CMD_TYPE,      0,
                                           FM_CMD_CLEAR,     FM_CMD_RUN,       FM_CMD_EDIT,
                                           FM_CMD_BOOKMARK1, FM_CMD_BOOKMARK2, FM_CMD_DIRECTORY,
                                           FM_CMD_COMMAND};

static void clear_marks(struct fm_model *model)
{
    fm_marks_clear(&model->files);
}

/* Single translation from a key/menu selection to a command identity. */
static int command_key(struct fm_model *model, int key)
{
    if (key >= FM_KEY_F1 && key < FM_KEY_F1 + 10)
        return key == FM_KEY_F1 + 2 ? model->last_command : function_commands[key - FM_KEY_F1];
    if (key == 'c' || key == 'C')
        return FM_CMD_COPY;
    if (key == 24)
        return FM_CMD_CLEAR;
    if (key == FM_KEY_ENTER) {
        if (model->files.focused)
            return FM_CMD_OPEN;
        if (model->actions.cursor.selected != FM_NO_SELECTION)
            return model->actions.items[model->actions.cursor.selected].id;
    }
    return 0;
}

int fm_dispatch_to(struct fm_model *model, struct fm_invoker *invoker,
                   struct fm_action_handler *handler, int key)
{
    int delta, index, rows, columns, count, command;
    struct fm_cursor *cursor;

    if (fm_validate(model) || !fm_control_valid(model))
        return FM_BAD_MODEL;
    if (model->job_active && (!invoker || !invoker->advance || !invoker->cancel))
        return FM_BAD_MODEL;
    if (model->dialog.kind)
        return dialog_key(model, invoker, handler, key);
    if (key == FM_KEY_TICK)
        return FM_SUCCESS;
    if (key == FM_KEY_ESCAPE) {
        model->exit_requested = 1;
        return FM_SUCCESS;
    }
    if (key == FM_KEY_TAB) {
        model->files.focused = !model->files.focused;
        model->actions.focused = !model->files.focused;
        return FM_SUCCESS;
    }
    command = command_key(model, key);
    if (command) {
        if (command == FM_CMD_CLEAR) {
            clear_marks(model);
            return FM_SUCCESS;
        }
        if (handler && handler->execute)
            return (*handler->execute)(handler->context, model, command);
        fm_dialog_info(model, "Action unavailable", "No action controller is bound.");
        return FM_SUCCESS;
    }
    if (model->files.focused && model->files.cursor.selected != FM_NO_SELECTION) {
        index = model->files.cursor.selected;
        if (key == ' ' || key == ',' || key == FM_KEY_INSERT)
            fm_mark_set(&model->files, index, !fm_marked(&model->files, index));
        if (key == FM_KEY_DELETE)
            fm_mark_set(&model->files, index, 0);
    }
    cursor = model->files.focused ? &model->files.cursor : &model->actions.cursor;
    count = model->files.focused ? model->files.count : model->actions.count;
    columns = model->files.focused ? model->files.columns : model->actions.columns;
    rows = model->files.focused
               ? model->files.panel.rect.height - 4
               : model->actions.panel.rect.height - (model->actions.panel.rect.height == 1 ? 0 : 2);
    delta = 0;
    switch (key) {
    case FM_KEY_UP:
        delta = -columns;
        break;
    case FM_KEY_DOWN:
        delta = columns;
        break;
    case FM_KEY_LEFT:
        delta = -1;
        break;
    case FM_KEY_RIGHT:
        delta = 1;
        break;
    case FM_KEY_HOME:
        delta = -count;
        break;
    case FM_KEY_END:
        delta = count;
        break;
    case FM_KEY_PGUP:
        delta = -columns * rows;
        break;
    case FM_KEY_PGDN:
        delta = columns * rows;
        break;
    }
    if (delta)
        fm_cursor_move(cursor, count, delta, columns, rows);
    if (delta && model->actions.focused && cursor->selected >= 0)
        model->help.status = model->actions.items[cursor->selected].help;
    return FM_SUCCESS;
}
