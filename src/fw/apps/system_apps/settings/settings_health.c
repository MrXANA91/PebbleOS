/* SPDX-License-Identifier: Apache-2.0 */

#include "settings_health.h"
#include "settings_menu.h"
#include "settings_option_menu.h"
#include "settings_window.h"

#include "applib/ui/dialogs/dialog.h"
#include "applib/ui/dialogs/simple_dialog.h"
#include "kernel/pbl_malloc.h"
#include "resource/resource_ids.auto.h"
#include "services/common/i18n/i18n.h"
#include "services/normal/menu_preferences.h"
#include "shell/prefs.h"
#include "system/passert.h"

#include <string.h>
#include <stdio.h>

typedef struct SettingsHealthData {
    SettingsCallbacks callbacks;
} SettingsHealthData;

static const char *s_units_distance_labels[] = {
    i18n_noop("Kilometers"),
    i18n_noop("Miles"),
};

enum SettingsHealthItem {
    SettingsHealthUnitDistance,
    SettingsHealthMenuPrefScrollWrapAround,
    SettingsHealthMenuPrefScrollVibeOnWrap,
    SettingsHealthMenuPrefScrollVibeOnBlocked,
    SettingsHealthMenuPrefClean,
    NumSettingsHealthItems
};

static void prv_push_confirmation_dialog(uint8_t status) {
    SimpleDialog* dialog = simple_dialog_create("MenuPrefCleanUpConfirm");

    if (status == 0) {
        dialog_set_text(&(dialog->dialog), "Success");
        dialog_set_icon(&(dialog->dialog), RESOURCE_ID_GENERIC_CONFIRMATION_LARGE);
    } else {
        char buffer[13];
        snprintf(buffer, 13, "0x%x", status);
        dialog_set_text(&(dialog->dialog), (const char *)buffer);
        dialog_set_icon(&(dialog->dialog), RESOURCE_ID_HEAVY_RAIN_LARGE);
    }
    dialog_set_destroy_on_pop(&(dialog->dialog), true);
    dialog_set_timeout(&(dialog->dialog), DIALOG_TIMEOUT_DEFAULT);
    simple_dialog_set_buttons_enabled(dialog, false);

    app_simple_dialog_push(dialog);
}

static void prv_deinit_cb(SettingsCallbacks *context) {
    SettingsHealthData *data = (SettingsHealthData*)context;

    i18n_free_all(data);
    app_free(data);
}

static void prv_draw_row_cb(SettingsCallbacks *context, GContext *ctx,
                            const Layer *cell_layer, uint16_t row, bool selected) {
    SettingsHealthData *data = (SettingsHealthData*) context;

    const char *title = NULL;
    const char *subtitle = NULL;

    switch (row) {
        case SettingsHealthUnitDistance:
            title = i18n_noop("Distance Unit");
            UnitsDistance unit = shell_prefs_get_units_distance();
            if (unit >= UnitsDistanceCount) {
                subtitle = i18n_noop("Unknown");
            } else {
                subtitle = s_units_distance_labels[unit];
            }
            break;
        case SettingsHealthMenuPrefScrollWrapAround:
            title = i18n_noop("ScrollWrapAround");
            if (menu_preferences_get_scroll_wrap_around()) {
                subtitle = i18n_noop("On");
            } else {
                subtitle = i18n_noop("Off");
            }
            break;
        case SettingsHealthMenuPrefScrollVibeOnWrap:
            title = i18n_noop("ScrollVibeOnWrap");
            if (menu_preferences_get_scroll_vibe_on_wrap_around()) {
                subtitle = i18n_noop("On");
            } else {
                subtitle = i18n_noop("Off");
            }
            break;
        case SettingsHealthMenuPrefScrollVibeOnBlocked:
            title = i18n_noop("ScrollVibeOnBlocked");
            if (menu_preferences_get_scroll_vibe_on_blocked()) {
                subtitle = i18n_noop("On");
            } else {
                subtitle = i18n_noop("Off");
            }
            break;
        case SettingsHealthMenuPrefClean:
            title = i18n_noop("MenuPrefClean");
            break;
        default:
            WTF;
    }
    menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static void prv_select_click_cb(SettingsCallbacks *context, uint16_t row) {
    SettingsHealthData *data = (SettingsHealthData*)context;
    switch (row) {
        case SettingsHealthUnitDistance:
            UnitsDistance unit = shell_prefs_get_units_distance();
            unit = (unit + 1) % UnitsDistanceCount;
            shell_prefs_set_units_distance(unit);
            break;
        case SettingsHealthMenuPrefScrollWrapAround:
            menu_preferences_set_scroll_wrap_around(!menu_preferences_get_scroll_wrap_around());
            break;
        case SettingsHealthMenuPrefScrollVibeOnWrap:
            menu_preferences_set_scroll_vibe_on_wrap_around(!menu_preferences_get_scroll_vibe_on_wrap_around());
            break;
        case SettingsHealthMenuPrefScrollVibeOnBlocked:
            menu_preferences_set_scroll_vibe_on_blocked(!menu_preferences_get_scroll_vibe_on_blocked());
            break;
        case SettingsHealthMenuPrefClean:
            prv_push_confirmation_dialog(menu_preferences_system_file_clean_up());
            break;
        default:
            WTF;
    }
    settings_menu_reload_data(SettingsMenuItemHealth);
    settings_menu_mark_dirty(SettingsMenuItemHealth);
}

static uint16_t prv_num_rows_cb(SettingsCallbacks *context) {
    uint16_t rows = NumSettingsHealthItems;

    return rows;
}

static void prv_appear_cb(SettingsCallbacks *context) {
    SettingsHealthData *data = (SettingsHealthData*)context;
}

static void prv_hide_cb(SettingsCallbacks *context) {
    SettingsHealthData *data = (SettingsHealthData*)context;
}

static Window *prv_init(void) {
    SettingsHealthData *data = app_malloc_check(sizeof(*data));
    *data = (SettingsHealthData){};

    data->callbacks = (SettingsCallbacks) {
        .deinit = prv_deinit_cb,
        .draw_row = prv_draw_row_cb,
        .select_click = prv_select_click_cb,
        .num_rows = prv_num_rows_cb,
        .appear = prv_appear_cb,
        .hide = prv_hide_cb,
    };

    return settings_window_create(SettingsMenuItemHealth, &data->callbacks);
}

const SettingsModuleMetadata *settings_health_get_info(void) {
    static const SettingsModuleMetadata s_module_info = {
        .name = i18n_noop("Health"),
        .init = prv_init,
    };

    return &s_module_info;
}