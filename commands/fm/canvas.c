/* Counted-text canvas. Views supply spans; the runtime handles bulk fills. */
#include "fm.h"

static int begin_frame(void *context)
{
    struct fm_canvas *canvas = context;

    memset(canvas->text, ' ', sizeof(canvas->text));
    memset(canvas->attribute, 7, sizeof(canvas->attribute));
    return FM_SUCCESS;
}

static int write_run(void *context, int x, int y, int attribute, const char *text, int length)
{
    struct fm_canvas *canvas = context;

    if (!text || length < 0 || length > FM_COLUMNS || y < 0 || y >= FM_ROWS || x < 0 ||
        x > FM_COLUMNS - length)
        return FM_OUTPUT_ERROR;
    memcpy(&canvas->text[y][x], text, length);
    memset(&canvas->attribute[y][x], attribute, length);
    return FM_SUCCESS;
}

static int end_frame(void *context)
{
    return context ? FM_SUCCESS : FM_OUTPUT_ERROR;
}

void fm_canvas_bind(struct fm_display *display, struct fm_canvas *canvas)
{
    display->context = canvas;
    display->begin = begin_frame;
    display->write = write_run;
    display->end = end_frame;
}
