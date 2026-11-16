/* Concurrent 3.1/4.1 filesystem boundary. No UI or controller dependencies.
 * Explicit FCB drives avoid changing the default drive during file/search I/O.
 * Each call restores user and the composition root's DMA buffer.
 */
#include <stdio.h>
#include "fsnative.h"
int BDOS(int, ...);
int XINIT(void);
int XSEG(void);
int FARWORD(int, int);
int FMLOC(int, int *);

static void clear_fcb(struct fm_native_fs *state)
{
    int index;
    for (index = 0; index < 36; ++index)
        state->fcb[index] = 0;
    for (index = 1; index < 12; ++index)
        state->fcb[index] = ' ';
}

static int enter(struct fm_native_fs *state)
{
    int user;
    user = BDOS(32, 255) & 255;
    BDOS(32, state->user);
    BDOS(26, state->dma);
    return user;
}

static void leave(struct fm_native_fs *state, int user)
{
    BDOS(26, state->restore_dma);
    BDOS(32, user);
}

static int drives(void *context, unsigned *mask)
{
    struct fm_native_fs *state = context;
    int index;
    if (!mask || (state->version != 0x1441 && state->version != 0x1431))
        return FM_FS_ERROR;
    XINIT();
    *mask = 0;
    for (index = 0; index < 16; ++index) {
        if (FARWORD(XSEG(), 0xc18 + index * 2))
            *mask |= (unsigned)1 << index;
    }
    return FM_FS_OK;
}

/* The special '?' search returns the media descriptor. This consumes search
 * state, so probing while a cursor/file is active is explicitly rejected. */
static int probe(void *context, int drive, int *media)
{
    struct fm_native_fs *state;
    int saved_drive, user, index, result;
    unsigned mask;

    state = (struct fm_native_fs *)context;
    if (!media || state->active || drive < 0 || drive > 15 || drives(context, &mask) ||
        !(mask & ((unsigned)1 << drive)))
        return FM_FS_ERROR;
    /* CCP/M has CP/M media only. Do not issue CDOS's descriptor search. */
    if (state->version == 0x1431) {
        *media = FM_MEDIA_CPM;
        return FM_FS_OK;
    }
    saved_drive = BDOS(25, 0) & 255;
    result = FM_FS_ERROR;
    if ((BDOS(14, drive) & 255) != 255) {
        clear_fcb(state);
        state->fcb[0] = '?';
        for (index = 0; index < 128; ++index)
            state->dma[index] = 0;
        user = enter(state);
        BDOS(17, state->fcb);
        *media = (state->dma[0] & 255) == 32 && (state->dma[15] & 255) == 128 ? FM_MEDIA_FAT
                                                                              : FM_MEDIA_CPM;
        leave(state, user);
        result = FM_FS_OK;
    }
    if ((BDOS(14, saved_drive) & 255) == 255)
        result = FM_FS_ERROR;
    return result;
}

/* Coordinates are absolute and bounded. Native names are parsed by BDOS;
 * path separators and traversal components are handled here, not by FCB I/O. */
static int valid_location(FM_CONST struct fm_location *location)
{
    int length;
    if (!location || location->drive < 0 || location->drive > 15 || location->user < 0 ||
        location->user > 15 || (location->media != FM_MEDIA_CPM && location->media != FM_MEDIA_FAT))
        return 0;
    if (location->media == FM_MEDIA_CPM)
        return 1;
    for (length = 0; length < FM_PATH_CAPACITY; ++length)
        if (!location->path[length])
            return length && location->path[0] == '\\';
    return 0;
}

static int parse_name(struct fm_native_fs *state, FM_CONST char *name)
{
    char *packet[2];
    int index;
    if (!name || !name[0])
        return FM_FS_ERROR;
    for (index = 0; name[index]; ++index) {
        if (index >= 12 || name[index] == ':' || name[index] == '\\' || name[index] == '/' ||
            name[index] == '*' || name[index] == '?')
            return FM_FS_ERROR;
    }
    clear_fcb(state);
    packet[0] = (char *)name;
    packet[1] = state->fcb;
    return (BDOS(152, packet) & 255) == 255 ? FM_FS_ERROR : FM_FS_OK;
}

/* Reserve a free process-local N:/O:/P: mapping. Only this owned alias is
 * navigated; source/default mappings are never changed. Extended CLOSE moves
 * the alias to root; it does not release it. CDOS reclaims it on process exit.
 * Each reuse checks identity so another component's mapping cannot be stolen.
 */
