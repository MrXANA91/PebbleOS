/* SPDX-FileCopyrightText: 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#include "drivers/i2c.h"
#include "os/mutex.h"
#include "system/logging.h"
#include "system/passert.h"

#include "bmp390.h"
#include "bmp390_reg.h"

// --- Static variables ---

static bool s_initialized = false;
static int8_t s_preset_index = -1;
static int s_use_refcount = 0;
static PebbleMutex *s_bar_mutex;

// --- Structures ---

typedef struct {
  uint8_t oversamp_pressure;
  uint8_t oversamp_temperature;
  uint8_t iir_filter_coef;
  uint8_t sampling_freq_hz;
  bool forced_mode;
  uint16_t manual_sampling_period_sec;
} bmp390_preset_config_data_t;
static bmp390_preset_config_data_t s_presets_config[BMP390_PRESET_COUNT] = {
  { BMP390_OVERSAMP_HIGH_RES_X8, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_2, BMP390_SAMP_FREQ_12p5, false, 0},
  { BMP390_OVERSAMP_STANDARD_RES_X4, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_4, BMP390_SAMP_FREQ_50, false, 0},
  { BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_OFF, 0, true, 60},
  { BMP390_OVERSAMP_LOW_POWER_X2, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_OFF, BMP390_SAMP_FREQ_100, false, 0},
  { BMP390_OVERSAMP_ULTRA_HIGH_RES_X16, BMP390_OVERSAMP_LOW_POWER_X2, BMP390_FILTER_COEF_4, BMP390_SAMP_FREQ_25, false, 0},
  { BMP390_OVERSAMP_STANDARD_RES_X4, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_2, BMP390_SAMP_FREQ_50, false, 0},
  { BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_4, BMP390_SAMP_FREQ_1p5, false, 0},
};

// --- Functions prototypes ---

static bool prv_read_register(uint8_t register_address, uint8_t *result);
static bool prv_write_register(uint8_t register_address, uint8_t datum);
static bool prv_get_preset_config(bmp390_presets_t preset, bmp390_preset_config_data_t *config);
static uint8_t prv_set_osr_register(uint8_t pressure, uint8_t temperature);

// --- Initialisation ---

void bmp390_init() {
  uint8_t result;
  s_bar_mutex = mutex_create();
  bool rv = prv_read_register(BMP390_REG_CHIP_ID, 1, &result);
  if (!rv || result != BMP390_CHIP_ID_VALUE) {
    PBL_LOG(LOG_LEVEL_DEBUG, "BMP390 probe failed; rv %d, result 0x%02x", rv, result);
  } else {
    PBL_LOG(LOG_LEVEL_DEBUG, "found the BMP390, setting to low power");
    (void) prv_write_register(BMP390_REG_PWR_CTRL, BMP390_MODE_SLEEP);
    s_initialized = true;
  }
}

// --- Barometer API implementation ---

void bar_use(void) {
  PBL_ASSERTN(s_initialized);
  mutex_lock(s_bar_mutex);
  ++s_use_refcount;
  mutex_unlock(s_bar_mutex);
}

void bar_start_sampling(void) {
  bar_use();
  (void)bar_change_sample_mode(BarSampleLowPower);
}

void bar_release(void) {
  PBL_ASSERTN(s_initialized && s_use_refcount != 0);
  mutex_lock(s_mag_mutex);
  --s_use_refcount;
  if (s_use_refcount == 0) {
    if (!prv_mmc5603nj_set_sample_rate_hz(0)) {
      PBL_LOG(LOG_LEVEL_ERROR, "MMC5603NJ: Failed to disable sensor on release");
    }
  }
  mutex_unlock(s_mag_mutex);
}

BarReadStatus bar_read_data(BarData *data) {
  mutex_lock(s_bar_mutex);
  BarReadStatus ret = BarReadCommunicationFail; // TODO
  mutex_unlock(s_bar_mutex);
  return ret;
}

bool bar_change_sample_mode(BarSampleMode mode) {
  mutex_lock(s_bar_mutex);

  if (s_use_refcount == 0) {
    mutex_unlock(s_bar_mutex);
    return true;
  }

  bmp390_presets_t preset;
  switch (mode) {
    case BarSampleLowPower:
      preset = BMP390_PRESET_HANDHELD_LOWPOWER;
      break;
    case BarSampleDynamic:
      preset = BMP390_PRESET_HANDHELD_DYNAMIC;
      break;
    default:
      mutex_unlock(s_bar_mutex);
      return false;
  }

  bool rv = bmp390_apply_preset(preset);
  mutex_unlock(s_bar_mutex);
  return rv;
}

// --- I2C helper functions ---

static bool prv_read_register(uint8_t register_address, uint8_t data_len, uint8_t *data) {
  i2c_use(I2C_BMP390);
  bool rv = i2c_write_block(I2C_BMP390, 1, &register_address);
  if (rv) {
    rv = i2c_read_block(I2C_BMP390, data_len, data);
  }
  i2c_release(I2C_BMP390);
  return rv;
}

static bool prv_write_register(uint8_t register_address, uint8_t data) {
  i2c_use(I2C_BMP390);
  uint8_t d[2] = { register_address, data };
  bool rv = i2c_write_block(I2C_BMP390, 2, d);
  i2c_release(I2C_BMP390);
  return rv;
}

// --- Configuration ---

static bool prv_get_preset_config(bmp390_presets_t preset, bmp390_preset_config_data_t *config) {
  if (preset >= BMP390_PRESET_COUNT) {
    PBL_LOG(LOG_LEVEL_ERROR, "BMP390: Invalid preset");
    return false;
  }

  config = &(s_presets_config[preset]);
  return true;
}

bool bmp390_apply_preset(bmp390_presets_t preset) {
  bmp390_preset_config_data_t config;
  if (!prv_get_preset_config(preset, &config)) {
    return false;
  }

  if (!prv_write_register(BMP390_REG_OSR, config.oversamp_pressure | (config.oversamp_temperature << 3))) {
    PBL_LOG(LOG_LEVEL_ERROR, "BMP390: Failed to set oversampling");
    return false;
  }

  if (!prv_write_register(BMP390_REG_CONFIG, config.iir_filter_coef)) {
    PBL_LOG(LOG_LEVEL_ERROR, "BMP390: Failed to set iir filter coefficient");
    return false;
  }

  if (!prv_write_register(BMP390_REG_ODR, config.sampling_freq_hz)) {
    PBL_LOG(LOG_LEVEL_ERROR, "BMP390: Failed to set sampling frequency");
    return false;
  }

  return true;
}

// --- Samples ---

static bool prv_is_data_ready(void) {
  uint8_t status = 0;
  prv_read_register(BMP390_REG_STATUS, 1, &status);
  return (status & (BMP390_STATUS_MASK_PRES_DATA_READY | BMP390_STATUS_MASK_TEMP_DATA_READY)) != 0;
}

static BarReadStatus prv_get_sample(BarData *sample) {
  // Check if sensor enabled

  // Check if data is ready

  // Data ready: read registers

  // Convert (?)

  return BarReadSuccess;
}