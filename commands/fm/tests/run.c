/* Executor injection verifies every action without starting real utilities. */
#include <assert.h>
#include <stdio.h>
#include "run.h"

struct recorder { int calls, failure; char command[128], previous[128]; };
static int execute(void *context, const struct fm_location *location,
                   const char *command)
{
    struct recorder *r = context;
    assert(location->drive == 1);
    ++r->calls;
    strcpy(r->previous, r->command);
    strcpy(r->command, command);
    return r->failure;
}
static const char *error(void *context) { return "injected failure"; }

int main(void)
{
    struct recorder recorder = {0};
    struct fm_executor executor = {&recorder, execute, error};
    struct fm_run_job job;
    struct fm_invoker port;
    struct fm_request request;
    struct fm_reply reply;
    struct fm_job_result result;
    struct fm_location location;
    struct fm_entry entry;
    int i, calls;
    static const int actions[] = {FM_CMD_RUN, FM_CMD_EDIT, FM_CMD_HELP,
        FM_CMD_COPY_DISK, FM_CMD_FORMAT_DISK, FM_CMD_MEMORY, FM_CMD_COMMAND,
        FM_CMD_LOAD_KEYS, FM_CMD_ASSIGN_KEYS, FM_CMD_SERIAL, FM_CMD_DEFAULTS,
        FM_CMD_MEMORY_DISK, FM_CMD_BOOT_DISK, FM_CMD_HARD_MAINT, FM_CMD_FLOPPY_MAINT};
    static const char *arguments[] = {"Mixed Case", "", "DIR", "A: B:", "B:", "", "PROBE.CMD",
        "", "", "", "/PARAM", "", "DOS", "", ""};
    static const char *expected[] = {"PROBE.CMD Mixed Case", "DREDIX.CMD PROBE.CMD",
        "HELP.CMD DIR", "DCOPY.CMD A: B:", "DFORM.CMD B:", "STOP.CMD", "PROBE.CMD",
        "FUNCTION.CMD PROBE.CMD", "SETUP.CMD /PARAM", "SETUP.CMD /PARAM",
        "SETUP.CMD /PARAM", "SETUP.CMD /MDISK", "HDMAINT.CMD [F5,F6,F0,F0]",
        "HDMAINT.CMD", "DSKMAINT.CMD"};

    memset(&location, 0, sizeof(location)); location.drive = 1;
    memset(&entry, 0, sizeof(entry)); strcpy(entry.name, "PROBE.CMD");
    fm_run_bind(&port, &job, &executor, &location);
    assert(!fm_run_configure(&job, &entry, 1));
    memset(&request, 0, sizeof(request)); request.token = 17; request.generation = 2;
    for (i = 0; i < (int)(sizeof(actions) / sizeof(actions[0])); ++i) {
        request.command = actions[i];
        request.count = fm_run_policy(actions[i]) == FM_TARGET_ONE;
        assert(!port.invoke(port.context, &request));
        assert(fm_run_configure(&job, 0, 0) == FM_BAD_MODEL);
        reply.token = 17; reply.answer = FM_REPLY_NONE; reply.decision = 0; reply.text = 0;
        assert(!port.advance(port.context, &reply, &result));
        assert(result.state == FM_JOB_INPUT);
        reply.answer = FM_REPLY_TEXT; reply.text = arguments[i];
        reply.decision = result.decision + 1;
        calls = recorder.calls;
        assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
        assert(recorder.calls == calls);
        --reply.decision;
        assert(!port.advance(port.context, &reply, &result));
        assert(result.state == FM_JOB_COMPLETE && recorder.calls == calls + 1 +
               (actions[i] == FM_CMD_ASSIGN_KEYS || actions[i] == FM_CMD_SERIAL));
        assert(!strcmp(recorder.command, expected[i]));
        if (actions[i] == FM_CMD_ASSIGN_KEYS) assert(!strcmp(recorder.previous, "FUNCTION.CMD"));
        if (actions[i] == FM_CMD_SERIAL) assert(!strcmp(recorder.previous, "SETPORT.CMD"));
        assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
        assert(!port.cancel(port.context));
    }
    request.command = FM_CMD_COMMAND;
    request.count = 0;
    assert(!port.invoke(port.context, &request));
    assert(!port.cancel(port.context));
    calls = recorder.calls;
    assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
    assert(recorder.calls == calls);
    assert(!port.invoke(port.context, &request));
    reply.answer = FM_REPLY_NONE; reply.decision = 0;
    assert(!port.advance(port.context, &reply, &result));
    reply.answer = FM_REPLY_TEXT; reply.decision = result.decision; reply.text = "";
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_INPUT && recorder.calls == calls);
    reply.decision = result.decision; reply.text = "MISSING.CMD";
    recorder.failure = FM_FS_ERROR;
    assert(port.advance(port.context, &reply, &result) == FM_FS_ERROR);
    assert(!strcmp(port.error(port.context), "injected failure"));
    assert(!port.cancel(port.context));
    puts("PASS: all launch policies, optional input, empty command, cancellation, replay and launch failure");
    return 0;
}
