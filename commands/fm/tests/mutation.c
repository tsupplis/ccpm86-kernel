/* Pure mutation lifecycle tests: injected storage never touches user files. */
#include <assert.h>
#include <stdio.h>
#include "mutate.h"

struct store { int prepared, commits, fail_prepare, fail_commit; };
static int prepare(void *context, const struct fm_location *location,
                   const char *name, const char *replacement)
{
    struct store *s = context;
    (void)location; (void)name; (void)replacement;
    ++s->prepared;
    return s->fail_prepare;
}
static int commit(void *context)
{
    struct store *s = context;
    if (s->fail_commit) return FM_FS_ERROR;
    ++s->commits;
    return FM_FS_OK;
}
static const char *error(void *context)
{
    (void)context;
    return "Injected failure";
}
int main(void)
{
    struct store s = {0};
    struct fm_mutation_store storage = {&s, prepare, commit, error, 0};
    struct fm_location location;
    struct fm_entry entries[2];
    struct fm_mutation_job job;
    struct fm_invoker port;
    struct fm_request request;
    struct fm_reply reply;
    struct fm_job_result result;
    unsigned decision;

    memset(&location, 0, sizeof(location));
    memset(entries, 0, sizeof(entries));
    strcpy(entries[0].name, "ONE.TXT");
    strcpy(entries[1].name, "TWO.TXT");
    memset(&request, 0, sizeof(request));
    request.command = FM_CMD_DELETE;
    request.token = 1;
    request.generation = 5;
    request.count = 2;
    request.files[1] = 1;
    fm_mutation_bind(&port, &job, &storage, &location);
    assert(!fm_mutation_configure(&job, entries, 2));
    assert(!port.invoke(port.context, &request));
        assert(fm_mutation_configure(&job, 0, 0) == FM_BAD_MODEL);
    reply.token = 1; reply.answer = FM_REPLY_NONE; reply.decision = 0;
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_DECISION && !s.commits);
    decision = result.decision;
    reply.answer = FM_REPLY_ACCEPT; reply.decision = decision + 1;
    assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
    assert(!s.commits);
    reply.decision = decision;
    assert(!port.advance(port.context, &reply, &result));
    assert(s.commits == 1 && result.completed == 1);
    assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
    reply.answer = FM_REPLY_NONE; reply.decision = 0;
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_DECISION && result.decision != decision);
    assert(!port.cancel(port.context));
    assert(s.commits == 1);
    assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);

    request.command = FM_CMD_RENAME;
    request.count = 2;
    reply.answer = FM_REPLY_NONE; reply.decision = 0; reply.text = 0;
    assert(!port.invoke(port.context, &request));
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_INPUT);
    reply.answer = FM_REPLY_TEXT; reply.decision = result.decision;
    reply.text = "*.TXT";
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_INPUT && s.commits == 1);
    reply.decision = result.decision; reply.text = "NEW.TXT";
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_DECISION);
    reply.answer = FM_REPLY_ACCEPT; reply.decision = result.decision;
    assert(!port.advance(port.context, &reply, &result));
    assert(result.completed == 1 && s.commits == 2);
    reply.answer = FM_REPLY_NONE; reply.decision = 0;
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_INPUT && strstr(result.prompt, "TWO.TXT"));
    reply.answer = FM_REPLY_TEXT; reply.decision = result.decision;
    reply.text = "SECOND.TXT";
    assert(!port.advance(port.context, &reply, &result));
    reply.answer = FM_REPLY_ACCEPT; reply.decision = result.decision;
    s.fail_commit = 1;
    assert(port.advance(port.context, &reply, &result));
    assert(s.commits == 2);
    port.cancel(port.context); s.fail_commit = 0;
    request.command = FM_CMD_DELETE;
    reply.answer = FM_REPLY_NONE; reply.decision = 0;
    s.fail_prepare = 1;
    assert(!port.invoke(port.context, &request));
    assert(port.advance(port.context, &reply, &result));
    port.cancel(port.context); s.fail_prepare = 0;
    assert(!fm_mutation_name("*.TXT") && !fm_mutation_name("A:FILE") &&
           !fm_mutation_name("..") && !fm_mutation_name("TOOLONG99.TXT") &&
           fm_mutation_name("GOOD.TXT") && fm_mutation_name("NOEXT"));
    entries[0].directory = 1;
    assert(port.invoke(port.context, &request) == FM_BAD_MODEL);
    puts("PASS: mutation confirmation, stale/replayed replies, partial cancellation, failures and exact names");
    return 0;
}
