/* Shared UI composition: layout and action metadata, no fixture locations. */
#include <stdio.h>
#include "fm.h"
#include "config.h"

static FM_CONST struct fm_command commands[] = {
    {FM_CMD_HELP, "Help", "Command: Help, Concurrent DOS Help System", FM_TARGET_NONE},
    {FM_CMD_DIRECTORY, "File Directory", "Choose a file directory", FM_TARGET_NONE},
    {FM_CMD_SUBSET, "Subset of Files", "Select a filename pattern", FM_TARGET_NONE},
    {FM_CMD_DRIVE, "Drive Selection", "Choose a drive", FM_TARGET_NONE},
    {FM_CMD_TYPE, "Type   File(s)", "Display selected files", FM_TARGET_MANY},
    {FM_CMD_PRINT, "Print  File(s)", "Print selected files", FM_TARGET_MANY},
    {FM_CMD_COPY, "Copy   File(s)", "Copy selected files", FM_TARGET_MANY},
    {FM_CMD_RENAME, "Rename File(s)", "Rename selected files", FM_TARGET_MANY},
    {FM_CMD_DELETE, "Delete File(s)", "Delete selected files", FM_TARGET_MANY},
    {FM_CMD_BACKUP, "Backup File(s)", "Back up selected files", FM_TARGET_MANY},
    {FM_CMD_EDIT, "Edit a File", "Edit the selected file", FM_TARGET_ONE},
    {FM_CMD_RUN, "Run a Program", "Run the selected program", FM_TARGET_ONE},
    {FM_CMD_COPY_DISK, "Copy   Diskette", "Copy a diskette", FM_TARGET_NONE},
    {FM_CMD_FORMAT_DISK, "Format Diskette", "Format a diskette", FM_TARGET_NONE},
    {FM_CMD_MEMORY, "Free   Memory", "Display available memory", FM_TARGET_NONE},
    {FM_CMD_DETAILS, "Size/Date", "Toggle file size and date", FM_TARGET_NONE},
    {FM_CMD_SETUP, "Set Up System", "Open system setup", FM_TARGET_NONE}};
static const struct fm_command setup_commands[] = {
    {FM_CMD_SYSTEM_COPY, "Copy to Sys Disk",
     "Copy selected files to system disk, user 0/root, SYS and RO", FM_TARGET_MANY},
    {FM_CMD_SYSTEM_ATTR, "System Attributes", "Set selected files SYS and read-only",
     FM_TARGET_MANY},
    {FM_CMD_LOCAL_ATTR, "Local Attributes", "Set selected files DIR and read/write",
     FM_TARGET_MANY},
    {FM_CMD_LOAD_KEYS, "Load Func Keys", "Load function keys from the selected file",
     FM_TARGET_ONE},
    {FM_CMD_ASSIGN_KEYS, "Assign Func Keys", "Assign function keys, then save system parameters",
     FM_TARGET_NONE},
    {FM_CMD_SERIAL, "Serial Ports", "Configure serial ports, then save system parameters",
     FM_TARGET_NONE},
    {FM_CMD_DEFAULTS, "System Defaults", "Configure system defaults", FM_TARGET_NONE},
    {FM_CMD_MEMORY_DISK, "Memory Disk", "Configure memory disk", FM_TARGET_NONE},
    {FM_CMD_BOOT_DISK, "Boot From H D", "Install hard-disk boot support", FM_TARGET_NONE},
    {FM_CMD_HARD_MAINT, "Hard Disk Maint", "Open hard disk maintenance", FM_TARGET_NONE},
    {FM_CMD_FLOPPY_MAINT, "Diskette Maint", "Open diskette maintenance", FM_TARGET_NONE}};

/* Capability policy is shared by menus and dispatch, including Repeat and keys.
 * The composition root supplies features; this module knows no OS versions. */
int fm_command_available(const struct fm_model *model, int command)
{
    if (command == FM_CMD_BACKUP)
        return (model->features & FM_FEATURE_BACKUP) != 0;
    if (command == FM_CMD_ASSIGN_KEYS || command == FM_CMD_SERIAL ||
        command == FM_CMD_DEFAULTS || command == FM_CMD_MEMORY_DISK ||
        command == FM_CMD_BOOT_DISK)
        return (model->features & FM_FEATURE_SETUP_ARGS) != 0;
    return 1;
}

/* Reuse the Actions view; submenu changes never replace the Files model.
 * Model-owned descriptors keep independent instances and filtered menus safe. */
