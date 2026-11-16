/*
 * Native transaction tests: inject BDOS, exercise real rename/rollback code. The
 * in-memory directory is separate from the action-level storage fake.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "fsnative.h"
struct file {
    int used, size;
    char name[11], data[512];
};
static struct file files[8];
static int user = 7, drive = 3, rename_calls, fail_renames, fail_close, fail_write;
static char restore[128], *dma = restore;
static int check_fat_close, byte_updates;
unsigned short XDATA;
int XSEG(void) { return XDATA; }
int XINIT(void) {
    return 0;
}
int FARWORD(int segment, int offset) {
    return 1;
}
int FMLOC(int drive_number, int *identity) {
    return 0;
}
static int find(const char *name) {
    int i, j;
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 11; ++j)
            if ((files[i].name[j] & 127) != (name[j] & 127)) break;
        if (files[i].used && j == 11) return i;
    }
    return -1;
}
int BDOS(int function,...) {
    va_list args;
    char *fcb, **packet;
    int value = 0, i, j, number;
    va_start(args, function);
    switch (function) {
    case 12:
        value = 0x1441;
        break;
    case 25:
        value = drive;
        break;
    case 14:
        drive = va_arg(args, int);
        break;
    case 32:
        number = va_arg(args, int);
        if (number == 255)
            value = user;
        else
            user = number;
        break;
    case 26:
        dma = va_arg(args, char *);
        break;
    case 152:
        packet = va_arg(args, char **);
        j = 1;
        for (i = 0; packet[0][i]; ++i) {
            if (packet[0][i] == '.')
                j = 9;
            else
                packet[1][j++] = packet[0][i];
        }
        break;
    case 17:
        fcb = va_arg(args, char *);
        i = find(fcb + 1);
        assert(user == 5 && dma != restore);
        if (i < 0)
            value = 255;
        else {
            memset(dma, 0, 128);
            dma[0] = 5;
            memcpy(dma + 1, files[i].name, 11);
        }
        break;
    case 22:
        fcb = va_arg(args, char *);
        assert(user == 5 && find(fcb + 1) < 0);
        for (i = 0; i < 8 && files[i].used; ++i) {
        }
        assert(i < 8);
        files[i].used = 1;
        files[i].size = 0;
        memcpy(files[i].name, fcb + 1, 11);
        fcb[24] = (char)i;
        break;
    case 21:
        fcb = va_arg(args, char *);
        i = fcb[24];
        assert(user == 5 && files[i].used);
        if (fail_write) {
            value = 2;
            break;
        }
        assert(files[i].size + 128 <= 512);
        memcpy(files[i].data + files[i].size, dma, 128);
        files[i].size += 128;
        break;
    case 16:
        fcb = va_arg(args, char *);
        if (check_fat_close) {
            /* Our shim leaves checksum-protected size bytes zero. */
            assert(!fcb[28] && !fcb[29] && !fcb[30] && !fcb[31]);
        }
        value = fail_close ? 255 : 0;
        break;
    case 30:
        fcb = va_arg(args, char *);
        if (check_fat_close) {
            assert((fcb[6] & 128) && fcb[32] == 3);
            ++byte_updates;
        } else {
            i = find(fcb + 1);
            assert(i >= 0 && user == 5 && dma != restore);
            memcpy(files[i].name, fcb + 1, 11);
        }
        break;
    case 23:
        fcb = va_arg(args, char *);
        i = find(fcb + 1);
        ++rename_calls;
        if ((fail_renames & (1 << rename_calls)) || i < 0 || find(fcb + 17) >= 0)
            value = 255;
        else
            memcpy(files[i].name, fcb + 17, 11);
        break;
    case 19:
        fcb = va_arg(args, char *);
        i = find(fcb + 1);
        if (i < 0)
            value = 255;
        else
            files[i].used = 0;
        break;
    default:
        assert(!"Unexpected native Copy call");
    }
    va_end(args);
    return value;
}
static void reset(struct fm_copy_store *port, struct fm_native_copy *copy,
    struct fm_native_fs *source) {
    memset(files, 0, sizeof(files));
    memset(source, 0, sizeof(*source));
    memset(copy, 0, sizeof(*copy));
    source->restore_dma = restore;
    files[0].used = 1;
    files[0].size = 3;
    memcpy(files[0].name, "FILE    DAT", 11);
    memcpy(files[0].data, "OLD", 3);
    fail_renames = fail_close = fail_write = rename_calls = 0;
    check_fat_close = byte_updates = 0;
    fm_native_copy_bind(port, copy, source, restore);
}
int main(void) {
    struct fm_copy_store port;
    struct fm_native_copy copy;
    struct fm_native_fs native;
    struct fm_location source, destination;
    struct fm_mutation_store changes;
    struct fm_native_mutation mutation;
    int exists, index;
    memset(&source, 0, sizeof(source));
    memset(&destination, 0, sizeof(destination));
    source.user = 4;
    destination.user = 5;
    reset(&port, &copy, &native);
    assert(port.prepare(port.context, &source, &source, "FILE.DAT", &exists));
    assert(!port.prepare(port.context, &source, &destination, "FILE.DAT", &exists) && exists);
    assert(!port.begin(port.context));
    assert(!port.write(port.context, "NEW", 3));
    assert(!port.commit(port.context, 1));
    index = find("FILE    DAT");
    assert(index >= 0 && !memcmp(files[index].data, "NEW", 3));
    assert(!copy.owned && !copy.backed && !port.abort(port.context));
    assert(user == 7 && drive == 3 && dma == restore);

    reset(&port, &copy, &native);
    assert(!port.prepare(port.context, &source, &destination, "FILE.DAT", &exists));
    assert(!port.begin(port.context));
    assert(!port.write(port.context, "NEW", 3));
    fail_renames = 1 << 2;      /* Publication fails; rollback succeeds. */
    assert(port.commit(port.context, 1));
    assert(!port.abort(port.context));
    index = find("FILE    DAT");
    assert(index >= 0 && !memcmp(files[index].data, "OLD", 3));
    assert(!copy.owned && !copy.backed);

    reset(&port, &copy, &native);
    assert(!port.prepare(port.context, &source, &destination, "FILE.DAT", &exists));
    assert(!port.begin(port.context));
    assert(!port.write(port.context, "NEW", 3));
    fail_renames = (1 << 2) | (1 << 3);
    assert(port.commit(port.context, 1));
    assert(port.abort(port.context));
    assert(copy.backed && strstr(port.error(port.context), "Recovery"));
    assert(files[0].used && !memcmp(files[0].data, "OLD", 3));

    reset(&port, &copy, &native);
    assert(!port.prepare(port.context, &source, &destination, "FILE.DAT", &exists));
    assert(!port.begin(port.context));
    fail_close = 1;
    assert(port.abort(port.context));
    assert(copy.owned && copy.opened);
    assert(!memcmp(files[0].data, "OLD", 3));
    fail_close = 0;
    assert(!port.abort(port.context));
    assert(!copy.owned && !copy.opened && user == 7 && dma == restore);
    /* Exercise FAT length finalization independently of alias mapping.
     * CLOSE must receive the original size fields; F6'/CR updates follow it. */
    reset(&port, &copy, &native);
    assert(!port.prepare(port.context, &source, &destination, "FILE.DAT", &exists));
    assert(!port.begin(port.context));
    copy.destination.media = FM_MEDIA_FAT;
    check_fat_close = 1;
    assert(!port.write(port.context, "NEW", 3));
    assert(!port.commit(port.context, 1));
    assert(byte_updates == 1 && !copy.opened && !copy.owned);
    assert(!port.abort(port.context));

    /* The same native FCB adapter must preserve CP/M user/DMA state. */
    reset(&port, &copy, &native);
    fm_native_mutation_bind(&changes, &mutation, &copy);
    assert(!changes.prepare(changes.context, &destination, "FILE.DAT", "NEW.DAT"));
    assert(!changes.commit(changes.context));
    assert(find("FILE    DAT") < 0 && find("NEW     DAT") >= 0);
    assert(changes.commit(changes.context)); /* No replay. */
    assert(changes.prepare(changes.context, &destination, "NEW.DAT", "NEW.DAT"));
    assert(!changes.prepare(changes.context, &destination, "NEW.DAT", 0));
    assert(!changes.commit(changes.context));
    assert(find("NEW     DAT") < 0);
    assert(user == 7 && drive == 3 && dma == restore);
    reset(&port, &copy, &native);
    fm_native_mutation_bind(&changes, &mutation, &copy);
    assert(!changes.prepare(changes.context, &destination, "FILE.DAT", 0));
    files[0].used = 0; /* File disappeared while confirmation was pending. */
    assert(changes.commit(changes.context));
    assert(changes.prepare(changes.context, &destination, "*.DAT", 0));
    assert(user == 7 && drive == 3 && dma == restore);
    reset(&port, &copy, &native);
    fm_native_mutation_bind(&changes, &mutation, &copy);
    files[0].name[0] |= 128; /* Preserve F1 and Archive while changing RO/SYS. */
    files[0].name[10] |= 128;
    assert(!changes.prepare_attributes(changes.context, &destination, "FILE.DAT", 1));
    assert(!changes.commit(changes.context));
    assert((files[0].name[8] & 128) && (files[0].name[9] & 128));
    assert((files[0].name[0] & 128) && (files[0].name[10] & 128));
    assert(changes.commit(changes.context));
    assert(!changes.prepare_attributes(changes.context, &destination, "FILE.DAT", 0));
    assert(!changes.commit(changes.context));
    assert(!(files[0].name[8] & 128) && !(files[0].name[9] & 128));
    assert((files[0].name[0] & 128) && (files[0].name[10] & 128));
    assert(user == 7 && drive == 3 && dma == restore);
    puts("PASS: SYS/RO and DIR/RW attributes, unrelated flags retained, no replay");
    puts("PASS: native mutations, target collision, stale source, replay and process-state restoration");

    puts("PASS: native Copy self-check, publication, rollback, retained recovery, close failure and process-state restoration");
    return 0;
}
