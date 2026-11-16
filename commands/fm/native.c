/* Concurrent 3.1/4.1 PC-window adapter. Screen ownership never spans keyboard/BDOS I/O.
 * The gateway ABI is shared with WINDOW. Serial TERM support is not claimed.
 */
#include "fm.h"
static unsigned system_segment;
int XSEG(void), XINIT(void);
int XCALL(int function, int bx, int cx, int dx);
int FARWORD(unsigned segment, unsigned offset);
int FARBYTE(unsigned segment, unsigned offset);
int FMLOCK(unsigned segment, unsigned window, int release);
int FMCOPY(unsigned segment, const char *text, const char *attributes, int count);
int BDOS(int function, ...);
static unsigned window, image;
static int console_number, expansion, opened;

int fm_native_open(void)
{
    unsigned table, video;
    int count, version;

    version = BDOS(12, 0) & 65535;
    if (version != 0x1441 && version != 0x1431)
        return FM_BAD_MODEL;
    XINIT();
    system_segment = XSEG();
    table = XCALL(16, 0, 0, 255);
    if (!table || table > 65530)
        return FM_BAD_MODEL;
    count = FARBYTE(system_segment, table + 1);
    console_number = BDOS(153, 0) & 255;
    if (count < 1 || console_number >= count)
        return FM_BAD_MODEL;
    window = XCALL(16, 0, 0, console_number);
    if (!window || window > 65482)
        return FM_BAD_MODEL;
    image = FARWORD(system_segment, window + 22);
    video = FARWORD(system_segment, window + 24);
    if (!image || (video != 0xb000 && video != 0xb800))
        return FM_BAD_MODEL;
    expansion = FARBYTE(system_segment, window + 51);
    BDOS(6, 27);
    BDOS(6, '6');
    BDOS(6, 27);
    BDOS(6, 'f');
    opened = 1;
    return FM_SUCCESS;
}

int fm_native_present(const struct fm_canvas *canvas)
{
    int changed;

    if (!opened || !canvas || !FMLOCK(system_segment, window, 0))
        return FM_OUTPUT_ERROR;
    /* One bounded transfer, not 1,920 calls across the far-memory boundary.
     * Compare against the authoritative window image: no duplicate shadow
     * buffer, and external window redraws cannot invalidate a private cache. */
    changed = FMCOPY(image, &canvas->text[0][0], &canvas->attribute[0][0], FM_ROWS * FM_COLUMNS);
    FMLOCK(system_segment, window, 1);
    if (changed)
        XCALL(20, FARWORD(system_segment, window + 4), FARWORD(system_segment, window + 2),
              console_number);
    return FM_SUCCESS;
}

int fm_native_read(void)
{
    int key;

    if (!(BDOS(6, 254) & 1)) {
        BDOS(141, 1); /* One scheduler tick; never busy-wait. */
        return FM_KEY_TICK;
    }
    key = BDOS(6, 253) & 255;
    return fm_key_decode(key);
}

void fm_native_close(void)
{
    if (!opened)
        return;
    BDOS(6, 27);
    BDOS(6, expansion ? '7' : '6');
    BDOS(6, 27);
    BDOS(6, 'e');
    BDOS(6, 27);
    BDOS(6, 'E');
    opened = 0;
}

/* Original FM uses the current logical printer; no hidden printer override. */
#include "print.h"

static int printer_attach(void *context)
{
    return (BDOS(161, 0) & 255) ? FM_FS_ERROR : FM_FS_OK;
}

static int printer_put(void *context, int byte)
{
    BDOS(5, byte & 255);
    return FM_FS_OK;
}

static int printer_detach(void *context)
{
    BDOS(159, 0);
    return FM_FS_OK;
}

void fm_native_printer_bind(struct fm_printer *printer)
{
    printer->context = 0;
    printer->attach = printer_attach;
    printer->put = printer_put;
    printer->detach = printer_detach;
}