void fm_setup_menu(struct fm_model *model, int enabled)
{
    const struct fm_command *items = enabled ? setup_commands : commands;
    int index, count = enabled ? sizeof(setup_commands) / sizeof(setup_commands[0])
                               : sizeof(commands) / sizeof(commands[0]);

    model->actions.count = 0;
    for (index = 0; index < count; ++index)
        if (fm_command_available(model, items[index].id))
            model->menu_items[model->actions.count++] = items[index];
    model->actions.items = model->menu_items;
    model->actions.cursor.first = model->actions.cursor.selected = 0;
    model->actions.focused = 1;
    model->files.focused = 0;
    model->help.status = enabled ? "System setup. Esc returns to the main menu." : commands[0].help;
}

void fm_features_apply(struct fm_model *model, unsigned features)
{
    model->features = features;
    fm_setup_menu(model, 0);
}

static FM_CONST struct fm_hint hints[] = {
    {0, 1, "Esc="},       {0, 2, "EXIT"},       {11, 1, "F1=HELP"},      {11, 2, "F2=Type"},
    {25, 1, "F3=Repeat"}, {25, 2, "F4=Cancel"}, {39, 1, "F5=Run"},       {39, 2, "F6=Edit"},
    {53, 1, "F7=Dir1"},   {53, 2, "F8=Dir2"},   {66, 1, "F9=Directory"}, {66, 2, "F10=Command"}};

static void panel_init(struct fm_panel *panel, int x, int y, int width, int height, int normal,
                       int selected, int accent)
{
    panel->rect.x = x;
    panel->rect.y = y;
    panel->rect.width = width;
    panel->rect.height = height;
    panel->style.normal = normal;
    panel->style.border = normal;
    panel->style.selected = selected;
    panel->style.marked = accent;
    panel->style.accent = accent;
}

void fm_model_init(struct fm_model *model)
{
    panel_init(&model->device.panel, 0, 0, 60, 4, 48, 3, 63);
    panel_init(&model->files.panel, 0, 3, 60, 18, 48, 3, 63);
    panel_init(&model->actions.panel, 60, 0, 20, 21, 32, 2, 47);
    panel_init(&model->help.panel, 0, 21, 80, 3, 31, 112, 30);
    model->device.media_width = 10;
    model->device.media = "";
    model->device.capacity = "";
    model->device.location = "Select a drive";
    model->files.items = 0;
    model->files.validated_items = 0;
    model->files.validated_count = -1;
    model->files.count = 0;
    model->files.columns = model->files.normal_columns = 4;
    model->files.cursor.selected = FM_NO_SELECTION;
    model->files.cursor.first = 0;
    model->files.focused = 0;
    model->files.details = 0;
    model->files.heading = "Select a command";
    model->files.empty_text = "No files";
    fm_marks_clear(&model->files);
    model->actions.label_command = FM_CMD_DETAILS;
    model->actions.label = "Size/Date ON";
    model->actions.columns = 1;
    model->features = FM_FEATURE_ALL;
    model->actions.items = commands;
    model->actions.count = sizeof(commands) / sizeof(commands[0]);
    model->actions.cursor.selected = 0;
    model->actions.cursor.first = 0;
    model->actions.focused = 1;
    model->actions.empty_text = "No actions";
    model->help.status = commands[0].help;
    model->help.hints = hints;
    model->help.count = sizeof(hints) / sizeof(hints[0]);
    model->generation = 1;
    model->exit_requested = 0;
    panel_init(&model->dialog.panel, 8, 7, 64, 10, 112, 7, 15);
    model->dialog.kind = FM_DIALOG_NONE;
    model->dialog.footer[0] = model->dialog.footer[1] = 0;
    model->dialog.more_before = model->dialog.more_after = 0;
    model->dialog.title = "";
    model->dialog.count = 0;
    model->dialog.text[0] = model->dialog.input[0] = 0;
    model->catalog = 0;
    model->catalog_count = model->files.count;
    model->pending.count = 0;
    model->job_active = model->last_command = 0;
    model->next_job_token = 0;
    model->job.token = model->job.generation = model->job.decision = 0;
    model->job.state = model->job.completed = model->job.failed = model->job.total = 0;
    model->status[0] = 0;
}

/* Shared overlay presentation, with no command-specific policy. */
void fm_dialog_close(struct fm_model *model)
{
    model->dialog.kind = FM_DIALOG_NONE;
    model->dialog.footer[0] = model->dialog.footer[1] = 0;
    model->dialog.more_before = model->dialog.more_after = 0;
    model->dialog.panel.rect.x = 8;
    model->dialog.panel.rect.y = 7;
    model->dialog.panel.rect.width = 64;
    model->dialog.panel.rect.height = 10;
}

void fm_dialog_info(struct fm_model *model, FM_CONST char *title, FM_CONST char *text)
{
    fm_dialog_close(model);
    model->dialog.kind = FM_DIALOG_INFO;
    model->dialog.title = title;
    sprintf(model->dialog.text, "%.79s", text);
}

