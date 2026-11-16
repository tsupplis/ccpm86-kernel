/* Text paging action. The filesystem returns bytes; this module interprets
 * CR/LF, tabs and CP/M text EOF. Output is a page model consumed by the text overlay.
 * Long lines wrap, and a CR/LF pair split across reads/pages stays one newline.
 */
#include "type.h"

int fm_type_close(struct fm_type *state)
{
    int result;

    if (!state)
        return FM_FS_ERROR;
    if (!state->opened)
        return FM_FS_OK;
    state->opened = 0;
    result = (*state->filesystem->file_close)(state->filesystem->context, state->handle);
    if (result != FM_FS_OK)
        state->status = FM_FS_ERROR;
    return result;
}

int fm_type_begin(struct fm_type *state, struct fm_filesystem *filesystem,
                  FM_CONST struct fm_location *location, FM_CONST char *name)
{
    int result;

    if (!state || !filesystem || !location || !name || !filesystem->file_open ||
        !filesystem->file_read || !filesystem->file_close)
        return FM_FS_ERROR;
    /* State must initially be zeroed by its owner. Do not overwrite a live
     * handle: a second invocation must close the previous action explicitly. */
    if (state->opened)
        return FM_FS_ERROR;
    state->filesystem = filesystem;
    state->buffered = state->position = state->skip_lf = 0;
    state->pending_spaces = state->stream_end = state->wrapped = 0;
    state->rows = state->eof = 0;
    state->status = FM_FS_OK;
    result = (*filesystem->file_open)(filesystem->context, location, name, &state->handle);
    if (result != FM_FS_OK) {
        state->status = result;
        return result;
    }
    state->opened = 1;
    return FM_FS_OK;
}

/* A malformed adapter result is an error, never a fabricated end of file. */
static int next_byte(struct fm_type *state, int *value)
{
    int result, count;

    if (state->position == state->buffered) {
        if (state->stream_end)
            return FM_FS_END;
        count = 0;
        result = (*state->filesystem->file_read)(state->filesystem->context, state->handle,
                                                 state->buffer, sizeof(state->buffer), &count);
        if (count < 0 || count > (int)sizeof(state->buffer) ||
            (result != FM_FS_OK && result != FM_FS_END) || (result == FM_FS_OK && count == 0))
            return FM_FS_ERROR;
        if (!count)
            return FM_FS_END;
        state->stream_end = result == FM_FS_END;
        state->buffered = count;
        state->position = 0;
    }
    *value = state->buffer[state->position++] & 255;
    return FM_FS_OK;
}

int fm_type_next(struct fm_type *state)
{
    int row, column, value, result, spaces, started;

    if (!state || state->status != FM_FS_OK)
        return FM_FS_ERROR;
    if (state->eof)
        return FM_FS_END;
    if (!state->opened)
        return FM_FS_ERROR;
    started = 0;
    row = column = 0;
    while (row < FM_TEXT_ROWS) {
        if (state->pending_spaces) {
            value = ' ';
            --state->pending_spaces;
            result = FM_FS_OK;
        } else {
            result = next_byte(state, &value);
        }
        if (result == FM_FS_ERROR) {
            state->status = FM_FS_ERROR;
            fm_type_close(state);
            return FM_FS_ERROR;
        }
        if (result == FM_FS_END || value == 26) {
            state->eof = 1;
            if (fm_type_close(state) != FM_FS_OK)
                return FM_FS_ERROR;
            return started ? FM_FS_OK : FM_FS_END;
        }
        if (state->skip_lf) {
            state->skip_lf = 0;
            if (value == 10)
                continue;
        }
        if ((value == 13 || value == 10) && state->wrapped) {
            state->skip_lf = value == 13;
            state->wrapped = 0;
            continue;
        }
        /* Skip the prior page's CR/LF before discarding it. EOF, including
         * CP/M ^Z, leaves that page available without a second page buffer. */
        if (!started) {
            for (row = 0; row < FM_TEXT_ROWS; ++row)
                state->lines[row][0] = 0;
            row = 0;
            state->rows = 0;
            started = 1;
        }
        if (value == 13 || value == 10) {
            state->skip_lf = value == 13;
            state->rows = ++row;
            column = 0;
            continue;
        }
        state->wrapped = 0;
        spaces = value == 9 ? 8 - column % 8 : 1;
        while (spaces > 0 && row < FM_TEXT_ROWS) {
            --spaces;
            state->lines[row][column++] = value == 9                   ? ' '
                                          : value < 32 || value == 127 ? '.'
                                                                       : value;
            state->lines[row][column] = 0;
            state->rows = row + 1;
            if (column == FM_TEXT_COLUMNS) {
                ++row;
                column = 0;
                state->wrapped = 1;
            }
        }
        if (spaces > 0)
            state->pending_spaces = spaces;
    }
    return FM_FS_OK;
}

/* Invocation adapter: filesystem work and text paging only. The shared
 * controller handles keys, validates tokens/generation and presents results. */
#include <stdio.h>

