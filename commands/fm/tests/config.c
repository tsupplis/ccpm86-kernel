/* Configuration contracts: fake read service, real parser and model styling.
 * Errors must retain the caller's complete prior configuration. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
struct source {
    const char *text;
    int position, missing, fail_read, fail_close, closes;
};
static int open_file(void *context, const struct fm_location *location,
                     const char *name, int *handle)
{
    struct source *source = context;
    assert(!strcmp(name, "FM.CFG"));
    source->position = 0;
    *handle = 1;
    return source->missing ? FM_FS_ERROR : FM_FS_OK;
}
static int read_file(void *context, int handle, char *data, int capacity, int *count)
{
    struct source *source = context;
    int remaining = strlen(source->text + source->position);
    *count = remaining > 7 ? 7 : remaining; /* Split assignments across reads. */
    memcpy(data, source->text + source->position, *count);
    source->position += *count;
    if (source->fail_read && source->position > 21)
        return FM_FS_ERROR;
    return *count ? FM_FS_OK : FM_FS_END;
}
static int close_file(void *context, int handle)
{
    struct source *source = context;
    ++source->closes;
    return source->fail_close ? FM_FS_ERROR : FM_FS_OK;
}
int main(void)
{
    struct fm_config config;
    struct fm_filesystem fs;
    struct fm_location location;
    static struct fm_model model;
    struct source source;
    char oversized[150];
    FILE *fixture;
    char line[256];

    fm_config_defaults(&config);
    fixture = fopen("tests/fm.cfg", "rb");
    assert(fixture);
    while (fgets(line, sizeof(line), fixture))
        fm_config_line(&config, line);
    fclose(fixture);
    assert(config.background[FM_CFG_OBJECT] == 5 && !config.unsupported);
    fm_config_line(&config, "object fg color=2");
    fm_config_line(&config, "OBJECT BG COLOR=12");
    fm_config_line(&config, "OBJECT FG COLOR=65538");
    fm_config_line(&config, " OBJECT FG COLOR=7");
    assert(config.foreground[FM_CFG_OBJECT] == 2);
    fm_model_init(&model);
    assert(!fm_config_apply(&config, &model));
    assert(model.files.panel.style.normal == 0x24);
    assert(model.files.panel.style.selected == 0x4a);
    config.foreground[FM_CFG_OBJECT] = 1;
    config.background[FM_CFG_KEY] = 16;
    assert(fm_config_apply(&config, &model) == FM_BAD_MODEL);
    assert(model.files.panel.style.normal == 0x24);
    config.background[FM_CFG_KEY] = -1;
    assert(fm_config_apply(&config, &model) == FM_BAD_MODEL);
    assert(fm_config_apply(0, &model) == FM_BAD_MODEL);
    assert(fm_config_apply(&config, 0) == FM_BAD_MODEL);
    fm_config_defaults(&config);
    fm_config_line(&config, "MENU POSITION=3");
    assert(config.geometry[FM_CFG_POSITION] == 3);
    fm_config_line(&config, "OVPMODE=7");
    assert(config.unsupported);

    /* Both configuration paths share the bounded decimal parser. */
    fm_config_line(&config, "TOP MARGIN=255suffix");
    assert(config.geometry[FM_CFG_TOP] == 255);
    fm_config_line(&config, "TOP MARGIN=256");
    fm_config_line(&config, "TOP MARGIN=999999999999999999");
    fm_config_line(&config, "TOP MARGIN=-1");
    assert(config.geometry[FM_CFG_TOP] == 255);
    fm_config_line(&config, "OBJECT FG COLOR= 15suffix");
    fm_config_line(&config, "OBJECT FG COLOR=16");
    assert(config.foreground[FM_CFG_OBJECT] == 15);
    fm_config_line(&config, "OBJECT FG COLOR=0000");
    assert(config.foreground[FM_CFG_OBJECT] == 0);

    memset(&fs, 0, sizeof(fs));
    memset(&location, 0, sizeof(location));
    memset(&source, 0, sizeof(source));
    fs.context = &source;
    fs.file_open = open_file;
    fs.file_read = read_file;
    fs.file_close = close_file;
    source.text = "OBJECT FG COLOR=7\r\nCOMMAND FG COLOR=2\nKEY BG COLOR=4\032OBJECT FG COLOR=1";
    fm_config_defaults(&config);
    assert(fm_config_load(&config, &fs, &location) == FM_CFG_LOADED);
    assert(config.foreground[FM_CFG_OBJECT] == 7 && source.closes == 1);
    assert(config.foreground[FM_CFG_COMMAND] == 2 && config.background[FM_CFG_KEY] == 4);
    fm_config_defaults(&config);
    source.fail_read = 1;
    assert(fm_config_load(&config, &fs, &location) == FM_CFG_ERROR);
    assert(!config.foreground[FM_CFG_OBJECT] && source.closes == 2);
    source.fail_read = 0;
    source.fail_close = 1;
    assert(fm_config_load(&config, &fs, &location) == FM_CFG_ERROR);
    assert(!config.foreground[FM_CFG_OBJECT] && source.closes == 3);
    source.fail_close = 0;
    source.missing = 1;
    assert(fm_config_load(&config, &fs, &location) == FM_CFG_ABSENT);
    assert(source.closes == 3);
    source.missing = 0;
    memset(oversized, ' ', sizeof(oversized));
    memcpy(oversized, "OBJECT FG COLOR=7", 17);
    oversized[sizeof(oversized) - 1] = 0;
    source.text = oversized;
    assert(fm_config_load(&config, &fs, &location) == FM_CFG_LOADED);
    assert(!config.foreground[FM_CFG_OBJECT]);
    source.text = "KEY FG COLOR=10";
    assert(fm_config_load(&config, &fs, &location) == FM_CFG_LOADED);
    assert(config.foreground[FM_CFG_KEY] == 10);
    puts("PASS: shipped configuration, case/spacing, colours, intensity, split reads, EOF, bounds and rollback");
    return 0;
}
