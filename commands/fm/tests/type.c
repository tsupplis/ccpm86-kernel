/* Read-only Type tests through an injected byte source; no native runtime/UI. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "type.h"
struct source {
    const char *text;
    int size, position, chunk, closes, fail, close_error;
};
static int open_file(void *context, const struct fm_location *location,
                     const char *name, int *handle)
{
    struct source *s = context;
    s->position = 0;
    *handle = 1;
    return FM_FS_OK;
}
static int read_file(void *context, int handle, char *buffer, int capacity, int *count)
{
    struct source *s = context;
    int n = s->size - s->position;
    if (s->fail) return FM_FS_ERROR;
    if (n > s->chunk) n = s->chunk;
    if (n > capacity) n = capacity;
    memcpy(buffer, s->text + s->position, n);
    s->position += n;
    *count = n;
    return s->position == s->size ? FM_FS_END : FM_FS_OK;
}
static int close_file(void *context, int handle)
{
    struct source *s = context;
    ++s->closes;
    return s->close_error ? FM_FS_ERROR : FM_FS_OK;
}
/* Same invocation port as Copy: stale replies, cooperative replay and cleanup. */
static void lifecycle(void)
{
    struct fm_type_job job;
    struct fm_filesystem fs;
    struct fm_invoker port;
    struct fm_location location;
    struct fm_entry entry;
    struct fm_request request;
    struct fm_reply reply;
    struct fm_job_result result;
    struct source source;
    char text[1000];
    int row, length;
    memset(&job, 0, sizeof(job));
    memset(&fs, 0, sizeof(fs));
    memset(&location, 0, sizeof(location));
    memset(&source, 0, sizeof(source));
    strcpy(entry.name, "PAGES.TXT"); entry.directory = 0;
    length = 0;
    for (row = 0; row < 80; ++row) length += sprintf(text + length, "LINE%03d\r\n", row);
    source.text = text; source.size = length; source.chunk = 128;
    fs.context = &source; fs.file_open = open_file; fs.file_read = read_file; fs.file_close = close_file;
    fm_type_job_bind(&port, &job, &fs, &location, &entry, 1);
    request.command = FM_CMD_TYPE; request.token = 7; request.generation = 9;
    request.count = 1; request.files[0] = 0;
    assert(!port.invoke(port.context, &request));
    reply.token = 8; reply.decision = 0; reply.answer = FM_REPLY_NONE;
    assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL && source.position == 0);
    reply.token = 7;
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_VIEW && !strcmp(result.page->lines[0], "LINE000"));
    reply.answer = FM_REPLY_NEXT;
    assert(!port.advance(port.context, &reply, &result));
    assert(!strcmp(result.page->lines[0], "LINE018"));
    assert(!port.advance(port.context, &reply, &result));
    assert(!strcmp(result.page->lines[0], "LINE036"));
    reply.answer = FM_REPLY_PREVIOUS;
    assert(!port.advance(port.context, &reply, &result) && result.state == FM_JOB_PROGRESS);
    reply.answer = FM_REPLY_NONE;
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_VIEW && !strcmp(result.page->lines[0], "LINE018"));
    reply.answer = FM_REPLY_LAST;
    assert(!port.advance(port.context, &reply, &result));
    reply.answer = FM_REPLY_NONE;
    while (result.state == FM_JOB_PROGRESS) assert(!port.advance(port.context, &reply, &result));
    assert(job.reader.eof && !strcmp(result.page->lines[0], "LINE072"));
    reply.answer = FM_REPLY_FIRST;
    assert(!port.advance(port.context, &reply, &result));
    assert(!strcmp(result.page->lines[0], "LINE000"));
    reply.answer = FM_REPLY_LAST;
    assert(!port.advance(port.context, &reply, &result) && result.state == FM_JOB_PROGRESS);
    assert(!port.cancel(port.context) && !job.reader.opened && !job.active);

    /* An exact-sized page remains visible after the EOF probe. */
    source.size = 18 * 9;
    assert(!port.invoke(port.context, &request));
    reply.answer = FM_REPLY_NONE;
    assert(!port.advance(port.context, &reply, &result));
    reply.answer = FM_REPLY_NEXT;
    assert(!port.advance(port.context, &reply, &result));
    assert(job.reader.eof && result.page->count == 18 && job.page_number == 0);
    assert(!strcmp(result.page->lines[0], "LINE000"));
    assert(!port.advance(port.context, &reply, &result));
    assert(result.state == FM_JOB_COMPLETE && result.completed == 1);
    assert(!port.cancel(port.context));
    entry.directory = 1;
    assert(port.invoke(port.context, &request) == FM_BAD_MODEL);
    entry.directory = 0;
    source.size = length;
    assert(!port.invoke(port.context, &request));
    source.close_error = 1;
    assert(port.cancel(port.context) == FM_FS_ERROR && !job.reader.opened);
    assert(!port.cancel(port.context));
}