static int map_location(struct fm_native_fs *state, FM_CONST struct fm_location *location)
{
    int identity[4], candidate, index, start, length, user, result;
    char component[FM_NAME_CAPACITY];
    state->media = location->media;
    state->user = location->media == FM_MEDIA_CPM ? location->user : 0;
    state->effective_drive = location->drive + 1;
    if (location->media == FM_MEDIA_CPM)
        return FM_FS_OK;
    if (state->version != 0x1441)
        return FM_FS_ERROR;
    if (state->alias) {
        if (!FMLOC(state->alias, identity))
            return FM_FS_ERROR;
        for (index = 0; index < 4; ++index)
            if (identity[index] != state->identity[index])
                return FM_FS_ERROR;
    } else {
        for (candidate = 14; candidate <= 16; ++candidate) {
            if (candidate != location->drive + 1 && FMLOC(candidate, identity) && !identity[3])
                break;
        }
        if (candidate > 16)
            return FM_FS_ERROR;
        state->alias = candidate;
    }
    user = enter(state);
    clear_fcb(state);
    state->fcb[0] = (location->drive + 1) | 128;
    state->fcb[1] = '.';
    state->fcb[12] = state->alias - 13;
    result = BDOS(15, state->fcb) & 255;
    if (!result) {
        clear_fcb(state);
        state->fcb[0] = state->alias | 128;
        result = BDOS(16, state->fcb) & 255;
    }
    start = 1;
    while (!result && location->path[start]) {
        length = 0;
        while (location->path[start] && location->path[start] != '\\') {
            if (length == FM_NAME_CAPACITY - 1) {
                result = 255;
                break;
            }
            component[length++] = location->path[start++];
        }
        component[length] = 0;
        if (result || !length || component[0] == '.' || parse_name(state, component)) {
            result = 255;
            break;
        }
        state->fcb[0] = state->alias | 128;
        result = BDOS(15, state->fcb) & 255;
        if (location->path[start] == '\\')
            ++start;
    }
    /* Retain identity even after a failed descent: a later valid request may
     * safely reset our alias to root. Failure to identify it prevents reuse. */
    if (!FMLOC(state->alias, state->identity))
        result = 255;
    leave(state, user);
    state->effective_drive = state->alias;
    return result ? FM_FS_ERROR : FM_FS_OK;
}

static int list_open(void *context, FM_CONST struct fm_location *location, int *handle)
{
    struct fm_native_fs *state;
    int index;
    state = (struct fm_native_fs *)context;
    if (state->active || !handle || !valid_location(location))
        return FM_FS_ERROR;
    if (map_location(state, location))
        return FM_FS_ERROR;
    clear_fcb(state);
    state->fcb[0] = state->effective_drive | (state->media == FM_MEDIA_FAT ? 128 : 0);
    for (index = 1; index < 12; ++index)
        state->fcb[index] = '?';
    state->first = 1;
    state->active = *handle = 1;
    return FM_FS_OK;
}

static int list_next(void *context, int handle, struct fm_entry *output)
{
    struct fm_native_fs *state;
    char *entry;
    int user, slot, index, used;
    state = (struct fm_native_fs *)context;
    if (!output || handle != 1 || state->active != 1)
        return FM_FS_ERROR;
    user = enter(state);
    for (;;) {
        state->fcb[0] = state->effective_drive | (state->media == FM_MEDIA_FAT ? 128 : 0);
        slot = BDOS(state->first ? 17 : 18, state->fcb) & 255;
        state->first = 0;
        if (slot > 3) {
            leave(state, user);
            return slot == 255 ? FM_FS_END : FM_FS_ERROR;
        }
        entry = state->dma + slot * 32;
        /* Only the first logical extent represents a new filename. */
        if ((entry[0] & 255) != state->user ||
            (state->media == FM_MEDIA_CPM && ((entry[12] & 31) || (entry[14] & 63))))
            continue;
        used = 0;
        for (index = 1; index < 9; ++index) {
            if ((entry[index] & 127) != ' ')
                output->name[used++] = entry[index] & 127;
        }
        if ((entry[9] & 127) != ' ')
            output->name[used++] = '.';
        for (index = 9; index < 12; ++index) {
            if ((entry[index] & 127) != ' ')
                output->name[used++] = entry[index] & 127;
        }
        output->name[used] = 0;
        output->directory =
            state->media == FM_MEDIA_FAT && entry[16] == entry[17] && (entry[16] & 192) == 192;
        if (output->directory && output->name[0] == '.')
            continue;
        leave(state, user);
        return FM_FS_OK;
    }
}

static int list_close(void *context, int handle)
{
    struct fm_native_fs *state;
    state = (struct fm_native_fs *)context;
    if (handle != 1 || state->active != 1)
        return FM_FS_ERROR;
    state->active = 0;
    return FM_FS_OK;
}