/* Original colour translation, including bright backgrounds becoming bright
 * foregrounds in reverse video. Blinking is never enabled. */
static int attribute(int foreground, int background)
{
    static int pc[] = {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};
    return pc[foreground] | ((pc[background] & 7) << 4);
}

static void apply_style(struct fm_style *style, int foreground, int background)
{
    style->normal = style->border = attribute(foreground, background);
    style->selected = attribute(background, foreground);
    style->marked = style->accent = attribute(15, background);
}

int fm_config_apply(FM_CONST struct fm_config *config, struct fm_model *model)
{
    int index;

    /* Validate the entire request before changing any view model. */
    if (!config || !model)
        return FM_BAD_MODEL;
    for (index = 0; index < FM_CFG_REGIONS; ++index) {
        if (config->foreground[index] < 0 || config->foreground[index] > 15 ||
            config->background[index] < 0 || config->background[index] > 15)
            return FM_BAD_MODEL;
    }
    apply_style(&model->device.panel.style, config->foreground[FM_CFG_OBJECT],
                config->background[FM_CFG_OBJECT]);
    apply_style(&model->files.panel.style, config->foreground[FM_CFG_OBJECT],
                config->background[FM_CFG_OBJECT]);
    apply_style(&model->actions.panel.style, config->foreground[FM_CFG_COMMAND],
                config->background[FM_CFG_COMMAND]);
    apply_style(&model->help.panel.style, config->foreground[FM_CFG_PROMPT],
                config->background[FM_CFG_PROMPT]);
    model->help.panel.style.accent =
        attribute(config->foreground[FM_CFG_KEY], config->background[FM_CFG_KEY]);
    return FM_SUCCESS;
}

/* Legacy menu positions: right, bottom, left, top, compact bottom.
 * Calculation is local until all four regions fit. The original geometry is
 * retained; horizontal menus use the same scrollable command model as vertical
 * menus. Prompt and key legend remain one view with three or more rows.
 */
int fm_layout_apply(FM_CONST struct fm_config *config, struct fm_model *model)
{
    int position, width, height, menu, x, y, object_x, object_y;
    int object_width, object_height, menu_x, menu_y, menu_width, menu_height;
    int index;

    if (!config || !model)
        return FM_BAD_MODEL;
    for (index = 0; index < FM_CFG_GEOMETRY; ++index)
        if (config->geometry[index] < 0 || config->geometry[index] > 255)
            return FM_BAD_MODEL;
    position = config->geometry[FM_CFG_POSITION];
    x = config->geometry[FM_CFG_LEFT];
    y = config->geometry[FM_CFG_TOP];
    width = FM_COLUMNS - x - config->geometry[FM_CFG_RIGHT];
    height = FM_ROWS - y - config->geometry[FM_CFG_BOTTOM] - config->geometry[FM_CFG_PROMPT_SIZE];
    if (position < 1 || position > 5 || width < 67 || height < 8 ||
        config->geometry[FM_CFG_PROMPT_SIZE] < 1 || FM_ROWS - y - height < 3)
        return FM_BAD_MODEL;
    object_x = menu_x = x;
    object_y = menu_y = y;
    object_width = menu_width = width;
    object_height = menu_height = height;
    if (position == 1 || position == 3) {
        menu = config->geometry[FM_CFG_MENU_SIZE] + 3;
        if (menu < 10 || width - menu < 16)
            return FM_BAD_MODEL;
        object_width -= menu;
        menu_width = menu;
        if (position == 1)
            menu_x += object_width;
        else
            object_x += menu;
    } else {
        menu = position == 5 ? 1 : 3;
        object_height -= menu;
        menu_height = menu;
        if (object_height < 8)
            return FM_BAD_MODEL;
        if (position == 4)
            object_y += menu;
        else
            menu_y += object_height;
    }
    model->device.panel.rect.x = object_x;
    model->device.panel.rect.y = object_y;
    model->device.panel.rect.width = object_width;
    model->files.panel.rect.x = object_x;
    model->files.panel.rect.y = object_y + 3;
    model->files.panel.rect.width = object_width;
    model->files.panel.rect.height = object_height - 3;
    model->files.normal_columns = (object_width - 2) / 14;
    model->files.columns = model->files.details ? 1 : model->files.normal_columns;
    model->actions.panel.rect.x = menu_x;
    model->actions.panel.rect.y = menu_y;
    model->actions.panel.rect.width = menu_width;
    model->actions.panel.rect.height = menu_height;
    model->actions.columns = position == 1 || position == 3 ? 1 : menu_width / 20;
    model->help.panel.rect.x = x;
    model->help.panel.rect.y = y + height;
    model->help.panel.rect.width = width;
    model->help.panel.rect.height = FM_ROWS - y - height;
    return FM_SUCCESS;
}