static int open_target(struct fm_type_job *job)
{
    int index;
    index = job->request->files[job->target];
    job->page_number = -1;
    job->goal = job->ending = 0;
    job->loading = 1;
    return fm_type_begin(&job->reader, job->filesystem, job->location, job->entries[index].name);
}

static int type_invoke(void *context, FM_CONST struct fm_request *request)
{
    struct fm_type_job *job;
    int index, selected;
    job = (struct fm_type_job *)context;
    if (!job->filesystem || !job->location || !job->entries || job->count < 1 ||
        job->count > FM_MAX_FILES || job->active || !request || request->command != FM_CMD_TYPE ||
        !request->token || request->count < 1 || request->count > FM_MAX_FILES)
        return FM_BAD_MODEL;
    /* Validate every target before opening the first file. Mixed directory/file
     * selections fail as a whole, rather than stopping halfway through Type. */
    for (index = 0; index < request->count; ++index) {
        selected = request->files[index];
        if (selected < 0 || selected >= job->count || job->entries[selected].directory ||
            (index && selected <= request->files[index - 1]))
            return FM_BAD_MODEL;
    }
    job->request = request;
    job->target = 0;
    job->active = 1;
    return open_target(job);
}

static int type_cancel(void *context)
{
    struct fm_type_job *job;
    job = (struct fm_type_job *)context;
    job->active = 0;
    job->request = 0;
    return fm_type_close(&job->reader);
}

static int type_advance(void *context, FM_CONST struct fm_reply *reply,
                        struct fm_job_result *result)
{
    struct fm_type_job *job;
    int row, status, destination;
    job = (struct fm_type_job *)context;
    if (!job->active || job->target >= job->request->count || !reply || !result ||
        reply->token != job->request->token || reply->decision || reply->answer < FM_REPLY_NONE ||
        reply->answer > FM_REPLY_LAST || reply->answer == FM_REPLY_ACCEPT)
        return FM_BAD_MODEL;
    if (!job->loading) {
        if (reply->answer == FM_REPLY_NEXT) {
            if (job->reader.eof) {
                ++job->target;
                if (job->target < job->request->count && open_target(job))
                    return FM_JOB_ERROR;
            } else {
                if (job->page_number == 32766)
                    return FM_JOB_ERROR;
                job->goal = job->page_number + 1;
                job->loading = 1;
            }
        } else if (reply->answer == FM_REPLY_PREVIOUS || reply->answer == FM_REPLY_FIRST) {
            destination = reply->answer == FM_REPLY_FIRST ? 0 : job->page_number - 1;
            if (destination < 0)
                destination = 0;
            if (destination != job->page_number) {
                if (fm_type_close(&job->reader) || open_target(job))
                    return FM_JOB_ERROR;
                job->goal = destination;
            }
        } else if (reply->answer == FM_REPLY_LAST && !job->reader.eof) {
            job->ending = job->loading = 1;
        }
    }
    result->token = job->request->token;
    result->generation = job->request->generation;
    result->decision = 0;
    result->completed = job->target;
    result->failed = 0;
    result->total = job->request->count;
    result->page = 0;
    result->prompt = 0;
    if (job->target == job->request->count) {
        result->state = FM_JOB_COMPLETE;
        return FM_SUCCESS;
    }
    if (job->loading) {
        status = fm_type_next(&job->reader);
        if (status != FM_FS_OK && status != FM_FS_END)
            return FM_JOB_ERROR;
        /* EOF without a new page preserves both its text and page number. */
        if (status != FM_FS_END || job->page_number < 0) {
            if (job->page_number == 32766)
                return FM_JOB_ERROR;
            ++job->page_number;
        }
        job->loading = !job->reader.eof && (job->ending || job->page_number < job->goal);
    }
    if (job->loading) {
        result->state = FM_JOB_PROGRESS;
        return FM_SUCCESS;
    }
    snprintf(job->title, sizeof(job->title), "Type: %.12s - page %d (%d/%d)",
             job->entries[job->request->files[job->target]].name, job->page_number + 1,
             job->target + 1, job->request->count);
    job->page.title = job->title;
    job->page.hint = job->reader.eof
                         ? "EOF. Enter: next file/return. PgUp/Home: back. Esc: return."
                         : "PgDn/Space: next. PgUp: previous. Home/End: first/last. Esc: return.";
    job->page.count = job->reader.rows;
    for (row = 0; row < job->reader.rows; ++row)
        job->page.lines[row] = job->reader.lines[row];
    result->page = &job->page;
    result->state = FM_JOB_VIEW;
    return FM_SUCCESS;
}

void fm_type_job_bind(struct fm_invoker *port, struct fm_type_job *job,
                      struct fm_filesystem *filesystem, FM_CONST struct fm_location *location,
                      FM_CONST struct fm_entry *entries, int count)
{
    job->filesystem = filesystem;
    job->location = location;
    job->entries = entries;
    job->count = count;
    job->active = job->reader.opened = 0;
    port->context = job;
    port->invoke = type_invoke;
    port->advance = type_advance;
    port->cancel = type_cancel;
    port->error = 0;
    port->interactive = 1;
}
