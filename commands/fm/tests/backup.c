/* Backup uses injected transactions: never launch after an incomplete publish. */
#include <assert.h>
#include <stdio.h>
#include "backup.h"

struct fixture {
    char bytes[4096], command[40];
    int used, opened, committed, executed, aborted, failure;
};

static int root(void *context, const struct fm_location *source,
                struct fm_location *destination)
{
    *destination = *source;
    destination->drive = 2;
    destination->user = 0;
    strcpy(destination->path, "\\");
    return 0;
}

static int execute(void *context, const struct fm_location *location,
                   const char *command)
{
    struct fixture *f = context;
    assert(location->drive == 2 && !location->user);
    assert(!strcmp(location->path, "\\"));
    if (!strncmp(command, "BACK", 4)) assert(f->committed);
    strcpy(f->command, command);
    ++f->executed;
    return 0;
}

static int prepare(void *context, const struct fm_location *source,
                   const struct fm_location *destination, const char *name,
                   int *exists)
{
    assert(!source && destination->drive == 2);
    assert(!strcmp(name, "CONTROLC.BR"));
    *exists = 1;
    return 0;
}

static int begin(void *context)
{
    ((struct fixture *)context)->opened = 1;
    return 0;
}

static int write_block(void *context, const char *bytes, int length)
{
    struct fixture *f = context;
    assert(f->opened && !f->committed && length > 0);
    if (f->failure == 1) return FM_FS_ERROR;
    assert(f->used + length < (int)sizeof(f->bytes));
    memcpy(f->bytes + f->used, bytes, length);
    f->used += length;
    f->bytes[f->used] = 0;
    return 0;
}

static int commit(void *context, int replace)
{
    struct fixture *f = context;
    assert(f->opened && replace);
    if (f->failure == 2) return FM_FS_ERROR;
    f->committed = 1;
    f->opened = 0;
    return 0;
}

static int abort_write(void *context)
{
    struct fixture *f = context;
    f->opened = 0;
    ++f->aborted;
    return 0;
}

static const char *error(void *context) { return "Injected failure"; }

int main(void)
{
    struct fixture f;
    struct fm_copy_store store = {&f, prepare, begin, write_block, commit, abort_write, error};
    struct fm_backup_host host = {&f, root, execute, error};
    struct fm_backup_job job;
    struct fm_invoker port;
    struct fm_location location;
    struct fm_entry entries[2];
    struct fm_request request;
    struct fm_reply reply;
    struct fm_job_result result;
    int mode, step, status;

    memset(&location, 0, sizeof(location));
    location.drive = 13;
    strcpy(location.path, "\\DOCS\\NESTED");
    memset(entries, 0, sizeof(entries));
    strcpy(entries[0].name, "ONE.TXT");
    strcpy(entries[1].name, "TWO.TXT");
    memset(&request, 0, sizeof(request));
    request.token = 7;
    request.command = FM_CMD_BACKUP;
    request.count = 2;
    request.files[1] = 1;
    for (mode = 0; mode < 7; ++mode) {
        memset(&f, 0, sizeof(f));
        f.failure = mode == 3 ? 1 : mode == 4 ? 2 : 0;
        location.media = mode == 1 ? FM_MEDIA_CPM : FM_MEDIA_FAT;
        location.user = 4;
        fm_backup_bind(&port, &job, &host, &store, &location);
        assert(!fm_backup_configure(&job, entries, 2));
        request.count = mode == 6 ? 0 : 2;
        assert(!port.invoke(port.context, &request));
        assert(fm_backup_configure(&job, 0, 0) == FM_BAD_MODEL);
        memset(&reply, 0, sizeof(reply));
        reply.token = 7;
        assert(!port.advance(port.context, &reply, &result));
        assert(result.state == FM_JOB_INPUT);
        reply.answer = FM_REPLY_TEXT;
        reply.text = mode == 2 ? "R" : mode == 6 ? "4" : "1";
        reply.decision = result.decision + 1;
        assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
        --reply.decision;
        assert(!port.advance(port.context, &reply, &result));
        assert(result.state == FM_JOB_DECISION);
        if (mode == 5) {
            assert(!port.cancel(port.context));
            assert(!f.executed && !f.opened && !f.committed);
            continue;
        }
        reply.answer = FM_REPLY_ACCEPT;
        reply.decision = result.decision;
        assert(!port.advance(port.context, &reply, &result));
        reply.answer = FM_REPLY_NONE;
        reply.decision = 0;
        status = 0;
        for (step = 0; step < 40; ++step) {
            status = port.advance(port.context, &reply, &result);
            if (status || result.state == FM_JOB_COMPLETE) break;
        }
        assert(step < 40);
        if (f.failure) {
            assert(status && !f.executed && !f.committed);
            assert(!strcmp(port.error(port.context), "Injected failure"));
        } else {
            assert(!status && f.executed == 1 && result.total == request.count && result.completed == request.count);
            if (mode == 2) assert(!f.committed && !strcmp(f.command, "REST.CMD CTL=C"));
            else {
                assert(f.committed && !strcmp(f.command, "BACK.CMD CTL=C"));
                assert(strstr(f.bytes, "SOURCE: C\r\nCONTROL DRIVE: C\r\n"));
                assert(!strcmp(f.bytes + f.used - strlen("EXC:  ?,?,n,k,*.*\r\n"), "EXC:  ?,?,n,k,*.*\r\n"));
                if (mode == 1) assert(strstr(f.bytes, "EXC:  4,C,a,k,ONE.TXT\r\n"));
                else assert(strstr(f.bytes, "PATH: C:\\DOCS\\NESTED\r\n"));
                if (mode == 6) assert(strstr(f.bytes, "DEST DRIVE: B\r\n") && strstr(f.bytes, "EXC:  D,C,a,k,*.*\r\n"));
                else assert(strstr(f.bytes, "TWO.TXT\r\n"));
            }
        }
        assert(!port.cancel(port.context));
        assert(!f.opened);
    }
    puts("PASS: Backup CP/M/FAT rules, restore, whole-directory mode, confirmation, stale replies, write/commit failure and cleanup");
    return 0;
}
