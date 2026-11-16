/* Run, Edit, Help and disk utilities are policies over one launch lifecycle.
 * No console, filesystem or process API is used by this module.
 */
#include <stdio.h>
#include "run.h"

struct launch_policy {
    int command, selection;
    const char *program, *prompt;
};

static const struct launch_policy policies[] = {
    {FM_CMD_LOAD_KEYS, FM_TARGET_ONE, "FUNCTION.CMD",
     "Enter loads this function-key file; Esc cancels."},
    {FM_CMD_ASSIGN_KEYS, FM_TARGET_NONE, "FUNCTION.CMD",
     "Enter opens FUNCTION, then SETUP /PARAM; Esc cancels."},
    {FM_CMD_SERIAL, FM_TARGET_NONE, "SETPORT.CMD",
     "Enter opens SETPORT, then SETUP /PARAM; Esc cancels."},
    {FM_CMD_DEFAULTS, FM_TARGET_NONE, "SETUP.CMD", "SETUP arguments (optional). Esc cancels."},
    {FM_CMD_MEMORY_DISK, FM_TARGET_NONE, "SETUP.CMD /MDISK",
     "Enter opens memory-disk setup; Esc cancels."},
    {FM_CMD_BOOT_DISK, FM_TARGET_NONE, "", "Boot installation: enter DOS or CP/M. Esc cancels."},
    {FM_CMD_HARD_MAINT, FM_TARGET_NONE, "HDMAINT.CMD",
     "Enter opens hard-disk maintenance; Esc cancels."},
    {FM_CMD_FLOPPY_MAINT, FM_TARGET_NONE, "DSKMAINT.CMD",
     "Enter opens diskette maintenance; Esc cancels."},
    {FM_CMD_MEMORY, FM_TARGET_NONE, "STOP.CMD",
     "STOP arguments (optional). Enter opens the process display."},
    {FM_CMD_RUN, FM_TARGET_ONE, "", "Program arguments (optional). Enter runs; Esc cancels."},
    {FM_CMD_EDIT, FM_TARGET_ONE, "DREDIX.CMD",
     "Editor arguments (optional). Enter runs; Esc cancels."},
    {FM_CMD_HELP, FM_TARGET_NONE, "HELP.CMD", "Help topic (optional). Enter runs; Esc cancels."},
    {FM_CMD_COPY_DISK, FM_TARGET_NONE, "DCOPY.CMD",
     "DCOPY arguments. Enter opens the utility; Esc cancels."},
    {FM_CMD_FORMAT_DISK, FM_TARGET_NONE, "DFORM.CMD",
     "DFORM arguments. Enter opens the utility; Esc cancels."},
    {FM_CMD_COMMAND, FM_TARGET_NONE, "", "External command and arguments. Esc cancels."}};

static const struct launch_policy *policy(int command)
{
    unsigned i;
    for (i = 0; i < sizeof(policies) / sizeof(policies[0]); ++i)
        if (policies[i].command == command)
            return &policies[i];
    return 0;
}

int fm_run_policy(int command)
{
    const struct launch_policy *item = policy(command);
    return item ? item->selection : -1;
}

static int invoke(void *context, const struct fm_request *request)
{
    struct fm_run_job *job = context;
    const struct launch_policy *item;
    const char *name = "";
    int index;

    if (!request || !request->token || job->active || !job->executor || !job->executor->execute)
        return FM_BAD_MODEL;
    item = policy(request->command);
    if (!item || request->count != (item->selection == FM_TARGET_ONE))
        return FM_BAD_MODEL;
    if (request->count) {
        index = request->files[0];
        if (!job->entries || index < 0 || index >= job->count || job->entries[index].directory)
            return FM_BAD_MODEL;
        name = job->entries[index].name;
    }
    snprintf(job->prefix, sizeof(job->prefix), "%s%s%s", item->program,
             item->program[0] && name[0] ? " " : "", name);
    job->request = request;
    job->prompt = item->prompt;
    job->active = 1;
    job->asking = 0;
    job->decision = 0;
    job->followup = request->command == FM_CMD_ASSIGN_KEYS || request->command == FM_CMD_SERIAL;
    return FM_SUCCESS;
}

