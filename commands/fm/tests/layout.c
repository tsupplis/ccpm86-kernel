/* Layout and controller integration: all five placements, atomic rejection,
 * passive rendering and command reachability, without a native adapter. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
static int execute(void *context, struct fm_model *model, int command)
{
    *(int *)context = command;
    return FM_SUCCESS;
}
int main(void)
{
    static struct fm_model model, before;
    static struct fm_canvas canvas;
    struct fm_display display;
    struct fm_action_handler handler;
    struct fm_config config;
    int position, command;
    fm_canvas_bind(&display, &canvas);
    handler.context = &command;
    handler.execute = execute;
    handler.accept = 0;
    for (position = 1; position <= 5; ++position) {
        fm_model_init(&model);
        fm_config_defaults(&config);
        config.geometry[FM_CFG_POSITION] = position;
        assert(!fm_layout_apply(&config, &model));
        assert(model.help.panel.rect.y == 21);
        assert(model.help.panel.rect.height == 3);
        if (position == 1 || position == 3) {
            assert(model.files.panel.rect.width == 60);
            assert(model.actions.panel.rect.width == 20);
            assert(model.actions.panel.rect.x == (position == 1 ? 60 : 0));
            assert(model.files.panel.rect.x == (position == 1 ? 0 : 20));
        } else {
            assert(model.actions.panel.rect.width == 80);
            assert(model.actions.columns == 4);
            assert(model.actions.panel.rect.height == (position == 5 ? 1 : 3));
            assert(model.actions.panel.rect.y == (position == 4 ? 0 : position == 5 ? 20 : 18));
        }
        before = model;
        assert(!fm_render(&model, &display));
        assert(!memcmp(&before, &model, sizeof(model)));
        assert(!fm_dispatch_to(&model, 0, &handler, FM_KEY_END));
        assert(model.actions.cursor.selected == model.actions.count - 1);
        assert(!fm_render(&model, &display));
        command = 0;
        assert(!fm_dispatch_to(&model, 0, &handler, FM_KEY_ENTER));
        assert(command == FM_CMD_SETUP);
        assert(!fm_dispatch_to(&model, 0, &handler, FM_KEY_HOME));
        assert(model.actions.cursor.selected == 0);
    }
    fm_model_init(&model);
    fm_config_defaults(&config);
    before = model;
    config.geometry[FM_CFG_MENU_SIZE] = 255;
    assert(fm_layout_apply(&config, &model) == FM_BAD_MODEL);
    assert(!memcmp(&before, &model, sizeof(model)));
    fm_config_defaults(&config);
    config.geometry[FM_CFG_TOP] = 255;
    assert(fm_layout_apply(&config, &model) == FM_BAD_MODEL);
    assert(!memcmp(&before, &model, sizeof(model)));
    fm_config_defaults(&config);
    config.geometry[FM_CFG_LEFT] = 2;
    config.geometry[FM_CFG_TOP] = 1;
    assert(!fm_layout_apply(&config, &model));
    assert(!fm_render(&model, &display));
    assert(model.device.panel.rect.x == 2 && model.device.panel.rect.y == 1);
    puts("PASS: five layouts, bounded margins, passive rendering, atomic rejection and menu reachability");
    return 0;
}