static int file_open(void *context, FM_CONST struct fm_location *location, FM_CONST char *name,
                     int *handle)
{
    struct fm_native_fs *state;
    int user, result;
    state = (struct fm_native_fs *)context;
    if (state->active || !handle || !valid_location(location) || parse_name(state, name) ||
        map_location(state, location) || parse_name(state, name))
        return FM_FS_ERROR;
    state->fcb[0] = state->effective_drive;
    user = enter(state);
    result = BDOS(15, state->fcb) & 255;
    leave(state, user);
    if (result == 255)
        return FM_FS_ERROR;
    state->remaining_low = (state->fcb[28] & 255) | ((unsigned)(state->fcb[29] & 255) << 8);
    state->remaining_high = (state->fcb[30] & 255) | ((unsigned)(state->fcb[31] & 255) << 8);
    state->position = state->available = 0;
    state->active = *handle = 2;
    return FM_FS_OK;
}

static int file_read(void *context, int handle, char *output, int capacity, int *count)
{
    struct fm_native_fs *state;
    int user, result;
    state = (struct fm_native_fs *)context;
    if (!output || !count || capacity < 1 || handle != 2 || state->active != 2)
        return FM_FS_ERROR;
    *count = 0;
    if (state->media == FM_MEDIA_FAT && !state->remaining_low && !state->remaining_high)
        return FM_FS_END;
    if (state->position == state->available) {
        user = enter(state);
        result = BDOS(20, state->fcb) & 255;
        leave(state, user);
        if (result)
            return result == 1 ? FM_FS_END : FM_FS_ERROR;
        state->position = 0;
        state->available = 128;
        if (state->media == FM_MEDIA_FAT && !state->remaining_high && state->remaining_low < 128)
            state->available = state->remaining_low;
    }
    while (*count < capacity && state->position < state->available) {
        output[(*count)++] = state->dma[state->position++];
        if (state->media == FM_MEDIA_FAT) {
            if (!state->remaining_low)
                --state->remaining_high;
            --state->remaining_low;
        }
    }
    return FM_FS_OK;
}

static int file_close(void *context, int handle)
{
    struct fm_native_fs *state;
    int user, result;
    state = (struct fm_native_fs *)context;
    if (handle != 2 || state->active != 2)
        return FM_FS_ERROR;
    user = enter(state);
    result = BDOS(16, state->fcb) & 255;
    leave(state, user);
    state->active = 0;
    return result == 255 ? FM_FS_ERROR : FM_FS_OK;
}

/* Metadata queries run outside enumeration, so BDOS search state cannot be
 * disturbed. Function 35 includes all extents; function 102 returns the update
 * stamp at FCB 28 on CP/M, FCB 20 on FAT (CDOS black.a86:xfcb).
 * Missing timestamps do not invalidate a known file size. */
static int metadata(void *context, FM_CONST struct fm_location *location, FM_CONST char *name,
                    struct fm_metadata *output)
{
    struct fm_native_fs *state;
    int user, result, hour, minute, stamp;
    unsigned low, middle, high;

    state = (struct fm_native_fs *)context;
    output->size_known = output->date_known = 0;
    if (state->active || !valid_location(location) || map_location(state, location) ||
        parse_name(state, name))
        return FM_FS_ERROR;
    state->fcb[0] = state->effective_drive;
    user = enter(state);
    result = BDOS(35, state->fcb) & 255;
    leave(state, user);
    if (result == 255)
        return FM_FS_ERROR;
    low = state->fcb[33] & 255;
    middle = state->fcb[34] & 255;
    high = state->fcb[35] & 255;
    output->size_low = (low << 7) | ((middle & 1) << 15);
    output->size_high = (middle >> 1) | (high << 7);
    output->size_known = 1;
    if (parse_name(state, name))
        return FM_FS_ERROR;
    state->fcb[0] = state->effective_drive;
    user = enter(state);
    result = BDOS(102, state->fcb) & 255;
    leave(state, user);
    if (result == 255)
        return FM_FS_OK;
    stamp = location->media == FM_MEDIA_FAT ? 20 : 28;
    output->days = (state->fcb[stamp] & 255) | ((state->fcb[stamp + 1] & 255) << 8);
    hour = state->fcb[stamp + 2] & 255;
    minute = state->fcb[stamp + 3] & 255;
    if ((hour & 15) > 9 || (minute & 15) > 9)
        return FM_FS_OK;
    output->hour = (hour >> 4) * 10 + (hour & 15);
    output->minute = (minute >> 4) * 10 + (minute & 15);
    output->date_known = output->days && output->hour < 24 && output->minute < 60;
    return FM_FS_OK;
}

/* Function 46 returns a 24-bit count of free 128-byte records for either
 * filesystem. Query after closing enumeration; it uses the process DMA. */
