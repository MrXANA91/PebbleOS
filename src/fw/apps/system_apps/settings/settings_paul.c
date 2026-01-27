/* SPDX-License-Identifier: Apache-2.0 */

#include "settings_paul.h"
#include "settings_menu.h"
#include "settings_option_menu.h"
#include "settings_window.h"

#include "kernel/pbl_malloc.h"
#include "services/common/i18n/i18n.h"
#include "shell/prefs.h"
#include "system/passert.h"

#include <string.h>

typedef struct SettingsPaulData {
    SettingsCallbacks callbacks;
} SettingsPaulData;

static const char *s_menu_vibes_labels[] = {
    i18n_noop("Off"),
    i18n_noop("On wrap around"),
    i18n_noop("On blocked"),
};

enum SettingsPaulItem {
    SettingsPaulMenuWrap,
    SettingsPaulMenuVibes,
    NumSettingsHealthItems
};

static void prv_deinit_cb(SettingsCallbacks *context) {
    SettingsPaulData *data = (SettingsPaulData*)context;

    i18n_free_all(data);
    app_free(data);
}

static void prv_draw_row_cb(SettingsCallbacks *context, GContext *ctx,
                            const Layer *cell_layer, uint16_t row, bool selected) {
    SettingsPaulData *data = (SettingsPaulData*) context;

    const char *title = NULL;
    const char *subtitle = NULL;

    switch (row) {
        case SettingsPaulMenuWrap:
            title = i18n_noop("Menu Wrap");
            subtitle = shell_prefs_get_menu_scroll_wrap_around_enable() ? i18n_noop("On") : i18n_noop("Off");
            break;
        case SettingsPaulMenuVibes:
            title = i18n_noop("Menu Vibes");
            MenuScrollVibeBehavior behavior = shell_prefs_get_menu_scroll_vibe_behavior();
            if (behavior >= MenuScrollVibeBehaviorsCount) {
                subtitle = i18n_noop("Unknown");
            } else {
                subtitle = s_menu_vibes_labels[behavior];
            }
            break;
        default:
            WTF;
    }
    menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static void prv_select_click_cb(SettingsCallbacks *context, uint16_t row) {
    SettingsPaulData *data = (SettingsPaulData*)context;
    switch (row) {
        case SettingsPaulMenuWrap:
            shell_prefs_set_menu_scroll_wrap_around_enable(!shell_prefs_get_menu_scroll_wrap_around_enable());
            break;
        case SettingsPaulMenuVibes:
            MenuScrollVibeBehavior behavior = shell_prefs_get_menu_scroll_vibe_behavior();
            behavior = (behavior + 1) % MenuScrollVibeBehaviorsCount;
            shell_prefs_set_menu_scroll_vibe_behavior(behavior);
            break;
        default:
            WTF;
    }
    settings_menu_reload_data(SettingsMenuItemPaul);
    settings_menu_mark_dirty(SettingsMenuItemPaul);
}

static uint16_t prv_num_rows_cb(SettingsCallbacks *context) {
    uint16_t rows = NumSettingsHealthItems;

    return rows;
}

static void prv_appear_cb(SettingsCallbacks *context) {
    SettingsPaulData *data = (SettingsPaulData*)context;
}

static void prv_hide_cb(SettingsCallbacks *context) {
    SettingsPaulData *data = (SettingsPaulData*)context;
}

static Window *prv_init(void) {
    SettingsPaulData *data = app_malloc_check(sizeof(*data));
    *data = (SettingsPaulData){};

    data->callbacks = (SettingsCallbacks) {
        .deinit = prv_deinit_cb,
        .draw_row = prv_draw_row_cb,
        .select_click = prv_select_click_cb,
        .num_rows = prv_num_rows_cb,
        .appear = prv_appear_cb,
        .hide = prv_hide_cb,
    };

    return settings_window_create(SettingsMenuItemPaul, &data->callbacks);
}

const SettingsModuleMetadata *settings_paul_get_info(void) {
    static const SettingsModuleMetadata s_module_info = {
        .name = i18n_noop("Paul"),
        .init = prv_init,
    };

    return &s_module_info;
}