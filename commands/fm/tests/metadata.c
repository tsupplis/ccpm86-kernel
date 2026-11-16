/* Pure metadata presentation: boundaries, Gregorian dates and missing data. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "location.h"
int main(void)
{
    struct fm_metadata data;
    char text[FM_DETAIL_CAPACITY];
    memset(&data, 0, sizeof(data));
    fm_metadata_format(&data, text);
    assert(!strcmp(text, "      --  --"));
    data.size_known = data.date_known = 1;
    data.size_low = 1025;
    data.days = 1;
    data.hour = 23;
    data.minute = 59;
    fm_metadata_format(&data, text);
    assert(!strcmp(text, "   2 KiB  1978-01-01 23:59"));
    data.days = 790;
    fm_metadata_format(&data, text);
    assert(strstr(text, "1980-02-29"));
    data.size_high = 0xffff;
    data.size_low = 0xffff;
    fm_metadata_format(&data, text);
    assert(strstr(text, "4096 MiB"));
    data.minute = 60;
    fm_metadata_format(&data, text);
    assert(strstr(text, "  --"));
    puts("PASS: size rounding, large sizes, epoch, leap day and unavailable dates");
    return 0;
}