static int space(void *context, int drive, unsigned long *kib)
{
    struct fm_native_fs *state = context;
    unsigned long records;
    int user, status;

    if (!kib || state->active || drive < 0 || drive > 15)
        return FM_FS_ERROR;
    user = enter(state);
    memset(state->dma, 255, 3);
    status = BDOS(46, drive);
    records = (unsigned char)state->dma[0] | ((unsigned long)(unsigned char)state->dma[1] << 8) |
              ((unsigned long)(unsigned char)state->dma[2] << 16);
    leave(state, user);
    if ((unsigned)status == 65535U || records == 0xffffffUL)
        return FM_FS_ERROR;
    *kib = records / 8;
    return FM_FS_OK;
}

void fm_native_fs_bind(struct fm_filesystem *port, struct fm_native_fs *state, char *restore_dma)
{
    state->version = BDOS(12, 0) & 65535;
    state->restore_dma = restore_dma;
    state->active = state->user = state->alias = 0;
    port->context = state;
    port->drives = drives;
    port->probe = probe;
    port->list_open = list_open;
    port->list_next = list_next;
    port->list_close = list_close;
    port->file_open = file_open;
    port->file_read = file_read;
    port->file_close = file_close;
    port->metadata = metadata;
    port->space = space;
}

/* Transactional destination adapter. Scratch FCB calls are used only when the
 * temporary file is closed. Source and destination use separate native state. */

static int lookup(struct fm_native_copy *copy, const char *name, int *found, char *snapshot,
                  int allow_readonly)
{
    struct fm_native_fs *state;
    char *entry;
    int user, slot, index;
    state = &copy->destination;
    if (parse_name(state, name))
        return FM_FS_ERROR;
    state->fcb[0] = state->effective_drive | (state->media == FM_MEDIA_FAT ? 128 : 0);
    user = enter(state);
    slot = BDOS(17, state->fcb) & 255;
    *found = slot <= 3;
    if (*found) {
        entry = state->dma + slot * 32;
        if (((entry[9] & 128) && !allow_readonly) ||
            (state->media == FM_MEDIA_FAT && entry[16] == entry[17] && (entry[16] & 192) == 192)) {
            leave(state, user);
            strcpy(copy->message, "Destination is protected or is a directory.");
            return FM_FS_ERROR;
        }
        if (snapshot)
            for (index = 0; index < 32; ++index)
                snapshot[index] = entry[index];
    }
    leave(state, user);
    return slot <= 3 || slot == 255 ? FM_FS_OK : FM_FS_ERROR;
}

static int copy_lookup(struct fm_native_copy *copy, const char *name, int *found, char *snapshot)
{
    return lookup(copy, name, found, snapshot, 0);
}

static int copy_prepare(void *context, FM_CONST struct fm_location *source,
                        FM_CONST struct fm_location *destination, FM_CONST char *name, int *exists)
{
    struct fm_native_copy *copy;
    int source_id[4], destination_id[4];
    copy = (struct fm_native_copy *)context;
    strcpy(copy->message, "I/O error; source retained.");
    if (copy->opened || copy->owned || copy->backed || copy->source->active ||
        (source && !valid_location(source)) || !valid_location(destination) ||
        parse_name(&copy->destination, name) || (source && map_location(copy->source, source)) ||
        map_location(&copy->destination, destination))
        return FM_FS_ERROR;
    if (source && source->media == destination->media) {
        if (source->media == FM_MEDIA_CPM) {
            if (source->drive == destination->drive && source->user == destination->user) {
                strcpy(copy->message, "Source and target: same location.");
                return FM_FS_ERROR;
            }
        } else {
            if (!FMLOC(copy->source->effective_drive, source_id) ||
                !FMLOC(copy->destination.effective_drive, destination_id))
                return FM_FS_ERROR;
            if (source_id[0] == destination_id[0] && source_id[2] == destination_id[2]) {
                strcpy(copy->message, "Source and target: same directory.");
                return FM_FS_ERROR;
            }
        }
    }
    strcpy(copy->name, name);
    if (copy_lookup(copy, name, &copy->exists, copy->original))
        return FM_FS_ERROR;
    *exists = copy->exists;
    return FM_FS_OK;
}

static int copy_unused(struct fm_native_copy *copy, char *name, FM_CONST char *prefix)
{
    int attempt, found;
    for (attempt = 0; attempt < 32; ++attempt) {
        /* DR C uses uppercase X for a long argument. Lowercase x consumes
         * our 16-bit sequence; the BDOS parser normalizes the filename. */
        sprintf(name, "%s%04x.$$$", prefix, ++copy->sequence);
        if (copy_lookup(copy, name, &found, (char *)0)) {
            sprintf(copy->message, "Cannot check temporary %.12s", name);
            return FM_FS_ERROR;
        }
        if (!found)
            return FM_FS_OK;
    }
    strcpy(copy->message, "No free temporary filename.");
    return FM_FS_ERROR;
}

