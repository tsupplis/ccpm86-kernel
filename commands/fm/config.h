/* FM.CFG colour configuration. Caller owns settings; no renderer or OS state.
 * Historical colour numbers use red=2, green=4 (not PC attribute numbering).
 * Loading uses the injected read-only filesystem and publishes only after a
 * successful close. Missing files leave defaults intact. UI policy validates
 * geometry atomically; non-default OVPMODE is reported as unsupported.
 */
#ifndef FM_CONFIG_H
#define FM_CONFIG_H

#include "location.h"

/* Colour regions. */
#define FM_CFG_OBJECT 0
#define FM_CFG_COMMAND 1
#define FM_CFG_PROMPT 2
#define FM_CFG_KEY 3
#define FM_CFG_REGIONS 4

/* Historical geometry fields. */
#define FM_CFG_OVPMODE 0
#define FM_CFG_POSITION 1
#define FM_CFG_MENU_SIZE 2
#define FM_CFG_PROMPT_SIZE 3
#define FM_CFG_TOP 4
#define FM_CFG_BOTTOM 5
#define FM_CFG_LEFT 6
#define FM_CFG_RIGHT 7
#define FM_CFG_GEOMETRY 8

/* Configuration load results. */
#define FM_CFG_ABSENT 0
#define FM_CFG_LOADED 1
#define FM_CFG_ERROR 2

struct fm_config {
    int foreground[FM_CFG_REGIONS];
    int background[FM_CFG_REGIONS];
    int geometry[FM_CFG_GEOMETRY];
    int unsupported;
};

void fm_config_defaults(struct fm_config *config);
void fm_config_line(struct fm_config *config, const char *line);

int fm_config_load(struct fm_config *config, struct fm_filesystem *filesystem,
                   const struct fm_location *location);

/* UI policy implemented in ui.c, not by the configuration I/O service.
 * Return FM_BAD_MODEL without mutation for invalid colours/null arguments;
 * otherwise return FM_SUCCESS. Selection and layout are unchanged. */
int fm_config_apply(const struct fm_config *config, struct fm_model *model);

/* Apply bounded legacy geometry to the four views. Invalid layouts leave all
 * models unchanged. Called after model initialization, before opening a catalog. */
int fm_layout_apply(const struct fm_config *config, struct fm_model *model);

#endif
