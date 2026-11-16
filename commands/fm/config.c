/* FM.CFG service: bounded parsing and transactional read through an injected
 * filesystem. Reference: original config_match_keyword/config_read_decimal;
 * extra/fm.cfg documents all eight colour assignments and reverse intensity.
 * Unlike the original byte-wrapping parser, invalid colours are ignored.
 */
#include "config.h"

static FM_CONST char *regions[] = {"OBJECT", "COMMAND", "PROMPT", "KEY"};
static FM_CONST char *geometry[] = {
    "OVPMODE=",    "MENU POSITION=", "MENU SIZE=",   "PROMPT SIZE=",
    "TOP MARGIN=", "BOTTOM MARGIN=", "LEFT MARGIN=", "RIGHT MARGIN="};

/* Match the exact keyword spacing, ignoring ASCII case only. */
static FM_CONST char *match(FM_CONST char *text, FM_CONST char *keyword)
{
    int character;
    while (*keyword) {
        character = *text;
        if (character >= 'a' && character <= 'z')
            character -= 'a' - 'A';
        if (character != *keyword++)
            return 0;
        ++text;
    }
    return text;
}

/* Configuration numbers have limits of 15 or 255. Check before multiplying
 * so long input cannot wrap a 16-bit int. Keep the historical suffix handling. */
static int decimal(FM_CONST char *text, int maximum)
{
    int number, digit;

    while (*text == ' ')
        ++text;
    if (*text < '0' || *text > '9')
        return -1;
    number = 0;
    while (*text >= '0' && *text <= '9') {
        digit = *text++ - '0';
        if (number > 25 || number * 10 + digit > maximum)
            return -1;
        number = number * 10 + digit;
    }
    return number;
}

void fm_config_defaults(struct fm_config *config)
{
    static int foreground[] = {0, 0, 15, 14};
    static int background[] = {5, 4, 1, 1};
    static int layout[] = {2, 1, 17, 1, 0, 2, 0, 0};
    int index;
    for (index = 0; index < FM_CFG_REGIONS; ++index) {
        config->foreground[index] = foreground[index];
        config->background[index] = background[index];
    }
    for (index = 0; index < FM_CFG_GEOMETRY; ++index)
        config->geometry[index] = layout[index];
    config->unsupported = 0;
}

void fm_config_line(struct fm_config *config, FM_CONST char *line)
{
    FM_CONST char *value, *suffix;
    int region, background, number, index;

    for (index = 0; index < FM_CFG_GEOMETRY; ++index) {
        value = match(line, geometry[index]);
        if (!value)
            continue;
        number = decimal(value, 255);
        if (number < 0)
            return;
        config->geometry[index] = number;
        if (index == FM_CFG_OVPMODE)
            config->unsupported = number != 2;
        return;
    }
    for (region = 0; region < FM_CFG_REGIONS; ++region) {
        suffix = match(line, regions[region]);
        if (!suffix)
            continue;
        value = match(suffix, " FG COLOR=");
        background = 0;
        if (!value) {
            value = match(suffix, " BG COLOR=");
            background = 1;
        }
        if (!value)
            return;
        number = decimal(value, 15);
        if (number < 0)
            return;
        if (background)
            config->background[region] = number;
        else
            config->foreground[region] = number;
        return;
    }
}

int fm_config_load(struct fm_config *config, struct fm_filesystem *filesystem,
                   FM_CONST struct fm_location *location)
{
    struct fm_config candidate;
    char record[128], line[82];
    int handle, status, count, index, character, used, overflow, ended, failed;

    if ((*filesystem->file_open)(filesystem->context, location, "FM.CFG", &handle))
        return FM_CFG_ABSENT;
    candidate = *config;
    used = overflow = ended = failed = 0;
    while (!ended) {
        count = 0;
        status =
            (*filesystem->file_read)(filesystem->context, handle, record, sizeof(record), &count);
        if ((status != FM_FS_OK && status != FM_FS_END) || count < 0 ||
            count > (int)sizeof(record) || (!count && status == FM_FS_OK)) {
            failed = 1;
            break;
        }
        for (index = 0; index < count && !ended; ++index) {
            character = record[index] & 255;
            if (character == 13 || character == 10 || character == 26) {
                line[used] = 0;
                if (!overflow)
                    fm_config_line(&candidate, line);
                used = overflow = 0;
                ended = character == 26;
            } else if (used < (int)sizeof(line) - 1) {
                line[used++] = character;
            } else {
                overflow = 1;
            }
        }
        if (status == FM_FS_END)
            ended = 1;
    }
    if (used && !overflow) {
        line[used] = 0;
        fm_config_line(&candidate, line);
    }
    if ((*filesystem->file_close)(filesystem->context, handle))
        failed = 1;
    if (failed)
        return FM_CFG_ERROR;
    *config = candidate;
    return FM_CFG_LOADED;
}
