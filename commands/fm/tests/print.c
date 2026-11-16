/* Print contracts: no OS, printer or disk is required. */
#include <assert.h>
#include <stdio.h>
#include "print.h"

struct fixture {
    int attached, detached, opened, closed, reads, failed, unavailable, longfile;
    char output[1024];
    int used;
};
static int attach(void *context)
{
    struct fixture *f = context;
    if (f->unavailable) return FM_FS_ERROR;
    ++f->attached; return FM_FS_OK;
}
static int put(void *context, int byte)
{
    struct fixture *f = context;
    if (f->failed) return FM_FS_ERROR;
    assert(f->used < (int)sizeof(f->output));
    f->output[f->used++] = byte; return FM_FS_OK;
}
static int detach(void *context) { ++((struct fixture *)context)->detached; return FM_FS_OK; }
static int open_file(void *context, const struct fm_location *location, const char *name, int *handle)
{
    struct fixture *f = context;
    ++f->opened; f->reads = 0; *handle = 1; return FM_FS_OK;
}
static int read_file(void *context, int handle, char *buffer, int capacity, int *count)
{
    struct fixture *f = context;
    const char text[] = "A\tB\r\n\001\214Z\232IGNORED";
    if (f->longfile) { memset(buffer, 'X', capacity); *count = capacity; return FM_FS_OK; }
    assert(!f->reads++);
    *count = sizeof(text) - 1; memcpy(buffer, text, *count); return FM_FS_END;
}
static int close_file(void *context, int handle) { ++((struct fixture *)context)->closed; return FM_FS_OK; }
int main(void)
{
    struct fixture f;
    struct fm_filesystem files;
    struct fm_printer printer;
    struct fm_print_job job;
    struct fm_invoker port;
    struct fm_location location;
    struct fm_entry entries[2];
    struct fm_request request;
    struct fm_reply reply;
    struct fm_job_result result;
    int mode, status;
    const char expected[] = "A       B\r\n \fZ\fA       B\r\n \fZ\f";
    memset(&files, 0, sizeof(files));
    files.context = &f; files.file_open = open_file;
    files.file_read = read_file; files.file_close = close_file;
    printer.context = &f; printer.attach = attach; printer.put = put; printer.detach = detach;
    memset(&location, 0, sizeof(location)); memset(entries, 0, sizeof(entries));
    strcpy(entries[0].name, "ONE.TXT"); strcpy(entries[1].name, "TWO.TXT");
    memset(&request, 0, sizeof(request)); request.command = FM_CMD_PRINT;
    request.token = 1; request.generation = 2; request.count = 2; request.files[1] = 1;
    memset(&reply, 0, sizeof(reply)); reply.token = 1;
    for (mode = 0; mode < 4; ++mode) {
        memset(&f, 0, sizeof(f)); f.unavailable = mode == 1;
        f.failed = mode == 2; f.longfile = mode == 3;
        fm_print_bind(&port, &job, &files, &printer, &location);
        assert(!fm_print_configure(&job, entries, 2));
        assert(!port.invoke(port.context, &request));
        assert(fm_print_configure(&job, 0, 0) == FM_BAD_MODEL);
        reply.token = 2;
        assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
        assert(!f.attached); reply.token = 1;
        status = port.advance(port.context, &reply, &result);
        if (!mode) {
            assert(!status && result.completed == 1);
            assert(!port.advance(port.context, &reply, &result));
            assert(result.state == FM_JOB_COMPLETE && result.completed == 2);
            assert(f.used == (int)sizeof(expected) - 1 && !memcmp(f.output, expected, f.used));
        } else if (mode == 3) assert(!status && !result.completed);
        else assert(status == FM_FS_ERROR);
        port.cancel(port.context);
        assert(f.opened == f.closed && f.attached == f.detached);
        assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
        port.cancel(port.context);
        assert(f.opened == f.closed && f.attached == f.detached);
    }
    puts("PASS: Print conversion, multiple files, stale requests, cancellation, printer refusal and cleanup");
    return 0;
}
