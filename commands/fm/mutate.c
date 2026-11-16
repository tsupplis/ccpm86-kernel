/* Rename/Delete/attribute jobs: one exact file per confirmed step, no OS or UI calls. */
#include <stdio.h>
#include "mutate.h"

/* No wildcard, drive, path, whitespace or attribute syntax is accepted. */
int fm_mutation_name(const char *name)
{
    int base = 0, extension = 0, dot = 0;
    unsigned char ch;

    if (!name)
        return 0;
    while ((ch = *name++) != 0) {
        if (ch == '.') {
            if (!base || dot)
                return 0;
            dot = 1;
        } else {
            if (ch <= 32 || ch >= 127 || strchr("<>/\\:;=,?*[]|\"", ch))
                return 0;
            if (dot) {
                if (++extension > 3)
                    return 0;
            } else if (++base > 8)
                return 0;
        }
    }
    return base && (!dot || extension);
}

static int invoke(void *context, const struct fm_request *request)
{
    struct fm_mutation_job *job = context;
    int index, target;

    if (job->active || !request || !request->token || request->count < 1 ||
        request->count > FM_MAX_FILES ||
        (request->command != FM_CMD_DELETE && request->command != FM_CMD_RENAME &&
         request->command != FM_CMD_SYSTEM_ATTR && request->command != FM_CMD_LOCAL_ATTR))
        return FM_BAD_MODEL;
    for (index = 0; index < request->count; ++index) {
        target = request->files[index];
        if (target < 0 || target >= job->count || job->entries[target].directory ||
            !fm_mutation_name(job->entries[target].name) ||
            (index && target <= request->files[index - 1]))
            return FM_BAD_MODEL;
    }
    job->request = request;
    job->active = 1;
    job->target = job->prepared = job->asking = 0;
    job->replacement[0] = 0;
    job->decision = 0;
    return FM_SUCCESS;
}

static int cancel(void *context)
{
    struct fm_mutation_job *job = context;
    job->active = job->prepared = job->asking = 0;
    job->request = 0;
    return FM_SUCCESS;
}

static const char *last_error(void *context)
{
    struct fm_mutation_job *job = context;
    return job->store->error(job->store->context);
}

static int advance(void *context, const struct fm_reply *reply, struct fm_job_result *result)
{
    struct fm_mutation_job *job = context;
    const char *name, *replacement;
    int status;

    if (!job->active || !reply || !result || reply->token != job->request->token ||
        (reply->answer != FM_REPLY_NONE && reply->answer != FM_REPLY_ACCEPT &&
         reply->answer != FM_REPLY_TEXT) ||
        (reply->answer == FM_REPLY_ACCEPT &&
         (!job->prepared || reply->decision != job->decision)) ||
        (reply->answer == FM_REPLY_TEXT &&
         (!job->asking || !reply->text || reply->decision != job->decision)) ||
        (reply->answer == FM_REPLY_NONE && reply->decision))
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
    name = job->entries[job->request->files[job->target]].name;
    if (job->request->command == FM_CMD_RENAME && !job->replacement[0]) {
        if (reply->answer == FM_REPLY_TEXT && fm_mutation_name(reply->text)) {
            strcpy(job->replacement, reply->text);
            job->asking = 0;
        } else {
            if (!++job->decision)
                ++job->decision;
            job->asking = 1;
            snprintf(job->message, sizeof(job->message),
                     "New 8.3 name for %s (%d/%d). No overwrite. Esc cancels.", name,
                     job->target + 1, result->total);
            result->state = FM_JOB_INPUT;
            result->decision = job->decision;
            result->prompt = job->message;
            return FM_SUCCESS;
        }
    }
    replacement = job->request->command == FM_CMD_RENAME ? job->replacement : 0;
    if (!job->prepared) {
        if (job->request->command == FM_CMD_SYSTEM_ATTR ||
            job->request->command == FM_CMD_LOCAL_ATTR) {
            if (!job->store->prepare_attributes)
                return FM_BAD_MODEL;
            status = job->store->prepare_attributes(job->store->context, job->location, name,
                                                    job->request->command == FM_CMD_SYSTEM_ATTR);
        } else {
            status = job->store->prepare(job->store->context, job->location, name, replacement);
        }
        if (status)
            return status;
        job->prepared = 1;
        if (!++job->decision)
            ++job->decision;
        if (job->request->command == FM_CMD_SYSTEM_ATTR ||
            job->request->command == FM_CMD_LOCAL_ATTR)
            snprintf(job->message, sizeof(job->message), "Set %s %s? Y confirms; N/Esc cancels.",
                     name, job->request->command == FM_CMD_SYSTEM_ATTR ? "SYS, RO" : "DIR, RW");
        else if (replacement)
            snprintf(job->message, sizeof(job->message),
                     "Rename %s to %s? Y confirms; N/Esc cancels.", name, replacement);
        else
            snprintf(job->message, sizeof(job->message),
                     "Delete %s permanently? Y confirms; N/Esc cancels.", name);
    } else if (reply->answer == FM_REPLY_ACCEPT) {
        status = job->store->commit(job->store->context);
        if (status)
            return status;
        job->prepared = 0;
        job->replacement[0] = 0;
        result->completed = ++job->target;
        result->state = job->target == result->total ? FM_JOB_COMPLETE : FM_JOB_PROGRESS;
        return FM_SUCCESS;
    }
    result->state = FM_JOB_DECISION;
    result->decision = job->decision;
    result->prompt = job->message;
    return FM_SUCCESS;
}

void fm_mutation_bind(struct fm_invoker *port, struct fm_mutation_job *job,
                      struct fm_mutation_store *store, const struct fm_location *location)
{
    memset(job, 0, sizeof(*job));
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
int fm_mutation_configure(struct fm_mutation_job *job, const struct fm_entry *entries, int count)
{
    if (!job || job->active || count < 0 || count > FM_MAX_FILES || (count && !entries))
        return FM_BAD_MODEL;
    job->entries = entries;
    job->count = count;
    return FM_SUCCESS;
}
