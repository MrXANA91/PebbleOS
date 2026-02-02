/* SPDX-FileCopyrightText: 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/attributes.h"

#include <stdint.h>

typedef enum {
    BMP390_PRESET_HANDHELD_LOWPOWER,
    BMP390_PRESET_HANDHELD_DYNAMIC,
    BMP390_PRESET_WEATHER_MONITOR,
    BMP390_PRESET_DROP_DETECTION,
    BMP390_PRESET_INDOOR_NAVIGATION,
    BMP390_PRESET_DRONE,
    BMP390_PRESET_INDOOR_LOCALIZATION,
    BMP390_PRESET_COUNT,
} bmp390_presets_t;

void bmp390_init(void);

// --- PLaceholder for barometer API ---

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