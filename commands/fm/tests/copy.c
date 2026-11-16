/*
 * Copy action tests through injected storage, with byte-exact publication. The fake
 * transaction publishes only on commit; failure/cancel retain old data.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "copy.h"
struct storage {
    unsigned char source[300], temporary[400], target[400];
    int position, used, size, opens, closes, commits, aborts, exists;
    int fail_write, fail_commit, fail_prepare, owned;
};
static int probe(void *context, int drive, int *media) {
    *media = drive ? FM_MEDIA_FAT : FM_MEDIA_CPM;
    return 0;
}
static int open_file(void *context, const struct fm_location *location, const char *name, int *handle) {
    struct storage *s = context;
    s->position = 0;
    ++s->opens;
    *handle = 1;
    return 0;
}
static int read_file(void *context, int handle, char *data, int capacity, int *count) {
    struct storage *s = context;
    *count = 300 - s->position;
    if (*count > 37)
        *count = 37;
    memcpy(data, s->source + s->position, *count);
    s->position += *count;
    return s->position == 300 ? FM_FS_END : FM_FS_OK;
}
static int close_file(void *context, int handle) {
    ++((struct storage *)context)->closes;
    return 0;
}
static int prepare(void *context, const struct fm_location *source,
    const struct fm_location *destination, const char *name, int *exists) {
    struct storage *s = context;
    *exists = s->exists;
    return s->fail_prepare;
}
static int begin(void *context) {
    struct storage *s = context;
    s->used = 0;
    s->owned = 1;
    return 0;
}
static int write_data(void *context, const char *data, int count) {
    struct storage *s = context;
    if (s->fail_write)
        return FM_FS_ERROR;
    assert(s->owned && s->used + count <= 400);
    memcpy(s->temporary + s->used, data, count);
    s->used += count;
    return 0;
}
static int commit(void *context, int replace) {
    struct storage *s = context;
    if (s->fail_commit)
        return FM_FS_ERROR;
    assert(!s->exists || replace);
    memcpy(s->target, s->temporary, s->used);
    s->size = s->used;
    s->owned = 0;
    ++s->commits;
    return 0;
}
static int abort_copy(void *context) {
    struct storage *s = context;
    s->owned = 0;
    ++s->aborts;
    return 0;
}
static const char *error(void *context) {
    return "Injected storage failure";
}
int main(void) {
    struct storage storage;
    struct fm_filesystem fs;
    struct fm_copy_store store;
    struct fm_copy_job job;
    struct fm_invoker port;
    struct fm_location source, destination;
    struct fm_entry entries[2];
    struct fm_request request;
    struct fm_reply reply;
    struct fm_job_result result;
    int i, steps;
    memset(&storage, 0, sizeof(storage));
    memset(&fs, 0, sizeof(fs));
    memset(&job, 0, sizeof(job));
    memset(&source, 0, sizeof(source));
    for (i = 0; i < 300; ++i)
        storage.source[i] = (unsigned char)i;
    strcpy(entries[0].name, "BINARY.DAT");
    entries[0].directory = 0;
    strcpy(entries[1].name, "SECOND.DAT");
    entries[1].directory = 0;
    fs.context = &storage;
    fs.probe = probe;
    fs.file_open = open_file;
    fs.file_read = read_file;
    fs.file_close = close_file;
    store.context = &storage;
    store.prepare = prepare;
    store.begin = begin;
    store.write = write_data;
    store.commit = commit;
    store.abort = abort_copy;
    store.error = error;
    fm_copy_bind(&port, &job, &fs, &store, &source, entries);
    assert(!fm_copy_configure(&job, 2, "A:15", 0));
    assert(fm_copy_configure(&job, -1, "A:15", 0) == FM_BAD_MODEL);
    assert(job.count == 2 && job.destination.user == 15);
    assert(!fm_copy_destination(&fs, "A:15", &destination) && destination.user == 15);
    assert(!fm_copy_destination(&fs, "b:\\DOCS", &destination) && !strcmp(destination.path, "\\DOCS"));
    assert(fm_copy_destination(&fs, "A:", &destination));
    assert(fm_copy_destination(&fs, "A:16", &destination));
    assert(fm_copy_destination(&fs, "B:DOCS", &destination));
    request.command = FM_CMD_COPY;
    request.token = 9;
    request.generation = 3;
    request.count = 2;
    request.files[0] = 0;
    request.files[1] = 1;
    reply.token = 9;
    reply.decision = 0;
    reply.answer = FM_REPLY_NONE;
    assert(!port.invoke(port.context, &request));
    assert(fm_copy_configure(&job, 0, "A:0", 0) == FM_BAD_MODEL);
    for (steps = 0; steps < 50; ++steps) {
        assert(!port.advance(port.context, &reply, &result));
        if (result.state == FM_JOB_COMPLETE)
            break;
    }
    assert(result.completed == 2 && storage.commits == 2 && storage.opens == storage.closes);
    assert(storage.size == 300 && !memcmp(storage.source, storage.target, 300));
    assert(!port.cancel(port.context));

    /* Existing target: decline, stale answer, accept, then injected failure. */
    request.count = 1;
    storage.exists = 1;
    memcpy(storage.target, "OLD", 3);
    storage.size = 3;
    assert(!port.invoke(port.context, &request));
    assert(!port.advance(port.context, &reply, &result) && result.state == FM_JOB_DECISION);
    reply.answer = FM_REPLY_ACCEPT;
    reply.decision = result.decision + 1;
    assert(port.advance(port.context, &reply, &result) == FM_BAD_MODEL);
    assert(!port.cancel(port.context) && !memcmp(storage.target, "OLD", 3));
    reply.answer = FM_REPLY_NONE;
    reply.decision = 0;
    assert(!port.invoke(port.context, &request));
    assert(!port.advance(port.context, &reply, &result));
    reply.answer = FM_REPLY_ACCEPT;
    reply.decision = result.decision;
    assert(!port.advance(port.context, &reply, &result));
    reply.answer = FM_REPLY_NONE;
    reply.decision = 0;
    assert(!port.advance(port.context, &reply, &result));
    storage.fail_write = 1;
    for (steps = 0; steps < 10; ++steps) {
        assert(!port.advance(port.context, &reply, &result));
        if (result.state == FM_JOB_FAILED)
            break;
    }
    assert(result.state == FM_JOB_FAILED);
    assert(!storage.owned && !memcmp(storage.target, "OLD", 3));
    assert(!port.cancel(port.context));
    storage.fail_write = 0;
    storage.exists = 0;
    assert(!port.invoke(port.context, &request));
    assert(!port.advance(port.context, &reply, &result));
    assert(!port.advance(port.context, &reply, &result));
    assert(!port.advance(port.context, &reply, &result));
    for (steps = 0; steps < 10 && !storage.used; ++steps)
        assert(!port.advance(port.context, &reply, &result));
    /* Eight short read/write pairs per advance, then cancellation before
     * close/publication. The ninth record must wait for the next advance. */
    assert(storage.position == 8 * 37 && storage.used == 8 * 37);
    assert(storage.owned && storage.used > 0);
    assert(!port.cancel(port.context) && !storage.owned && storage.opens == storage.closes);
    storage.fail_commit = 1;
    assert(!port.invoke(port.context, &request));
    for (steps = 0; steps < 30; ++steps) {
        assert(!port.advance(port.context, &reply, &result));
        if (result.state == FM_JOB_FAILED)
            break;
    }
    assert(result.state == FM_JOB_FAILED && !storage.owned && !memcmp(storage.target, "OLD", 3));
    assert(!port.cancel(port.context));
    entries[1].directory = 1;
    request.count = 2;
    assert(port.invoke(port.context, &request) == FM_BAD_MODEL);
    puts("PASS: binary Copy, short reads, multiple targets, destination syntax, confirmation, stale replies, write/commit failure and cancellation");
    return 0;
}