static int copy_begin(void *context)
{
    struct fm_native_copy *copy;
    struct fm_native_fs *state;
    int user, result;
    copy = (struct fm_native_copy *)context;
    state = &copy->destination;
    if (copy->owned || copy->opened)
        return FM_FS_ERROR;
    if (copy_unused(copy, copy->temporary, "FM"))
        return FM_FS_ERROR;
    if (parse_name(state, copy->temporary)) {
        strcpy(copy->message, "Cannot parse temporary filename.");
        return FM_FS_ERROR;
    }
    state->fcb[0] = state->effective_drive;
    user = enter(state);
    result = BDOS(22, state->fcb) & 255;
    leave(state, user);
    if (result == 255) {
        strcpy(copy->message, "Cannot create temporary file.");
        return FM_FS_ERROR;
    }
    copy->owned = copy->opened = 1;
    copy->filled = copy->bytes_low = copy->bytes_high = 0;
    return FM_FS_OK;
}

static int copy_record(struct fm_native_copy *copy)
{
    int user, result;
    user = enter(&copy->destination);
    result = BDOS(21, copy->destination.fcb) & 255;
    leave(&copy->destination, user);
    if (result)
        return FM_FS_ERROR;
    copy->filled = 0;
    return FM_FS_OK;
}

static int copy_write(void *context, FM_CONST char *data, int count)
{
    struct fm_native_copy *copy;
    unsigned before;
    int index;
    copy = (struct fm_native_copy *)context;
    if (!copy->opened || !data || count < 0 || count > 128)
        return FM_FS_ERROR;
    for (index = 0; index < count; ++index) {
        copy->destination.dma[copy->filled++] = data[index];
        if (copy->filled == 128 && copy_record(copy))
            return FM_FS_ERROR;
    }
    before = copy->bytes_low;
    copy->bytes_low = (before + count) & 65535;
    if (copy->bytes_low < before)
        ++copy->bytes_high;
    return FM_FS_OK;
}

static int copy_close(struct fm_native_copy *copy)
{
    struct fm_native_fs *state;
    int user, result;
    state = &copy->destination;
    if (!copy->opened)
        return FM_FS_OK;
    /* FCB bytes 28..31 participate in the kernel checksum. Never edit them
     * before CLOSE. Exact FAT length uses the documented F6'/CR attribute
     * update on the closed file, as the DOS personality does. */
    user = enter(state);
    result = BDOS(16, state->fcb) & 255;
    leave(state, user);
    if (result == 255)
        return FM_FS_ERROR;
    copy->opened = 0;
    if (state->media == FM_MEDIA_FAT && (copy->bytes_low & 127)) {
        state->fcb[6] |= 128;
        state->fcb[32] = copy->bytes_low & 127;
        user = enter(state);
        result = BDOS(30, state->fcb) & 255;
        leave(state, user);
        if (result == 255) {
            strcpy(copy->message, "Cannot set exact destination length.");
            return FM_FS_ERROR;
        }
    }
    return FM_FS_OK;
}

static int copy_rename(struct fm_native_copy *copy, FM_CONST char *oldname, FM_CONST char *newname)
{
    struct fm_native_fs *state;
    char old[12];
    int index, user, result;
    state = &copy->destination;
    if (parse_name(state, oldname))
        return FM_FS_ERROR;
    for (index = 1; index < 12; ++index)
        old[index] = state->fcb[index];
    if (parse_name(state, newname))
        return FM_FS_ERROR;
    for (index = 1; index < 12; ++index) {
        state->fcb[16 + index] = state->fcb[index];
        state->fcb[index] = old[index];
    }
    state->fcb[0] = state->fcb[16] = state->effective_drive;
    user = enter(state);
    result = BDOS(23, state->fcb) & 255;
    leave(state, user);
    return result == 255 ? FM_FS_ERROR : FM_FS_OK;
}

static int copy_remove(struct fm_native_copy *copy, FM_CONST char *name)
{
    struct fm_native_fs *state;
    int user, result;
    state = &copy->destination;
    if (parse_name(state, name))
        return FM_FS_ERROR;
    state->fcb[0] = state->effective_drive;
    user = enter(state);
    result = BDOS(19, state->fcb) & 255;
    leave(state, user);
    return result == 255 ? FM_FS_ERROR : FM_FS_OK;
}

