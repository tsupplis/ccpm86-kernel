/* Cooperative binary Copy. Each data advance transfers at most eight 128-byte blocks.
 * Selection is snapshotted; text EOF has no meaning in this action. */
#include <stdio.h>
#include "copy.h"
#define COPY_PREPARE 0
#define COPY_CONFIRM 1
#define COPY_OPEN 2
#define COPY_DATA 3
#define COPY_COMMIT 4
#define COPY_WRITE 5
#define COPY_CLOSE 6
#define COPY_IO_BUDGET 16 /* Eight read/write pairs; return to input afterwards. */

static int cleanup(struct fm_copy_job *job)
{
    int status, closed;
    status = FM_FS_OK;
    if (job->opened) {
        job->opened = 0;
        status = (*job->source->file_close)(job->source->context, job->handle);
    }
    closed = (*job->store->abort)(job->store->context);
    return status || closed ? FM_FS_ERROR : FM_FS_OK;
}

static int invoke(void *context, FM_CONST struct fm_request *request)
{
    struct fm_copy_job *job;
    int index, selected;
    job = (struct fm_copy_job *)context;
    if (job->active || !request ||
        (request->command != FM_CMD_COPY && request->command != FM_CMD_SYSTEM_COPY) ||
        !request->token || request->count < 1 || request->count > FM_MAX_FILES)
        return FM_BAD_MODEL;
    for (index = 0; index < request->count; ++index) {
        selected = request->files[index];
        if (selected < 0 || selected >= job->count || job->entries[selected].directory ||
            (index && selected <= request->files[index - 1]))
            return FM_BAD_MODEL;
    }
    job->request = request;
    job->target = 0;
    job->opened = 0;
    job->decision = 0;
    job->stage = COPY_PREPARE;
    job->active = 1;
    job->message[0] = 0;
    return FM_SUCCESS;
}

static int cancel(void *context)
{
    struct fm_copy_job *job;
    int status;
    job = (struct fm_copy_job *)context;
    status = cleanup(job);
    job->active = 0;
    job->request = 0;
    return status;
}

static int advance(void *context, FM_CONST struct fm_reply *reply, struct fm_job_result *result)
{
    struct fm_copy_job *job;
    FM_CONST char *name;
    FM_CONST char *failure;
    int status, count, budget;
    static FM_CONST char *stages[] = {"prepare", "confirm", "open", "read",
                                      "publish", "write",   "close"};
    job = (struct fm_copy_job *)context;
    if (!job->active || !reply || !result || reply->token != job->request->token ||
        (reply->answer != FM_REPLY_NONE && reply->answer != FM_REPLY_ACCEPT) ||
        (reply->answer == FM_REPLY_ACCEPT &&
         (job->stage != COPY_CONFIRM || reply->decision != job->decision)) ||
        (reply->answer == FM_REPLY_NONE && reply->decision))
        return FM_BAD_MODEL;
    result->token = job->request->token;
    result->generation = job->request->generation;
    result->completed = job->target;
    result->total = job->request->count;
    result->failed = 0;
    result->decision = 0;
    result->state = FM_JOB_PROGRESS;
    result->page = 0;
    result->prompt = 0;
    if (job->target == job->request->count) {
        result->state = FM_JOB_COMPLETE;
        return FM_SUCCESS;
    }
    name = job->entries[job->request->files[job->target]].name;
    snprintf(job->message, sizeof(job->message), "Copy %.12s: %.8s. Esc cancels.", name,
             job->stage == COPY_DATA || job->stage == COPY_WRITE ? "transfer" : stages[job->stage]);
    result->prompt = job->message;
    status = FM_FS_OK;
    failure = 0;
    budget = job->stage == COPY_DATA || job->stage == COPY_WRITE ? COPY_IO_BUDGET : 1;
    do {
        switch (job->stage) {
        /* Resolve and check the target before opening either file. */
        case COPY_PREPARE:
            status = (*job->store->prepare)(job->store->context, job->location, &job->destination,
                                            name, &job->replace);
            if (!status) {
                job->stage = job->replace ? COPY_CONFIRM : COPY_OPEN;
                if (job->replace) {
                    if (!++job->decision)
                        ++job->decision;
                    snprintf(job->message, sizeof(job->message),
                             "Replace %.12s? Y replaces; N/Esc cancels Copy.", name);
                }
            }
            break;
        case COPY_CONFIRM:
            if (reply->answer == FM_REPLY_ACCEPT)
                job->stage = COPY_OPEN;
            break;
        case COPY_OPEN:
            status =
                (*job->source->file_open)(job->source->context, job->location, name, &job->handle);
            if (status)
                failure = "Cannot open source file.";
            if (!status) {
                job->opened = 1;
                status = (*job->store->begin)(job->store->context);
                if (!status)
                    job->stage = COPY_DATA;
            }
            break;
        /* Bound I/O calls as well as bytes, including short reads. */
        case COPY_DATA:
            count = 0;
            status = (*job->source->file_read)(job->source->context, job->handle, job->buffer,
                                               sizeof(job->buffer), &count);
            if ((status != FM_FS_OK && status != FM_FS_END) || count < 0 || count > 128 ||
                (!count && status == FM_FS_OK)) {
                status = FM_FS_ERROR;
                break;
            }
            job->buffered = count;
            job->source_end = status == FM_FS_END;
            job->stage = count ? COPY_WRITE : COPY_CLOSE;
            status = FM_FS_OK;
            break;
        case COPY_WRITE:
            status = (*job->store->write)(job->store->context, job->buffer, job->buffered);
            if (!status)
                job->stage = job->source_end ? COPY_CLOSE : COPY_DATA;
            break;
        case COPY_CLOSE:
            job->opened = 0;
            status = (*job->source->file_close)(job->source->context, job->handle);
            if (!status)
                job->stage = COPY_COMMIT;
            break;
        /* Publication belongs to storage; only a successful commit counts. */
        case COPY_COMMIT:
            status = (*job->store->commit)(job->store->context, job->replace);
            if (!status && job->request->command == FM_CMD_SYSTEM_COPY) {
                if (!job->attributes || !job->attributes->prepare_attributes)
                    status = FM_FS_ERROR;
                else {
                    status = job->attributes->prepare_attributes(job->attributes->context,
                                                                 &job->destination, name, 1);
                    if (!status)
                        status = job->attributes->commit(job->attributes->context);
                }
                if (status)
                    failure = "File copied; SYS/RO attributes could not be set.";
            }
            if (!status) {
                result->completed = ++job->target;
                job->stage = COPY_PREPARE;
                if (job->target == job->request->count) {
                    result->state = FM_JOB_COMPLETE;
                    result->prompt = 0;
                }
            }
            break;
        }
    } while (!status && --budget && (job->stage == COPY_DATA || job->stage == COPY_WRITE));
    if (status) {
        cleanup(job);
        snprintf(job->message, sizeof(job->message), "Copy %.12s: %.8s failed. %.36s", name,
                 stages[job->stage], failure ? failure : (*job->store->error)(job->store->context));
        result->state = FM_JOB_FAILED;
        result->failed = 1;
        result->prompt = job->message;
    } else if (job->stage == COPY_CONFIRM) {
        result->state = FM_JOB_DECISION;
        result->decision = job->decision;
        result->prompt = job->message;
    }
    return FM_SUCCESS;
}

