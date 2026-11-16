/* Historical FM text printing: seven-bit text, tab stops every eight columns,
 * unsupported controls become spaces; FF/newlines pass through. FF terminates
 * each opened file. Ctrl-Z (including high-bit form) and FFh end text input.
 * One record per advance bounds work and leaves cancellation to the controller.
 */
#include <stdio.h>
#include "print.h"

static int close_file(struct fm_print_job *job)
{
    int status, feed;
    if (!job->opened)
        return FM_FS_OK;
    job->opened = 0;
    status = job->filesystem->file_close(job->filesystem->context, job->handle);
    feed = job->printer->put(job->printer->context, 12);
    return status || feed ? FM_FS_ERROR : FM_FS_OK;
}

static int cancel(void *context)
{
    struct fm_print_job *job = context;
    int status = close_file(job);
    if (job->attached) {
        job->attached = 0;
        if (job->printer->detach(job->printer->context))
            status = FM_FS_ERROR;
    }
    job->active = 0;
    job->request = 0;
    if (status)
        job->failure = "Could not close the print file or release the printer.";
    return status;
}

static int invoke(void *context, const struct fm_request *request)
{
    struct fm_print_job *job = context;
    int i, index;
    if (job->active || !request || !request->token || request->command != FM_CMD_PRINT ||
        request->count < 1 || request->count > FM_MAX_FILES || !job->entries)
        return FM_BAD_MODEL;
    for (i = 0; i < request->count; ++i) {
        index = request->files[i];
        if (index < 0 || index >= job->count || job->entries[index].directory ||
            (i && index <= request->files[i - 1]))
            return FM_BAD_MODEL;
    }
    job->request = request;
    job->active = 1;
    job->target = 0;
    job->failure = "Printing failed.";
    return FM_SUCCESS;
}

static int print_byte(struct fm_print_job *job, int byte)
{
    int repeat = 1;
    byte &= 127;
    if (byte == 9) {
        repeat = 8 - job->column;
        byte = ' ';
    } else if (byte < 32 && byte != 10 && byte != 12 && byte != 13)
        byte = ' ';
    while (repeat--) {
        if (job->printer->put(job->printer->context, byte))
            return FM_FS_ERROR;
        if (byte == 13)
            job->column = 0;
        else if (byte != 10 && byte != 12)
            job->column = (job->column + 1) & 7;
    }
    return FM_FS_OK;
}

static int advance(void *context, const struct fm_reply *reply, struct fm_job_result *result)
{
    struct fm_print_job *job = context;
    int status, count = 0, i, byte, ended;
    const char *name;
    if (!job->active || !reply || !result || reply->token != job->request->token ||
        reply->answer != FM_REPLY_NONE || reply->decision)
        return FM_BAD_MODEL;
    memset(result, 0, sizeof(*result));
    result->token = job->request->token;
    result->generation = job->request->generation;
    result->total = job->request->count;
    result->completed = job->target;
    result->state = FM_JOB_PROGRESS;
    if (job->target == result->total) {
        result->state = FM_JOB_COMPLETE;
        return FM_SUCCESS;
    }
    if (!job->attached) {
        job->failure = "The current printer is unavailable or already attached.";
        if (job->printer->attach(job->printer->context))
            return FM_FS_ERROR;
        job->attached = 1;
    }
    name = job->entries[job->request->files[job->target]].name;
    if (!job->opened) {
        job->failure = "Cannot open the selected print file.";
        if (job->filesystem->file_open(job->filesystem->context, job->location, name, &job->handle))
            return FM_FS_ERROR;
        job->opened = 1;
        job->column = 0;
    }
    snprintf(job->message, sizeof(job->message), "Printing %s (%d/%d). Esc cancels.", name,
             job->target + 1, result->total);
    result->prompt = job->message;
    job->failure = "Cannot read the print file.";
    status = job->filesystem->file_read(job->filesystem->context, job->handle, job->buffer,
                                        sizeof(job->buffer), &count);
    if ((status != FM_FS_OK && status != FM_FS_END) || count < 0 ||
        count > (int)sizeof(job->buffer) || (!count && status == FM_FS_OK))
        return FM_FS_ERROR;
    ended = status == FM_FS_END;
    job->failure = "Printer output failed.";
    for (i = 0; i < count; ++i) {
        byte = job->buffer[i] & 255;
        if (byte == 255 || (byte & 127) == 26) {
            ended = 1;
            break;
        }
        if (print_byte(job, byte))
            return FM_FS_ERROR;
    }
    if (ended) {
        if (close_file(job)) {
            job->failure = "Could not finish the print file.";
            return FM_FS_ERROR;
        }
        result->completed = ++job->target;
        if (job->target == result->total) {
            result->state = FM_JOB_COMPLETE;
            result->prompt = "Selected files printed.";
        }
    }
    return FM_SUCCESS;
}

static const char *last_error(void *context)
{
    return ((struct fm_print_job *)context)->failure;
}

void fm_print_bind(struct fm_invoker *port, struct fm_print_job *job,
                   struct fm_filesystem *filesystem, struct fm_printer *printer,
                   const struct fm_location *location)
{
    memset(job, 0, sizeof(*job));
    job->filesystem = filesystem;
    job->printer = printer;
    job->location = location;
    port->context = job;
    port->invoke = invoke;
    port->advance = advance;
    port->cancel = cancel;
    port->error = last_error;
    port->interactive = 0;
}

/* Borrow a stable catalog while idle; reject changes during an invocation. */
int fm_print_configure(struct fm_print_job *job, const struct fm_entry *entries, int count)
{
    if (!job || job->active || count < 0 || count > FM_MAX_FILES || (count && !entries))
        return FM_BAD_MODEL;
    job->entries = entries;
    job->count = count;
    return FM_SUCCESS;
}
