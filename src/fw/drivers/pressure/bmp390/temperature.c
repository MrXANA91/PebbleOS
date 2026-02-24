/* SPDX-FileCopyrightText: 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#include "board/board.h"
#include "console/prompt.h"
#include "drivers/temperature.h"
#include "bmp390.h"

#include <stdint.h>
#include <inttypes.h>

// --- Temperature API implementation ---

void temperature_init() {
  bmp390_init();
#ifndef RECOVERY_FW
  bmp390_configure();
#endif
}

int32_t temperature_read() {
  return bmp390_get_temperature();
}

void command_temperature_read(void) {
  char buffer[32];
  prompt_send_response_fmt(buffer, sizeof(buffer), "%" PRId32 " ", temperature_read());
}