static int cancel(void *context)
{
    struct fm_run_job *job = context;
    job->active = job->asking = 0;
    job->request = 0;
    return FM_SUCCESS;
}

static const char *last_error(void *context)
{
    struct fm_run_job *job = context;
    return job->executor->error ? job->executor->error(job->executor->context)
                                : "Program could not be launched.";
}

static int advance(void *context, const struct fm_reply *reply, struct fm_job_result *result)
{
    struct fm_run_job *job = context;
    unsigned length;
    int status;

    if (!job->active || !reply || !result || reply->token != job->request->token ||
        (reply->answer != FM_REPLY_NONE && reply->answer != FM_REPLY_TEXT) ||
        (reply->answer == FM_REPLY_NONE && reply->decision) ||
        (reply->answer == FM_REPLY_TEXT &&
         (!job->asking || !reply->text || reply->decision != job->decision)))
        return FM_BAD_MODEL;
    memset(result, 0, sizeof(*result));
    result->token = job->request->token;
    result->generation = job->request->generation;
    result->total = job->request->count;
    if (reply->answer == FM_REPLY_TEXT) {
        if (job->request->command == FM_CMD_BOOT_DISK) {
            if (!strcmp(reply->text, "DOS") || !strcmp(reply->text, "dos"))
                strcpy(job->prefix, "HDMAINT.CMD [F5,F6,F0,F0]");
            else if (!strcmp(reply->text, "CP/M") || !strcmp(reply->text, "cp/m"))
                strcpy(job->prefix, "HDMAINT.CMD [F5,F4,F0,F0]");
            else
                goto ask;
        }
        length = strlen(job->prefix) +
                 (job->request->command == FM_CMD_BOOT_DISK ? 0 : strlen(reply->text));
        if (job->prefix[0] && reply->text[0] && job->request->command != FM_CMD_BOOT_DISK)
            ++length;
        if (!length || length >= sizeof(job->command)) {
            job->prompt = "Enter a command of at most 127 characters. Esc cancels.";
        } else {
            if (job->request->command == FM_CMD_BOOT_DISK)
                strcpy(job->command, job->prefix);
            else
                snprintf(job->command, sizeof(job->command), "%s%s%s", job->prefix,
                         job->prefix[0] && reply->text[0] ? " " : "", reply->text);
            job->asking = 0; /* A reply can never execute a program twice. */
            status = job->executor->execute(job->executor->context, job->location, job->command);
            if (status)
                return status;
            if (job->followup) {
                job->followup = 0;
                status = job->executor->execute(job->executor->context, job->location,
                                                "SETUP.CMD /PARAM");
                if (status)
                    return status;
            }
            job->active = 0;
            result->completed = result->total;
            result->state = FM_JOB_COMPLETE;
            result->prompt = "Program returned to FM.";
            return FM_SUCCESS;
        }
    }
ask:
    if (!++job->decision)
        ++job->decision;
    job->asking = 1;
    result->decision = job->decision;
    result->state = FM_JOB_INPUT;
    result->prompt = job->prompt;
    return FM_SUCCESS;
}

void fm_run_bind(struct fm_invoker *port, struct fm_run_job *job, struct fm_executor *executor,
                 const struct fm_location *location)
{
    memset(job, 0, sizeof(*job));
    job->executor = executor;
    job->location = location;
    port->context = job;
    port->invoke = invoke;
    port->advance = advance;
    port->cancel = cancel;
    port->error = last_error;
    port->interactive = 0;
}

/* Borrow a stable catalog while idle; reject changes during an invocation. */
int fm_run_configure(struct fm_run_job *job, const struct fm_entry *entries, int count)
{
    if (!job || job->active || count < 0 || count > FM_MAX_FILES || (count && !entries))
        return FM_BAD_MODEL;
    job->entries = entries;
    job->count = count;
    return FM_SUCCESS;
}
