/* SPDX-FileCopyrightText: 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#include "applib/app.h"
#include "applib/ui/app_window_stack.h"
#include "applib/ui/window.h"
#include "applib/ui/window_private.h"
#include "applib/ui/text_layer.h"
#include "kernel/pbl_malloc.h"
#include "drivers/barometer.h"
#include "process_state/app_state/app_state.h"
#include "process_management/pebble_process_md.h"
#include "services/common/evented_timer.h"

#include <stdio.h>

#define STATUS_STRING_LEN 200
#define SAMPLE_INTERVAL_MS 100    // Sample every 100ms

static EventedTimerID s_timer;

typedef struct {
    Window window;

    TextLayer title;
    TextLayer status;
    char status_string[STATUS_STRING_LEN];
} AppData;

static void prv_update_display(void *context) {
    AppData *data = context;

    BarData sample;
    BarReadStatus ret = bar_read_data(&sample);

    if (ret != BarReadSuccess) {
        sniprintf(data->status_string, sizeof(data->status_string),
                "BAR ERROR:\n%d", ret);
        text_layer_set_text(&data->status, data->status_string);
        return;
    }

    sniprintf(data->status_string, sizeof(data->status_string),
            "Pressure: %.2f Pa\nTemperature: %.2f °C", sample.pressure, sample.temperature);
    text_layer_set_text(&data->status, data->status_string);
}

static void prv_handle_init(void) {
  AppData *data = app_malloc_check(sizeof(AppData));
  *data = (AppData) {};

  app_state_set_user_data(data);

  // Initialize barometer
  bar_start_sampling();

  Window *window = &data->window;
  window_init(window, "");
  window_set_fullscreen(window, true);

  TextLayer *title = &data->title;
  text_layer_init(title, &window->layer.bounds);
  text_layer_set_font(title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(title, GTextAlignmentCenter);
  text_layer_set_text(title, "BAR TEST");
  layer_add_child(&window->layer, &title->layer);

  TextLayer *status = &data->status;
  text_layer_init(status,
                  &GRect(5, 40,
                         window->layer.bounds.size.w - 5, window->layer.bounds.size.h - 40));
  text_layer_set_font(status, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(status, GTextAlignmentCenter);
  layer_add_child(&window->layer, &status->layer);

  app_window_stack_push(window, true /* Animated */);

  s_timer = evented_timer_register(SAMPLE_INTERVAL_MS, true /* repeating */, prv_update_display, data);
}

static void prv_handle_deinit(void) {
  evented_timer_cancel(s_timer);
  bar_release();
}

static void s_main(void) {
  prv_handle_init();

  app_event_loop();

  prv_handle_deinit();
}

const PebbleProcessMd* mfg_bar_app_get_info(void) {
  static const PebbleProcessMdSystem s_app_info = {
    .common.main_func = &s_main,
    // UUID: 3F4C8A2E-1B6D-4F9E-A3C5-7D8E9F0A1B2D
    .common.uuid = { 0x3F, 0x4C, 0x8A, 0x2E, 0x1B, 0x6D, 0x4F, 0x9E,
                     0xA3, 0xC5, 0x7D, 0x8E, 0x9F, 0x0A, 0x1B, 0x2D,
    },
    .name = "MfgBar",
  };
  return (const PebbleProcessMd*) &s_app_info;
}
