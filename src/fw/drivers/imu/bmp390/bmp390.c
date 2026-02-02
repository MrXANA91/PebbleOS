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
static int s_use_refcount = 0;
static PebbleMutex *s_bar_mutex;
static BarSampleMode s_sample_mode = BarSampleDisabled;
static bmp390_preset_config_data_t s_config_data = {0};
static bool s_measurement_ready = false;

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

static bool prv_apply_preset(bmp390_presets_t preset);
static bool prv_read_register(uint8_t register_address, uint8_t *result);
static bool prv_write_register(uint8_t register_address, uint8_t datum);
static bool prv_get_preset_config(bmp390_presets_t preset, bmp390_preset_config_data_t *config);

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
  mutex_lock(s_bar_mutex);
  --s_use_refcount;
  if (s_use_refcount == 0) {
    if (!bar_change_sample_mode(BarSampleDisabled)) {
      PBL_LOG(LOG_LEVEL_ERROR, "BMP390: Failed to disable sensor on release");
    }
  }
  mutex_unlock(s_bar_mutex);
}

BarReadStatus bar_read_data(BarData *data) {
  mutex_lock(s_bar_mutex);
  BarReadStatus ret = prv_get_sample(data);
  mutex_unlock(s_bar_mutex);
  return ret;
}

bool bar_change_sample_mode(BarSampleMode mode) {
  mutex_lock(s_bar_mutex);

  if (s_use_refcount == 0) {
    mutex_unlock(s_bar_mutex);
    return true;
  }

  // Reset device runtime
  if (!prv_write_register(BMP390_REG_CMD, BMP390_CMD_SOFT_RESET)) {
    PBL_LOG(LOG_LEVEL_ERROR, "BMP390: Failed to write command");
    return false;
  }

  bmp390_presets_t preset;
  switch (mode) {
    case BarSampleDisabled:
      s_sample_mode = mode;
      mutex_unlock(s_bar_mutex);
      return true;
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

  bool rv = prv_apply_preset(preset);
  if (rv) {
    s_sample_mode = mode;

    // Configure polling ?
  }
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

bool prv_apply_preset(bmp390_presets_t preset) {
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
  return (status & (BMP390_STATUS_MASK_PRES_DATA_READY | BMP390_STATUS_MASK_TEMP_DATA_READY)) == (BMP390_STATUS_MASK_PRES_DATA_READY | BMP390_STATUS_MASK_TEMP_DATA_READY);
}

static BarReadStatus prv_get_sample(BarData *sample) {
  // Check if sensor enabled
  if (s_sample_mode == BarSampleDisabled) {
    return BarReadBarOff;
  }

  // Check if data is ready
  if (!s_measurement_ready) {
    if (prv_is_data_ready()) {
      s_measurement_ready = true;
    }
  }
  if (!s_measurement_ready) {
    PBL_LOG(LOG_LEVEL_ERROR, "BMP390: No new measurements");
    return BarReadCommunicationFail;
  }
  s_measurement_ready = false;

  // Data ready: read registers
  uint8_t raw_data[6];
  if (!prv_read_register(BMP390_REG_DATA, sizeof(raw_data), raw_data)) {
    PBL_LOG(LOG_LEVEL_ERROR, "BMP390: Unable to read new measurements");
    return BarReadCommunicationFail;
  }

  // 16-bit values, with 5 bits of possible oversampling
  uint32_t raw_pressure = ((uint32_t)raw_data[0] << 13) + ((uint32_t)raw_data[1] << 5) + (raw_data[2] >> 3);
  uint32_t raw_temperature = ((uint32_t)raw_data[3] << 13) + ((uint32_t)raw_data[4] << 5) + (raw_data[5] >> 3);

  // Convert
  sample->pressure = raw_pressure * 0.085;
  sample->temperature = raw_temperature * 0.00015;

  return BarReadSuccess;
}