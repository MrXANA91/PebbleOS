/* SPDX-FileCopyrightText: 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

// --- Structures and enums ---

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

typedef enum {
  BMP390_SAMPLING_DISABLED,     // Sampling disabled
  BMP390_SAMPLING_SLOW,         // one sample per minute --> BMP390_PRESET_WEATHER_MONITOR
  BMP390_SAMPLING_FAST,         // 12 samples per seconds --> BMP390_PRESET_HANDHELD_LOWPOWER
  BMP390_SAMPLING_FASTER,       // 50 samples per seconds --> BMP390_PRESET_HANDHELD_DYNAMIC
  BMP390_SAMPLING_COUNT
} bmp390_sampling_t;

typedef struct {
  uint8_t oversamp_pressure;
  uint8_t oversamp_temperature;
  uint8_t iir_filter_coef;
  uint8_t sampling_freq_hz;
  bool forced_mode;
  uint16_t sampling_period_ms;
} bmp390_preset_config_data_t;

typedef struct {
  int32_t pressure;     //< in mPa
  int32_t temperature;  //< in m°C
} bmp390_readings_t;

// --- Functions prototypes ---

void bmp390_init(void);
void bmp390_configure(void);

int32_t bmp390_get_temperature(void);
int32_t bmp390_get_pressure(void);
bmp390_sampling_t bmp390_get_sampling_mode(void);