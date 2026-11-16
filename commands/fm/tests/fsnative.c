/* Native adapter contract test. The linked BDOS shim models the OS boundary;
 * application/controllers still use the normal injected filesystem interface.
 */
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "fsnative.h"
unsigned short XDATA;
static int user = 7, drive = 3, searches, reads, closes, read_error, close_error;
static int no_stamp, space_error;
static int version = 0x1441;
static unsigned long free_records;
static int fat, alias_mapped, alias_cluster, alias_busy, map_error;
int FMLOC(int number, int *identity)
{
    assert(version == 0x1441);
    if (!fat) return 0;
    identity[0] = 1; identity[1] = 1;
    identity[2] = alias_cluster;
    identity[3] = alias_busy || (number == 14 && alias_mapped);
    return 1;
}
static char *dma;
static char restore[128];
int XSEG(void) { return XDATA; }
int XINIT(void) { return 0; }
int FARWORD(int segment, int offset) { return offset == 0xc18; }
int BDOS(int function, ...)
{
    va_list args;
    int value, index;
    char *fcb;
    char **packet;
    va_start(args, function);
    value = 0;
    switch (function) {
    case 12: value = version; break;
    case 14: drive = va_arg(args, int); break;
    case 25: value = drive; break;
    case 26: dma = va_arg(args, char *); break;
    case 32:
        value = va_arg(args, int);
        if (value == 255) value = user;
        else { user = value; value = 0; }
        break;
    case 152:
        packet = va_arg(args, char **);
        memcpy(packet[1] + 1, "TEST    TXT", 11);
        break;
    case 15:
        fcb = va_arg(args, char *);
        assert(dma != restore && user == (fat ? 0 : 4));
        if (fat && (fcb[0] & 128)) {
            if (map_error) { value = 255; break; }
            if (fcb[12]) { assert(fcb[12] == 1); alias_mapped = 1; }
            else { assert((fcb[0] & 127) == 14); alias_cluster = 2; }
            break;
        }
        assert(fcb[0] == (fat ? 14 : 1));
        if (fat) fcb[28] = 3;
        reads = 0;
        break;
    case 16:
        fcb = va_arg(args, char *);
        assert(user == (fat ? 0 : 4) && dma != restore);
        if (fat && (fcb[0] & 128)) { alias_cluster = 0; break; }
        ++closes;
        value = close_error ? 255 : 0;
        break;
    case 17: case 18:
        fcb = va_arg(args, char *);
        assert((fcb[0] & 255) == (fat ? (128 | 14) : 1) && user == (fat ? 0 : 4) && dma != restore);
        if (function == 17) searches = 0;
        if (searches == 3) { value = 255; break; }
        memset(dma, 0, 128);
        dma[0] = user;
        memcpy(dma + 1, searches < 2 ? "FIRST   TXT" : "SECOND  TXT", 11);
        dma[12] = searches == 1 ? 1 : 0;
        if (fat) {
            memcpy(dma + 1, searches == 0 ? "DOCS       " : "TEST    TXT", 11);
            dma[16] = dma[17] = (char)(searches == 0 ? 192 : 128);
        }
        ++searches;
        break;
    case 46:
        assert(va_arg(args, int) == 0 && dma != restore);
        if (space_error) { value = 65535; break; }
        dma[0] = free_records & 255;
        dma[1] = (free_records >> 8) & 255;
        dma[2] = (free_records >> 16) & 255;
        break;
    case 35:
        fcb = va_arg(args, char *);
        fcb[33] = 9; fcb[34] = fcb[35] = 0;
        break;
    case 102:
        fcb = va_arg(args, char *);
        if (no_stamp) { value = 255; break; }
        index = fat ? 20 : 28;
        fcb[index] = 1;
        fcb[index + 1] = 0;
        fcb[index + 2] = 0x23;
        fcb[index + 3] = 0x59;
        break;
    case 20:
        assert(user == (fat ? 0 : 4) && dma != restore);
        if (read_error) { value = 255; break; }
        if (reads++) { value = 1; break; }
        for (index = 0; index < 128; ++index) dma[index] = index;
        break;
    default: assert(!"Unexpected BDOS call");
    }
    va_end(args);
    return value;
}
static void restored(void)
{
    assert(user == 7 && drive == 3 && dma == restore);
}
int main(void)
{
    struct fm_metadata metadata;
    struct fm_filesystem fs;
    struct fm_native_fs native;
    struct fm_location location;
    struct fm_entry entry;
    char data[128];
    int handle, count, i;
    unsigned mask;
    unsigned long kib;
    memset(&location, 0, sizeof(location));
    location.user = 4;
    dma = restore;
    fm_native_fs_bind(&fs, &native, restore);
    assert(!fs.drives(fs.context, &mask) && mask == 1);
    free_records = 43840UL;
    assert(!fs.space(fs.context, 0, &kib) && kib == 5480UL); restored();
    free_records = 524288UL;
    assert(!fs.space(fs.context, 0, &kib) && kib == 65536UL); restored();
    free_records = 0;
    assert(!fs.space(fs.context, 0, &kib) && kib == 0); restored();
    space_error = 1;
    assert(fs.space(fs.context, 0, &kib) == FM_FS_ERROR); restored();
    space_error = 0;
    assert(!fs.list_open(fs.context, &location, &handle));
    assert(fs.space(fs.context, 0, &kib) == FM_FS_ERROR);
    assert(!fs.list_next(fs.context, handle, &entry));
    assert(!strcmp(entry.name, "FIRST.TXT")); restored();
    assert(!fs.list_next(fs.context, handle, &entry));
    assert(!strcmp(entry.name, "SECOND.TXT")); restored();
    assert(fs.list_next(fs.context, handle, &entry) == FM_FS_END); restored();
    assert(!fs.list_close(fs.context, handle));
    assert(!fs.file_open(fs.context, &location, "TEST.TXT", &handle)); restored();
    assert(fs.list_open(fs.context, &location, &count) == FM_FS_ERROR);
    assert(!fs.file_read(fs.context, handle, data, 5, &count) && count == 5); restored();
    for (i = 0; i < count; ++i) assert(data[i] == i);
    assert(!fs.file_read(fs.context, handle, data, 128, &count) && count == 123); restored();
    for (i = 0; i < count; ++i) assert((data[i] & 255) == i + 5);
    assert(fs.file_read(fs.context, handle, data, 128, &count) == FM_FS_END); restored();
    assert(!fs.file_close(fs.context, handle) && closes == 1); restored();
    assert(!fs.file_open(fs.context, &location, "TEST.TXT", &handle));
    read_error = 1;
    assert(fs.file_read(fs.context, handle, data, 128, &count) == FM_FS_ERROR); restored();
    close_error = 1;
    assert(fs.file_close(fs.context, handle) == FM_FS_ERROR && !native.active); restored();
    location.media = FM_MEDIA_FAT;
    assert(fs.file_open(fs.context, &location, "TEST.TXT", &handle) == FM_FS_ERROR);
    fat = 1; read_error = close_error = 0;
    strcpy(location.path, "\\DOCS");
    assert(!fs.list_open(fs.context, &location, &handle)); restored();
    assert(alias_cluster == 2 && native.alias == 14);
    assert(!fs.list_next(fs.context, handle, &entry)); restored();
    assert(entry.directory && !strcmp(entry.name, "DOCS"));
    assert(!fs.list_close(fs.context, handle));
    assert(!fs.file_open(fs.context, &location, "TEST.TXT", &handle)); restored();
    assert(!fs.file_read(fs.context, handle, data, 128, &count) && count == 3); restored();
    assert(fs.file_read(fs.context, handle, data, 128, &count) == FM_FS_END && count == 0);
    assert(!fs.file_close(fs.context, handle)); restored();
    ++alias_cluster;
    assert(fs.list_open(fs.context, &location, &handle) == FM_FS_ERROR);
    fm_native_fs_bind(&fs, &native, restore);
    alias_busy = 1;
    assert(fs.list_open(fs.context, &location, &handle) == FM_FS_ERROR);
    alias_busy = alias_mapped = 0;
    fm_native_fs_bind(&fs, &native, restore);
    map_error = 1;
    assert(fs.list_open(fs.context, &location, &handle) == FM_FS_ERROR); restored();
    fat = map_error = 0;
    location.media = FM_MEDIA_CPM;
    location.user = 4;
    fm_native_fs_bind(&fs, &native, restore);
    assert(!fs.metadata(fs.context, &location, "TEST.TXT", &metadata));
    restored();
    assert(metadata.size_known && metadata.size_low == 1152 && !metadata.size_high);
    assert(metadata.date_known && metadata.days == 1 && metadata.hour == 23 && metadata.minute == 59);
    fat = 1;
    location.media = FM_MEDIA_FAT;
    fm_native_fs_bind(&fs, &native, restore);
    assert(!fs.metadata(fs.context, &location, "TEST.TXT", &metadata));
    restored();
    assert(metadata.date_known && metadata.days == 1 && metadata.hour == 23);
    no_stamp = 1;
    assert(!fs.metadata(fs.context, &location, "TEST.TXT", &metadata));
    restored();
    assert(metadata.size_known && !metadata.date_known);
    /* CCP/M probing must not perform CDOS's special descriptor search, and
     * even a forged FAT location must fail before FMLOC or directory I/O. */
    version = 0x1431;
    fat = 0;
    fm_native_fs_bind(&fs, &native, restore);
    assert(!fs.probe(fs.context, 0, &i) && i == FM_MEDIA_CPM);
    assert(fs.file_open(fs.context, &location, "TEST.TXT", &handle) == FM_FS_ERROR);
    assert(fs.list_open(fs.context, &location, &handle) == FM_FS_ERROR);
    assert(fm_native_backup_location(&native, &location, &location) == FM_FS_ERROR);
    location.media = FM_MEDIA_CPM;
    assert(!fs.file_open(fs.context, &location, "TEST.TXT", &handle));
    assert(!fs.file_read(fs.context, handle, data, 128, &count) && count == 128);
    assert(!fs.file_close(fs.context, handle));
    restored();
    puts("PASS: native adapter OS boundary, extent filtering, partial reads, EOF, exclusive handles, user/DMA restoration errors, FAT mapping ownership and exact byte EOF");
    return 0;
}
