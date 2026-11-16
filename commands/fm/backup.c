/* Cooperative BackRest control-file publication and launch. No OS/UI calls. */
#include <stdio.h>
#include "backup.h"

#define BACK_MODE 0
#define BACK_CONFIRM 1
#define BACK_BEGIN 2
#define BACK_HEADER 3
#define BACK_RULES 4
#define BACK_END 5
#define BACK_COMMIT 6
#define BACK_RUN 7

static const char header[] = "CLEAR: 2,27,69\r\n"
                             "ATTRIBUTE: start screen: 9,27,99,0,27,97,3,27,98,2\r\n"
                             "ATTRIBUTE: leave screen: 3,27,98,7\r\n"
                             "ATTRIBUTE: general: 3,27,98,2\r\n"
                             "ATTRIBUTE: errors: 3,27,98,15\r\n"
                             "ATTRIBUTE: message: 3,27,98,14\r\n"
                             "ATTRIBUTE: data: 3,27,98,3\r\n"
                             "ATTRIBUTE: input: 3,27,98,12\r\n"
                             "PRINTER INIT: 1,13\r\nFORMFEED: 1,12\r\nLENGTH: 60\r\nWIDTH: 80\r\n"
                             "REPORT PRINT: true\r\nSHOW SKIPS: true\r\n"
                             "ID:Concurrent DOS Hard Disk Backup\r\nSPLIT: true\r\nERASE: true\r\n";
static const char exclusions[] = "EXC:  ?,?,c,k,control?.br\r\nEXC:  ?,?,n,k,*.br?\r\n";

static int invoke(void *context, const struct fm_request *request)
{
    struct fm_backup_job *job = context;
    int i, selected;

    if (job->active || !request || !request->token || request->command != FM_CMD_BACKUP ||
        request->count < 0 || request->count > FM_MAX_FILES)
        return FM_BAD_MODEL;
    for (i = 0; i < request->count; ++i) {
        selected = request->files[i];
        if (!job->entries || selected < 0 || selected >= job->count ||
            job->entries[selected].directory || !fm_mutation_name(job->entries[selected].name) ||
            (i && selected <= request->files[i - 1]))
            return FM_BAD_MODEL;
    }
    job->request = request;
    job->stage = BACK_MODE;
    job->decision = job->line = job->target = 0;
    job->active = 1;
    job->failure = "Cannot prepare Backup: select a hard-disk location.";
    return job->host->prepare(job->host->context, job->location, &job->root);
}

static int cancel(void *context)
{
    struct fm_backup_job *job = context;
    int result = job->store->abort(job->store->context);

    job->active = 0;
    job->request = 0;
    return result;
}

static const char *last_error(void *context)
{
    return ((struct fm_backup_job *)context)->failure;
}

/* Emit at most one short line/block per advance. There is no whole-control
 * buffer or duplicated selection array, even for 1,024 selected files. */
static int write_text(struct fm_backup_job *job, const char *text, int size)
{
    int result = job->store->write(job->store->context, text, size);
    if (result)
        job->failure = job->store->error(job->store->context);
    return result;
}

