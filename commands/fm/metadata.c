/* Presentation formatting for metadata. Pure, bounded, and independently
 * testable: no BDOS, filesystem, UI state or rendering dependencies. */
#include <stdio.h>
#include "location.h"

static int leap(int year)
{
    return !(year % 4) && ((year % 100) || !(year % 400));
}

/* output provides FM_DETAIL_CAPACITY bytes. Sizes are rounded up to KiB;
 * very large files use MiB to stay within native 16-bit arithmetic. */
void fm_metadata_format(FM_CONST struct fm_metadata *metadata, char *output)
{
    static int months[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    char size[12];
    unsigned days, amount;
    int year, month, length;

    strcpy(size, "--");
    if (metadata->size_known) {
        if (metadata->size_high < 1023) {
            amount = (metadata->size_high << 6) + (metadata->size_low >> 10);
            if (metadata->size_low & 1023)
                ++amount;
            snprintf(size, sizeof(size), "%u KiB", amount);
        } else {
            amount = metadata->size_high >> 4;
            if ((metadata->size_high & 15) || metadata->size_low)
                ++amount;
            snprintf(size, sizeof(size), "%u MiB", amount);
        }
    }
    if (!metadata->date_known || !metadata->days || metadata->hour < 0 || metadata->hour > 23 ||
        metadata->minute < 0 || metadata->minute > 59) {
        snprintf(output, FM_DETAIL_CAPACITY, "%8s  --", size);
        return;
    }
    days = metadata->days - 1;
    year = 1978;
    while (days >= (unsigned)(365 + leap(year))) {
        days -= 365 + leap(year);
        ++year;
    }
    month = 0;
    for (;;) {
        length = months[month] + (month == 1 && leap(year));
        if (days < (unsigned)length)
            break;
        days -= length;
        ++month;
    }
    snprintf(output, FM_DETAIL_CAPACITY, "%8s  %04d-%02d-%02d %02d:%02d", size, year, month + 1,
             days + 1, metadata->hour, metadata->minute);
}