int main(void)
{
    struct fm_type state;
    struct fm_filesystem fs;
    struct fm_location location;
    struct source source;
    char text[1500];
    int i;
    memset(&state, 0, sizeof(state));
    memset(&fs, 0, sizeof(fs));
    memset(&source, 0, sizeof(source));
    memset(&location, 0, sizeof(location));
    fs.context = &source;
    fs.file_open = open_file;
    fs.file_read = read_file;
    fs.file_close = close_file;
    source.text = "first\r\nsecond\nthird\rtab\tend\032padding";
    source.size = strlen(source.text);
    source.chunk = 1;
    assert(!fm_type_begin(&state, &fs, &location, "TEST.TXT"));
    assert(!fm_type_next(&state));
    assert(!strcmp(state.lines[0], "first"));
    assert(!strcmp(state.lines[1], "second"));
    assert(!strcmp(state.lines[2], "third"));
    assert(!strcmp(state.lines[3], "tab     end"));
    assert(state.eof && source.closes == 1);
    assert(fm_type_next(&state) == FM_FS_END);
    assert(!fm_type_close(&state) && source.closes == 1);

    for (i = 0; i < 18 * 76; ++i) text[i] = 'x';
    strcpy(text + i, "\r\nlast");
    source.text = text;
    source.size = strlen(text);
    source.chunk = 128;
    assert(!fm_type_begin(&state, &fs, &location, "LONG.TXT"));
    assert(!fm_type_next(&state));
    assert(state.rows == 18 && strlen(state.lines[17]) == 76);
    assert(!fm_type_next(&state));
    assert(!strcmp(state.lines[0], "last") && state.rows == 1);

    for (i = 0; i < 17 * 76 + 75; ++i) text[i] = 'x';
    strcpy(text + i, "\tZ");
    source.size = strlen(text);
    assert(!fm_type_begin(&state, &fs, &location, "TAB.TXT"));
    assert(!fm_type_next(&state));
    assert(!fm_type_next(&state));
    assert(!strcmp(state.lines[0], "    Z"));

    /* A wrapped full page followed only by CR/LF and ^Z must survive the
     * EOF probe. Exercise byte-at-a-time reads across both terminators. */
    for (i = 0; i < 18 * 76; ++i) text[i] = 'q';
    strcpy(text + i, "\r\n\032padding");
    source.size = strlen(text);
    source.chunk = 1;
    assert(!fm_type_begin(&state, &fs, &location, "EXACT.TXT"));
    assert(!fm_type_next(&state));
    assert(fm_type_next(&state) == FM_FS_END);
    assert(state.rows == 18 && state.lines[0][0] == 'q');
    assert(strlen(state.lines[17]) == 76);

    /* Blank lines are content: replacing the preserved page must clear it. */
    source.text = "\r\n";
    source.size = 2;
    assert(!fm_type_begin(&state, &fs, &location, "BLANK.TXT"));
    assert(!fm_type_next(&state));
    assert(state.rows == 1 && state.lines[0][0] == 0);

    source.size = 0;
    assert(!fm_type_begin(&state, &fs, &location, "EMPTY.TXT"));
    assert(fm_type_next(&state) == FM_FS_END);
    source.fail = 1;
    assert(!fm_type_begin(&state, &fs, &location, "BAD.TXT"));
    assert(fm_type_next(&state) == FM_FS_ERROR && !state.opened);
    source.fail = 0;
    source.close_error = 1;
    assert(!fm_type_begin(&state, &fs, &location, "CLOSE.TXT"));
    assert(fm_type_close(&state) == FM_FS_ERROR && !state.opened);
    lifecycle();
    puts("PASS: injected Type reads, newline boundaries, wrapping, Ctrl-Z, EOF, errors, cleanup, shared lifecycle, replay and cancellation");
    return 0;
}