static int copy_commit(void *context, int replace)
{
    struct fm_native_copy *copy;
    char current[32];
    int index, found;
    copy = (struct fm_native_copy *)context;
    if (!copy->opened || !copy->owned)
        return FM_FS_ERROR;
    if (copy->filled) {
        for (index = copy->filled; index < 128; ++index)
            copy->destination.dma[index] = 26;
        if (copy_record(copy))
            return FM_FS_ERROR;
    }
    if (copy_close(copy) || copy_lookup(copy, copy->name, &found, current))
        return FM_FS_ERROR;
    if (found != copy->exists || (found && !replace))
        return FM_FS_ERROR;
    /* Recheck the original before replacing it: preparation may be old. */
    if (found) {
        for (index = 0; index < 32; ++index)
            if (current[index] != copy->original[index]) {
                strcpy(copy->message, "Destination changed during Copy; not replaced.");
                return FM_FS_ERROR;
            }
        if (copy_unused(copy, copy->backup, "FB") || copy_rename(copy, copy->name, copy->backup))
            return FM_FS_ERROR;
        copy->backed = 1;
    }
    /* Keep the old file under its backup name until publication succeeds.
     * Failed rollback retains that backup; abort must never remove it. */
    if (copy_rename(copy, copy->temporary, copy->name)) {
        if (copy->backed && !copy_rename(copy, copy->backup, copy->name))
            copy->backed = 0;
        return FM_FS_ERROR;
    }
    copy->owned = 0;
    if (copy->backed) {
        if (copy_remove(copy, copy->backup))
            return FM_FS_ERROR;
        copy->backed = 0;
    }
    return FM_FS_OK;
}

static int copy_abort(void *context)
{
    struct fm_native_copy *copy;
    int result;
    copy = (struct fm_native_copy *)context;
    result = copy_close(copy);
    /* Never destroy an unclosed file or a backup required for recovery. */
    if (!result && copy->owned) {
        result = copy_remove(copy, copy->temporary);
        if (!result)
            copy->owned = 0;
    }
    if (copy->backed) {
        sprintf(copy->message, "Recovery file retained: %.12s", copy->backup);
        return FM_FS_ERROR;
    }
    if (copy->owned)
        sprintf(copy->message, "Temporary retained: %.12s", copy->temporary);
    return result;
}

static FM_CONST char *copy_error(void *context)
{
    return ((struct fm_native_copy *)context)->message;
}

void fm_native_copy_bind(struct fm_copy_store *port, struct fm_native_copy *state,
                         struct fm_native_fs *source, char *restore_dma)
{
    state->source = source;
    state->destination.version = source->version;
    state->destination.restore_dma = restore_dma;
    state->destination.active = state->destination.alias = 0;
    state->owned = state->backed = state->opened = state->sequence = 0;
    state->message[0] = 0;
    port->context = state;
    port->prepare = copy_prepare;
    port->begin = copy_begin;
    port->write = copy_write;
    port->commit = copy_commit;
    port->abort = copy_abort;
    port->error = copy_error;
}

/* Rename/Delete share the closed-FCB machinery used by Copy, but never own a
 * temporary or backup. Confirmation holds no open file or filesystem lock.
 * Revalidation detects changes across that wait; BDOS enforces final locking.
 */
static int mutation_prepare(void *context, const struct fm_location *location, const char *name,
                            const char *replacement)
{
    struct fm_native_mutation *mutation = context;
    struct fm_native_copy *files = mutation->files;
    int found;

    mutation->prepared = 0;
    mutation->attributes = 0;
    strcpy(files->message, "Cannot prepare file change.");
    if (!valid_location(location) || !fm_mutation_name(name) ||
        (replacement && !fm_mutation_name(replacement)) || files->source->active || files->opened ||
        files->owned || files->backed || map_location(&files->destination, location))
        return FM_FS_ERROR;
    if (copy_lookup(files, name, &found, files->original) || !found)
        return FM_FS_ERROR;
    strcpy(files->name, name);
    mutation->replacement[0] = 0;
    if (replacement) {
        if (copy_lookup(files, replacement, &found, 0))
            return FM_FS_ERROR;
        if (found) {
            strcpy(files->message, "Rename target already exists; nothing changed.");
            return FM_FS_ERROR;
        }
        strcpy(mutation->replacement, replacement);
    }
    mutation->prepared = 1;
    return FM_FS_OK;
}

static int attributes_prepare(void *context, const struct fm_location *location, const char *name,
                              int system)
{
    struct fm_native_mutation *mutation = context;
    struct fm_native_copy *files = mutation->files;
    int found;

    mutation->prepared = 0;
    strcpy(files->message, "Cannot prepare attribute change.");
    if (!valid_location(location) || !fm_mutation_name(name) || files->source->active ||
        files->opened || files->owned || files->backed ||
        map_location(&files->destination, location) ||
        lookup(files, name, &found, files->original, 1) || !found)
        return FM_FS_ERROR;
    strcpy(files->name, name);
    mutation->attributes = system ? 2 : 1;
    mutation->prepared = 1;
    return FM_FS_OK;
}