static int advance(void *context, const struct fm_reply *reply, struct fm_job_result *result)
{
    struct fm_backup_job *job = context;
    const char *name;
    int length, status = FM_SUCCESS;
    char choice;

    if (!job->active || !reply || !result || reply->token != job->request->token ||
        (reply->answer != FM_REPLY_NONE && reply->answer != FM_REPLY_TEXT &&
         reply->answer != FM_REPLY_ACCEPT) ||
        (reply->answer == FM_REPLY_NONE && reply->decision) ||
        (reply->answer != FM_REPLY_NONE && reply->decision != job->decision) ||
        (reply->answer == FM_REPLY_TEXT && (job->stage != BACK_MODE || !reply->text)) ||
        (reply->answer == FM_REPLY_ACCEPT && job->stage != BACK_CONFIRM))
        return FM_BAD_MODEL;
    memset(result, 0, sizeof(*result));
    result->token = job->request->token;
    result->generation = job->request->generation;
    /* Counts cover selected rules handed to BackRest, not archive verification.
     * The outcome text explicitly leaves archive results to BACK/REST. */
    result->total = job->request->count;
    result->state = FM_JOB_PROGRESS;
    result->prompt = "Preparing BackRest control file. Esc cancels.";

    if (job->stage == BACK_MODE) {
        if (reply->answer == FM_REPLY_TEXT && reply->text[0] && !reply->text[1]) {
            choice = reply->text[0];
            if (choice >= 'a' && choice <= 'z')
                choice -= 'a' - 'A';
            if (choice == 'R' || (choice >= '1' && choice <= '4')) {
                job->restore = choice == 'R';
                job->whole = choice == '3' || choice == '4';
                job->destination = choice == '2' || choice == '4' ? 1 : 0;
                if (!job->restore && !job->whole && !job->request->count)
                    goto ask;
                snprintf(job->control, sizeof(job->control), "CONTROL%c.BR", 'A' + job->root.drive);
                snprintf(job->command, sizeof(job->command), "%s.CMD CTL=%c",
                         job->restore ? "REST" : "BACK", 'A' + job->root.drive);
                if (!job->restore && job->store->prepare(job->store->context, 0, &job->root,
                                                         job->control, &job->exists)) {
                    job->failure = job->store->error(job->store->context);
                    return FM_FS_ERROR;
                }
                job->stage = BACK_CONFIRM;
                if (!++job->decision)
                    ++job->decision;
            }
        }
    ask:
        if (job->stage == BACK_MODE) {
            if (!++job->decision)
                ++job->decision;
            result->state = FM_JOB_INPUT;
            result->decision = job->decision;
            result->prompt = "1/2: selected to A/B. 3/4: all here to A/B. R: restore.";
            return FM_SUCCESS;
        }
    }
    switch (job->stage) {
    case BACK_CONFIRM:
        if (reply->answer == FM_REPLY_ACCEPT) {
            job->stage = job->restore ? BACK_RUN : BACK_BEGIN;
            break;
        }
        snprintf(job->text, sizeof(job->text),
                 job->restore ? "Run REST with %s? Restored files may replace existing files."
                              : "Write %s and run BACK to %c:? Destination disks will be erased.",
                 job->control, 'A' + job->destination);
        result->state = FM_JOB_DECISION;
        result->decision = job->decision;
        result->prompt = job->text;
        break;
    case BACK_BEGIN:
        status = job->store->begin(job->store->context);
        if (status)
            job->failure = job->store->error(job->store->context);
        else
            job->stage = BACK_HEADER;
        break;
    case BACK_HEADER:
        length = sizeof(header) - 1 - job->line;
        if (length > 128)
            length = 128;
        if (length) {
            status = write_text(job, header + job->line, length);
            job->line += length;
        } else {
            snprintf(job->text, sizeof(job->text),
                     "DEST DRIVE: %c\r\nSOURCE: %c\r\nCONTROL DRIVE: %c\r\n",
                     'A' + job->destination, 'A' + job->root.drive, 'A' + job->root.drive);
            status = write_text(job, job->text, strlen(job->text));
            job->stage = BACK_RULES;
            job->line = 0;
        }
        break;
    case BACK_RULES:
        if (!job->line++) {
            if (job->location->media == FM_MEDIA_FAT)
                snprintf(job->text, sizeof(job->text), "PATH: %c:%s\r\n", 'A' + job->root.drive,
                         job->location->path);
            else
                strcpy(job->text, "USERS: 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15\r\n");
            status = write_text(job, job->text, strlen(job->text));
        } else if (job->line == 2) {
            status = write_text(job, exclusions, sizeof(exclusions) - 1);
        } else {
            name = job->whole ? "*.*" : job->entries[job->request->files[job->target]].name;
            if (job->location->media == FM_MEDIA_FAT)
                snprintf(job->text, sizeof(job->text), "EXC:  D,%c,a,k,%s\r\n",
                         'A' + job->root.drive, name);
            else
                snprintf(job->text, sizeof(job->text), "EXC: %2d,%c,a,k,%s\r\n",
                         job->location->user, 'A' + job->root.drive, name);
            status = write_text(job, job->text, strlen(job->text));
            if (job->whole || ++job->target == job->request->count)
                job->stage = BACK_END;
        }
        break;
    case BACK_END:
        status = write_text(job, "EXC:  ?,?,n,k,*.*\r\n", sizeof("EXC:  ?,?,n,k,*.*\r\n") - 1);
        job->stage = BACK_COMMIT;
        break;
    case BACK_COMMIT:
        status = job->store->commit(job->store->context, job->exists);
        if (status)
            job->failure = job->store->error(job->store->context);
        else
            job->stage = BACK_RUN;
        break;
    case BACK_RUN:
        status = job->host->execute(job->host->context, &job->root, job->command);
        if (status)
            job->failure = job->host->error(job->host->context);
        else {
            result->state = FM_JOB_COMPLETE;
            result->completed = result->total;
            result->prompt = "BackRest returned to FM. Check its report for archive results.";
            job->active = 0;
        }
        break;
    }
    return status;
}

void fm_backup_bind(struct fm_invoker *port, struct fm_backup_job *job, struct fm_backup_host *host,
                    struct fm_copy_store *store, const struct fm_location *location)
{
    memset(job, 0, sizeof(*job));
    job->host = host;
    job->store = store;
    job->location = location;
    port->context = job;
    port->invoke = invoke;
    port->advance = advance;
    port->cancel = cancel;
    port->error = last_error;
    port->interactive = 0;
}

/* Borrow a stable catalog while idle; reject changes during an invocation. */
int fm_backup_configure(struct fm_backup_job *job, const struct fm_entry *entries, int count)
{
    if (!job || job->active || count < 0 || count > FM_MAX_FILES || (count && !entries))
        return FM_BAD_MODEL;
    job->entries = entries;
    job->count = count;
    return FM_SUCCESS;
}