/* Explicit destination syntax: A:0..15 on CP/M, B:\absolute\path on FAT.
 * No implicit current drive/user and no path truncation. */
int fm_copy_destination(struct fm_filesystem *filesystem, FM_CONST char *text,
                        struct fm_location *location)
{
    int drive, media, index, user, length;
    if (!text || !text[0] || text[1] != ':')
        return FM_FS_ERROR;
    drive = text[0];
    if (drive >= 'a' && drive <= 'p')
        drive -= 'a' - 'A';
    if (drive < 'A' || drive > 'P' ||
        (*filesystem->probe)(filesystem->context, drive - 'A', &media))
        return FM_FS_ERROR;
    if (media == FM_MEDIA_CPM) {
        if (!text[2])
            return FM_FS_ERROR;
        user = 0;
        for (index = 2; text[index]; ++index) {
            if (index > 3 || text[index] < '0' || text[index] > '9')
                return FM_FS_ERROR;
            user = user * 10 + text[index] - '0';
        }
        if (user > 15)
            return FM_FS_ERROR;
        location->user = user;
        location->path[0] = 0;
    } else {
        length = strlen(text + 2);
        if (text[2] != '\\' || length >= FM_PATH_CAPACITY)
            return FM_FS_ERROR;
        strcpy(location->path, text + 2);
        location->user = 0;
    }
    location->drive = drive - 'A';
    location->media = media;
    return FM_FS_OK;
}

static FM_CONST char *last_error(void *context)
{
    struct fm_copy_job *job;
    job = (struct fm_copy_job *)context;
    return (*job->store->error)(job->store->context);
}

void fm_copy_bind(struct fm_invoker *port, struct fm_copy_job *job, struct fm_filesystem *source,
                  struct fm_copy_store *store, FM_CONST struct fm_location *location,
                  FM_CONST struct fm_entry *entries)
{
    job->source = source;
    job->store = store;
    job->attributes = 0;
    job->location = location;
    job->entries = entries;
    job->active = 0;
    job->opened = 0;
    job->count = 0;
    port->context = job;
    port->invoke = invoke;
    port->advance = advance;
    port->cancel = cancel;
    port->error = last_error;
    port->interactive = 0;
}

/* Configure an idle action without exposing its mutable state to controllers.
 * The source catalog remains borrowed and must be stable until cancellation.
 * Destination coordinates are copied only after complete syntax validation. */
int fm_copy_configure(struct fm_copy_job *job, int count, FM_CONST char *text,
                      struct fm_mutation_store *attributes)
{
    struct fm_location destination;

    if (!job || job->active || count < 0 || count > FM_MAX_FILES ||
        (attributes && (!attributes->prepare_attributes || !attributes->commit)) ||
        fm_copy_destination(job->source, text, &destination))
        return FM_BAD_MODEL;
    job->attributes = attributes;
    job->destination.drive = destination.drive;
    job->destination.media = destination.media;
    job->destination.user = destination.user;
    strcpy(job->destination.path, destination.path);
    job->count = count;
    return FM_SUCCESS;
}