static int mutation_commit(void *context)
{
    struct fm_native_mutation *mutation = context;
    struct fm_native_copy *files = mutation->files;
    char current[32];
    int found, result, user, index;
    struct fm_native_fs *state = &files->destination;

    if (!mutation->prepared)
        return FM_FS_ERROR;
    mutation->prepared = 0;
    strcpy(files->message, "File changed or unavailable; nothing changed.");
    if (files->source->active || files->opened || files->owned || files->backed ||
        lookup(files, files->name, &found, current, mutation->attributes != 0) || !found ||
        memcmp(current, files->original, sizeof(current)))
        return FM_FS_ERROR;
    if (mutation->attributes) {
        if (parse_name(state, files->name))
            return FM_FS_ERROR;
        for (index = 1; index <= 11; ++index)
            state->fcb[index] = current[index];
        state->fcb[9] &= 127;
        state->fcb[10] &= 127;
        if (mutation->attributes == 2) {
            state->fcb[9] |= 128;
            state->fcb[10] |= 128;
        }
        state->fcb[0] = state->effective_drive;
        user = enter(state);
        result = (BDOS(30, state->fcb) & 255) == 255 ? FM_FS_ERROR : FM_FS_OK;
        leave(state, user);
    } else if (mutation->replacement[0]) {
        if (copy_lookup(files, mutation->replacement, &found, 0) || found)
            return FM_FS_ERROR;
        result = copy_rename(files, files->name, mutation->replacement);
    } else {
        result = copy_remove(files, files->name);
    }
    if (result)
        strcpy(files->message, "File operation failed; listing will be refreshed.");
    return result;
}

static const char *mutation_error(void *context)
{
    return ((struct fm_native_mutation *)context)->files->message;
}

void fm_native_mutation_bind(struct fm_mutation_store *port, struct fm_native_mutation *state,
                             struct fm_native_copy *files)
{
    state->files = files;
    state->prepared = 0;
    port->context = state;
    port->prepare = mutation_prepare;
    port->commit = mutation_commit;
    port->error = mutation_error;
    port->prepare_attributes = attributes_prepare;
}

/* Reuse the same directory alias and validation as file operations. */
int fm_native_run_location(struct fm_native_fs *state, const struct fm_location *location,
                           int *drive, int *user)
{
    if (!state || state->active || !drive || !user || !valid_location(location) ||
        map_location(state, location))
        return FM_FS_ERROR;
    *drive = state->effective_drive - 1;
    *user = state->user;
    return FM_FS_OK;
}

/* Resolve permanent media through its DPH/DPB, as the historical FM does.
 * This is read-only. Generated controls always live at physical root/user 0. */
int fm_native_backup_location(struct fm_native_fs *state, const struct fm_location *location,
                              struct fm_location *root)
{
    int identity[4], physical;
    unsigned dph, dpb;

    if (state->version != 0x1441 || state->active || !valid_location(location))
        return FM_FS_ERROR;
    physical = location->drive + 1;
    if (location->media == FM_MEDIA_FAT) {
        if (!FMLOC(physical, identity))
            return FM_FS_ERROR;
        physical = identity[0];
    }
    if (physical < 1 || physical > 13)
        return FM_FS_ERROR;
    XINIT();
    dph = FARWORD(XSEG(), 0xc18 + 2 * (physical - 1));
    if (!dph || dph > 65525U)
        return FM_FS_ERROR;
    dpb = FARWORD(XSEG(), dph + 8);
    if (!dpb || dpb > 65510U)
        return FM_FS_ERROR;
    if ((unsigned)FARWORD(XSEG(), dpb) >= 65520U)
        dpb += 12;
    if (!(FARWORD(XSEG(), dpb + 11) & 32768U))
        return FM_FS_ERROR;
    *root = *location;
    root->drive = physical - 1;
    root->user = 0;
    strcpy(root->path, "\\");
    return FM_FS_OK;
}

/* Discover a physical drive's current path through our private alias. Each
 * parent is searched for the child cluster. No foreign alias is overwritten. */
