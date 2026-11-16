/* Production composition root. Owns native services and their lifetimes. */
#include <stdio.h>
#include <stdlib.h>
#include "fm.h"
#include "browser.h"
#include "fsnative.h"
#include "config.h"
#include "runos.h"

int BDOS(int function, ...);

static void *allocate_catalog(void *context, unsigned bytes)
{
    return malloc(bytes);
}
static void release_catalog(void *context, void *memory)
{
    free(memory);
}

int main(int argc, char **argv)
{
    static struct fm_model model;
    static struct fm_canvas canvas;
    struct fm_display display;
    struct fm_invoker invoker;
    int key, status, repaint, copy_tick;
    char progress_text[80];
    static struct fm_browser browser;
    static struct fm_native_fs filesystem_state;
    static struct fm_filesystem filesystem;
    static struct fm_native_copy copy_state;
    static struct fm_copy_store copy_store;
    static struct fm_mutation_store mutation_store;
    static struct fm_native_mutation mutation_state;
    static char default_dma[128];
    static struct fm_executor executor;
    static struct fm_printer printer;
    static struct fm_backup_host backup_host;
    static struct fm_native_backup backup_state;
    static struct fm_native_run run_state;
    struct fm_config config;
    struct fm_location config_location;
    struct fm_memory memory;
    int config_status;

    fm_model_init(&model);
    fm_canvas_bind(&display, &canvas);
    if (fm_native_open()) {
        puts("FM requires a Concurrent 3.1 or 4.1 PC console.");
        return 1;
    }
    fm_features_apply(&model, (BDOS(12, 0) & 65535) == 0x1441 ? FM_FEATURE_ALL : 0);
    {
        BDOS(26, default_dma);
        fm_native_fs_bind(&filesystem, &filesystem_state, default_dma);
        fm_config_defaults(&config);
        config_location.drive = BDOS(25, 0) & 255;
        config_location.user = BDOS(32, 255) & 255;
        strcpy(config_location.path, "\\");
        config_status = FM_CFG_ABSENT;
        if (!(*filesystem.probe)(filesystem.context, config_location.drive, &config_location.media))
            config_status = fm_config_load(&config, &filesystem, &config_location);
        if (config_status == FM_CFG_ABSENT) {
            config_location.drive = fm_native_system_drive();
            config_location.user = 0;
            if (!(*filesystem.probe)(filesystem.context, config_location.drive,
                                     &config_location.media))
                config_status = fm_config_load(&config, &filesystem, &config_location);
        }
        if (fm_config_apply(&config, &model)) {
            fm_native_close();
            puts("Invalid FM colour configuration.");
            return 1;
        }
        if (fm_layout_apply(&config, &model))
            config.unsupported = 1; /* Invalid geometry retains the default layout. */
        memory.context = 0;
        memory.allocate = allocate_catalog;
        memory.release = release_catalog;
        if (fm_browser_open(&browser, &model, &filesystem, &memory, BDOS(25, 0) & 255,
                            BDOS(32, 255) & 255)) {
            fm_native_close();
            puts("Cannot initialize browser.");
            return 1;
        }
        fm_native_copy_bind(&copy_store, &copy_state, &filesystem_state, default_dma);
        fm_browser_copy_bind(&browser, &copy_store);
        fm_native_mutation_bind(&mutation_store, &mutation_state, &copy_state);
        fm_browser_mutation_bind(&browser, &mutation_store);
        fm_native_run_bind(&executor, &run_state, &filesystem_state);
        fm_browser_run_bind(&browser, &executor);
        browser.system_drive = fm_native_system_drive();
        fm_native_printer_bind(&printer);
        fm_browser_print_bind(&browser, &printer);
        if (model.features & FM_FEATURE_BACKUP) {
            fm_native_backup_bind(&backup_host, &backup_state, &filesystem_state, &executor);
            fm_browser_backup_bind(&browser, &backup_host);
        }
        if (config_status == FM_CFG_ERROR)
            fm_dialog_info(&model, "FM.CFG", "Configuration read failed; defaults retained.");
        else if (config.unsupported)
            fm_dialog_info(
                &model, "FM.CFG",
                "Colours loaded. Unsupported OVPMODE or invalid layout; defaults retained.");
    }
    status = FM_SUCCESS;
    repaint = 1;
    while (!model.exit_requested) {
        if (repaint) {
            status = fm_render(&model, &display);
            if (status)
                break;
            status = fm_native_present(&canvas);
            if (status)
                break;
        }
        /* Idle ticks yield in the input adapter, but need no new frame.
         * Only a running job consumes ticks; a decision waits for an answer. */
        do {
            key = fm_native_read();
        } while (key == FM_KEY_TICK && (!model.job_active || (model.job.state == FM_JOB_DECISION ||
                                                              model.job.state == FM_JOB_VIEW ||
                                                              model.job.state == FM_JOB_INPUT)));
        if (key == '\n')
            key = FM_KEY_ENTER;
        copy_tick = key == FM_KEY_TICK && model.job_active &&
                    browser.active_service == &browser.copy_service &&
                    model.dialog.kind == FM_DIALOG_PROGRESS;
        if (copy_tick)
            strcpy(progress_text, model.dialog.text);
        status = fm_browser_key(&browser, &model, &invoker, key);
        /* Copy's progress overlay is its only visible change during a tick.
         * Decisions, completion, errors and keyboard input always repaint. */
        repaint = !copy_tick || status || !model.job_active ||
                  model.dialog.kind != FM_DIALOG_PROGRESS ||
                  strcmp(progress_text, model.dialog.text) != 0;
        if (status == FM_JOB_ERROR)
            status = FM_SUCCESS; /* Controller has prepared an error overlay. */
        if (status)
            break;
    }
    fm_browser_close(&browser);
    fm_native_close();
    if (status)
        printf("FM framework error: %d\n", status);
    return status;
}
