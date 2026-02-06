/* SPDX-License-Identifier: Apache-2.0 */

#include "applib/ui/ui.h"

typedef struct {
  ButtonId first_button;
  bool is_tap;
} TwoClicksArgs;

typedef struct {
  Window window;

  TextLayer text;

  const TwoClicksArgs* args;
} TwoClicksAppData;

// uuid: c9594fce-2c48-47fb-a2f2-8aaa04e5daf0
#define TWO_CLICKS_LAUNCHER_UUID_INIT {0xc9, 0x59, 0x4f, 0xce, 0x2c, 0x48, 0x47, 0xfb, \
                                       0xa2, 0xf2, 0x8a, 0xaa, 0x04, 0xe5, 0xda, 0xf0}

const PebbleProcessMd *two_clicks_launcher_get_app_info();