static int saved_path(struct fm_native_fs *state, int drive, char *path)
{
    struct fm_location root;
    int original[4], identity[4], after[4];
    int user, status, child, slot, length, used = 0, i;
    char name[13], *entry;

    if (!FMLOC(drive, original))
        return FM_FS_ERROR;
    if (!original[2]) {
        strcpy(path, "\\");
        return FM_FS_OK;
    }
    root.drive = drive - 1;
    root.media = FM_MEDIA_FAT;
    root.user = 0;
    strcpy(root.path, "\\");
    if (map_location(state, &root))
        return FM_FS_ERROR;
    user = enter(state);
    clear_fcb(state);
    state->fcb[0] = drive | 128;
    state->fcb[1] = '.';
    state->fcb[12] = state->alias - 13;
    status = BDOS(15, state->fcb) & 255;
    path[0] = 0;
    while (!status) {
        if (!FMLOC(state->alias, identity) || identity[0] != original[0] ||
            identity[1] != original[1]) {
            status = 255;
            break;
        }
        child = identity[2];
        if (!child)
            break;
        clear_fcb(state);
        state->fcb[0] = state->alias | 128;
        state->fcb[1] = state->fcb[2] = '.';
        status = BDOS(15, state->fcb) & 255;
        if (status)
            break;
        clear_fcb(state);
        memset(state->fcb + 1, '?', 11);
        state->fcb[0] = state->alias | 128;
        slot = BDOS(17, state->fcb) & 255;
        while (slot <= 3) {
            entry = state->dma + slot * 32;
            if ((entry[16] & 64) && (entry[1] & 127) != '.' &&
                ((entry[26] & 255) | ((entry[27] & 255) << 8)) == child)
                break;
            slot = BDOS(18, state->fcb) & 255;
        }
        if (slot > 3) {
            status = 255;
            break;
        }
        length = 0;
        for (i = 1; i <= 8 && (entry[i] & 127) != ' '; ++i)
            name[length++] = entry[i] & 127;
        if ((entry[9] & 127) != ' ') {
            name[length++] = '.';
            for (i = 9; i <= 11 && (entry[i] & 127) != ' '; ++i)
                name[length++] = entry[i] & 127;
        }
        name[length] = 0;
        if (!fm_mutation_name(name) || used + length + 2 >= FM_PATH_CAPACITY) {
            status = 255;
            break;
        }
        memmove(path + length + 1, path, used + 1);
        path[0] = '\\';
        memcpy(path + 1, name, length);
        used += length + 1;
    }
    if (!FMLOC(drive, after) || memcmp(original, after, sizeof(original)))
        status = 255;
    /* Retain the alias's actual identity on errors, so a later use is safe. */
    if (!FMLOC(state->alias, state->identity))
        status = 255;
    leave(state, user);
    if (!used)
        strcpy(path, "\\");
    return status ? FM_FS_ERROR : FM_FS_OK;
}

int fm_native_root_enter(struct fm_native_fs *state, const struct fm_location *root,
                         struct fm_root_guard *guard)
{
    int identity[4], user, status;

    guard->active = 0;
    if (state->version != 0x1441)
        return FM_FS_ERROR;
    if (root->media == FM_MEDIA_CPM)
        return FM_FS_OK;
    if (state->active || !FMLOC(root->drive + 1, identity) ||
        saved_path(state, root->drive + 1, guard->path))
        return FM_FS_ERROR;
    guard->drive = root->drive + 1;
    /* Unmapped physical roots have no sequence value to retain. */
    guard->sequence = identity[3] ? identity[1] : -1;
    guard->active = 1;
    user = enter(state);
    clear_fcb(state);
    state->fcb[0] = guard->drive | 128;
    state->fcb[1] = '.';
    status = BDOS(16, state->fcb) & 255;
    leave(state, user);
    return status ? FM_FS_ERROR : FM_FS_OK;
}

int fm_native_root_leave(struct fm_native_fs *state, struct fm_root_guard *guard)
{
    int identity[4], user, status, start, length;
    char name[13];

    if (!guard->active)
        return FM_FS_OK;
    guard->active = 0;
    if (state->active || !FMLOC(guard->drive, identity) || identity[0] != guard->drive ||
        (guard->sequence >= 0 && identity[3] && identity[1] != guard->sequence))
        return FM_FS_ERROR;
    user = enter(state);
    clear_fcb(state);
    state->fcb[0] = guard->drive | 128;
    state->fcb[1] = '.';
    status = BDOS(16, state->fcb) & 255;
    start = 1;
    while (!status && guard->path[start]) {
        length = 0;
        while (guard->path[start] && guard->path[start] != '\\') {
            if (length == 12) {
                status = 255;
                break;
            }
            name[length++] = guard->path[start++];
        }
        name[length] = 0;
        if (status || !fm_mutation_name(name) || parse_name(state, name)) {
            status = 255;
            break;
        }
        state->fcb[0] = guard->drive | 128;
        status = BDOS(15, state->fcb) & 255;
        if (guard->path[start] == '\\')
            ++start;
    }
    leave(state, user);
    return status ? FM_FS_ERROR : FM_FS_OK;
}
