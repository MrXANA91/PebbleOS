/* SPDX-FileCopyrightText: 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/attributes.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct PACKED {
    float pressure;
    float temperature;
} BarData;

typedef enum {
    BarReadSuccess = 0,
    BarReadClobbered = -1,
    BarReadCommunicationFail = -2,
    BarReadBarOff = -3,
    BarReadNoBar = -4,
} BarReadStatus;

typedef enum {
    BarSampleDisabled,
    BarSampleLowPower,
    BarSampleDynamic,
} BarSampleMode;

void bar_use(void);

void bar_start_sampling(void);

void bar_release(void);

BarReadStatus bar_read_data(BarData *data);

bool bar_change_sample_mode(BarSampleMode mode);