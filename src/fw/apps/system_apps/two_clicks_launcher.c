/* SPDX-License-Identifier: Apache-2.0 */

#include "two_clicks_launcher.h"

#include "applib/app.h"
#include "apps/system_app_ids.h"
#include "kernel/pbl_malloc.h"
#include "process_management/app_install_manager.h"
#include "process_management/app_manager.h"
#include "resource/resource_ids.auto.h"
#include "services/common/i18n/i18n.h"
#include "shell/normal/quick_launch.h"
#include "util/uuid.h"

#include <stdio.h>

// #define TWO_CLICKS_TESTS

static TwoClicksAppData *s_app_data;

static void prv_click_handler(ClickRecognizerRef recognizer, void *context) {
  TwoClicksAppData* data = context;
  AppInstallId app_id = INSTALL_ID_INVALID;
  bool is_enabled = false;
  ButtonId second_button_id = click_recognizer_get_button_id(recognizer);
  
  if (data->args->is_tap) {
    app_id = quick_launch_two_clicks_tap_get_app(data->args->first_button, second_button_id);
    is_enabled = quick_launch_two_clicks_tap_is_enabled(data->args->first_button, second_button_id);
  } else {
    app_id = quick_launch_two_clicks_get_app(data->args->first_button, second_button_id);
    is_enabled = quick_launch_two_clicks_is_enabled(data->args->first_button, second_button_id);
  }

#ifdef TWO_CLICKS_TESTS
  is_enabled = true;
  switch (second_button_id) {
    case BUTTON_ID_UP:
      app_id = APP_ID_SETTINGS;
      break;
    case BUTTON_ID_SELECT:
      app_id = APP_ID_MUSIC;
      break;
    case BUTTON_ID_DOWN:
      app_id = APP_ID_ALARMS;
      break;
    default:
      break;
  }
#endif

  if (!is_enabled) {
    return;
  }

  AppLaunchEventConfig config = (AppLaunchEventConfig) {
    .id = app_id,
    .common.reason = APP_LAUNCH_QUICK_LAUNCH,
    .common.button = second_button_id,
  };
  app_manager_put_launch_app_event(&config);
}

static void prv_set_welcome_text(TextLayer *text_layer, TwoClicksAppData* data) {
  AppInstallId app_up_id;
  AppInstallId app_select_id;
  AppInstallId app_down_id;

  if (data->args->is_tap) {
    app_up_id = quick_launch_two_clicks_tap_get_app(data->args->first_button, BUTTON_ID_UP);
    app_select_id = quick_launch_two_clicks_tap_get_app(data->args->first_button, BUTTON_ID_SELECT);
    app_down_id = quick_launch_two_clicks_tap_get_app(data->args->first_button, BUTTON_ID_DOWN);
  } else {
    app_up_id = quick_launch_two_clicks_get_app(data->args->first_button, BUTTON_ID_UP);
    app_select_id = quick_launch_two_clicks_get_app(data->args->first_button, BUTTON_ID_SELECT);
    app_down_id = quick_launch_two_clicks_get_app(data->args->first_button, BUTTON_ID_DOWN);
  }

#ifdef TWO_CLICKS_TESTS
  app_up_id = APP_ID_SETTINGS;
  app_select_id = APP_ID_MUSIC;
  app_down_id = APP_ID_ALARMS;
#endif

#define SUB_BUFFER_SIZE 20

  static char buffer[100];
  static char name_up[SUB_BUFFER_SIZE] = "Unknown";
  static char name_select[SUB_BUFFER_SIZE] = "Unknown";
  static char name_down[SUB_BUFFER_SIZE] = "Unknown";
  AppInstallEntry entry;
  if (app_install_get_entry_for_install_id(app_up_id, &entry)) {
    memcpy(name_up, entry.name, SUB_BUFFER_SIZE);
  }
  if (app_install_get_entry_for_install_id(app_select_id, &entry)) {
    memcpy(name_select, entry.name, SUB_BUFFER_SIZE);
  }
  if (app_install_get_entry_for_install_id(app_down_id, &entry)) {
    memcpy(name_down, entry.name, SUB_BUFFER_SIZE);
  }
  snprintf(buffer, 100, "Placeholder:\n\nUp: %s\nSelect: %s\nDown: %s", name_up, name_select, name_down);
  text_layer_set_text(text_layer, buffer);
}

static void prv_two_clicks_window_load(Window *window) {
  window_set_background_color(window, GColorWhite);
  TwoClicksAppData *data = window_get_user_data(window);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds_by_value(window_layer);

  // Text layer init
  TextLayer *text_layer = &data->text;
  text_layer_init(text_layer, &bounds);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  prv_set_welcome_text(text_layer, data);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void prv_two_clicks_window_appear(Window *window) {
  TwoClicksAppData *data = window_get_user_data(window);

  // re-enable the inactivity timer back in 2-clicks view
}

static void prv_two_clicks_window_disappear(Window *window) {
  TwoClicksAppData *data = window_get_user_data(window);

  // disable the inactivity timer when the user leaves
}

static void prv_two_clicks_window_unload(Window *window) {
  TwoClicksAppData *data = window_get_user_data(window);

  text_layer_deinit(&data->text);
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_click_handler);
}

static void prv_init() {
  TwoClicksAppData *data = app_malloc_check(sizeof(TwoClicksAppData));
  s_app_data = data;
  *data = (TwoClicksAppData){};

  const TwoClicksArgs *args = process_manager_get_current_process_args();
  data->args = args;

  Window *window = &data->window;
  window_init(window, WINDOW_NAME("2-Clicks"));
  window_set_user_data(window, data);
  window_set_window_handlers(window, &(WindowHandlers) {
    .load = prv_two_clicks_window_load,
    .appear = prv_two_clicks_window_appear,
    .disappear = prv_two_clicks_window_disappear,
    .unload = prv_two_clicks_window_unload
  });
  window_set_click_config_provider_with_context(window, prv_click_config_provider, data);

  app_window_stack_push(window, true /* animated */);
}

static void prv_deinit() {
  app_free(s_app_data);
}

static void prv_main() {
  prv_init();

  app_event_loop();

  prv_deinit();
}

const PebbleProcessMd *two_clicks_launcher_get_app_info() {
  static const PebbleProcessMdSystem s_app_md = {
    .common = {
      .main_func = prv_main,
      .uuid = TWO_CLICKS_LAUNCHER_UUID_INIT,
      .visibility = ProcessVisibilityQuickLaunch,
    },
    .name = i18n_noop("2-Clicks"),
  };
  return &s_app_md.common;
}