/* SPDX-FileCopyrightText: 2025 Core Devices LLC - 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#include "board/board.h"
#include "drivers/i2c.h"
#include "drivers/pressure.h"
#include "drivers/temperature.h"
#include "os/mutex.h"
#include "system/logging.h"
#include "system/passert.h"

#include "bmp390.h"
#include "bmp390_reg.h"

static bmp390_preset_config_data_t s_presets_config[BMP390_PRESET_COUNT] = {
  { BMP390_OVERSAMP_HIGH_RES_X8, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_2, BMP390_SAMP_FREQ_12p5, false, 0},
  { BMP390_OVERSAMP_STANDARD_RES_X4, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_4, BMP390_SAMP_FREQ_50, false, 0},
  { BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_OFF, 0, true, 60},
  { BMP390_OVERSAMP_LOW_POWER_X2, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_OFF, BMP390_SAMP_FREQ_100, false, 0},
  { BMP390_OVERSAMP_ULTRA_HIGH_RES_X16, BMP390_OVERSAMP_LOW_POWER_X2, BMP390_FILTER_COEF_4, BMP390_SAMP_FREQ_25, false, 0},
  { BMP390_OVERSAMP_STANDARD_RES_X4, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_2, BMP390_SAMP_FREQ_50, false, 0},
  { BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_OVERSAMP_ULTRA_LOW_POWER_X1, BMP390_FILTER_COEF_4, BMP390_SAMP_FREQ_1p5, false, 0},
};

// --- Private variables ---

static bool s_initialized = false;
static PebbleMutex *s_bmp390_mutex;
static bmp390_readings_t s_last_reading = {0};
static bmp390_sampling_t s_sampling_mode = BMP390_SAMPLING_DISABLED;
static bmp390_preset_config_data_t *s_config_data = NULL;
static uint16_t s_manual_sampling_period = -1;
static bool s_measurement_ready = false;

// --- Private functions prototypes ---

static bool prv_apply_preset(bmp390_presets_t preset);
static bool prv_read_register(uint8_t register_address, uint8_t data_len, uint8_t *result);
static bool prv_write_register(uint8_t register_address, uint8_t datum);
static bool prv_get_preset_config(bmp390_presets_t preset, bmp390_preset_config_data_t *config);
static bool prv_get_sample(bmp390_readings_t *sample);

// --- Initialisation ---

void bmp390_init() {
  if (s_initialized) {
    return;
  }
  uint8_t result;
  s_bmp390_mutex = mutex_create();
  bool rv = prv_read_register(BMP390_REG_CHIP_ID, 1, &result);
  if (!rv || result != BMP390_CHIP_ID_VALUE) {
    PBL_LOG_DBG("BMP390 probe failed; rv %d, result 0x%02x", rv, result);
  } else {
    PBL_LOG_DBG("found the BMP390, setting to low power");
    (void) prv_write_register(BMP390_REG_PWR_CTRL, BMP390_MODE_SLEEP);
    s_initialized = true;
  }
}

// --- Pressure API implementation ---

void pressure_init() {
  bmp390_init();
}

int32_t pressure_read() {
  int32_t result;
  mutex_lock(s_bmp390_mutex);
  result = s_last_reading.pressure;
  mutex_unlock(s_bmp390_mutex);
  return result;
}

// --- Temperature API implementation ---

void temperature_init() {
  bmp390_init();
}

int32_t temperature_read() {
  int32_t result;
  mutex_lock(s_bmp390_mutex);
  result = s_last_reading.temperature;
  mutex_unlock(s_bmp390_mutex);
  return result;
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
    PBL_LOG_ERR("BMP390: Invalid preset");
    return false;
  }

  config = &(s_presets_config[preset]);
  (void)config; // Bypass -Werror=unused-but-set-parameter
  return true;
}

bool prv_apply_preset(bmp390_presets_t preset) {
  bmp390_preset_config_data_t *config = NULL;
  if (!prv_get_preset_config(preset, config)) {
    return false;
  }

  if (!prv_write_register(BMP390_REG_OSR, config->oversamp_pressure | (config->oversamp_temperature << 3))) {
    PBL_LOG_ERR("BMP390: Failed to set oversampling");
    return false;
  }

  if (!prv_write_register(BMP390_REG_CONFIG, config->iir_filter_coef)) {
    PBL_LOG_ERR("BMP390: Failed to set iir filter coefficient");
    return false;
  }

  if (!prv_write_register(BMP390_REG_ODR, config->sampling_freq_hz)) {
    PBL_LOG_ERR("BMP390: Failed to set sampling frequency");
    return false;
  }

  uint8_t mode = config->forced_mode ? BMP390_MODE_FORCED : BMP390_MODE_NORMAL;
  if (!prv_write_register(BMP390_REG_PWR_CTRL, mode)) {
    PBL_LOG_ERR("BMP390: Failed to set power mode");
    return false;
  }

  if (config->forced_mode) {
    s_manual_sampling_period = config->manual_sampling_period_sec;
    // TODO: setup timer
  }

  s_config_data = config;
  return true;
}

// --- Samples ---

static bool prv_is_data_ready(void) {
  uint8_t status = 0;
  prv_read_register(BMP390_REG_STATUS, 1, &status);
  return (status & (BMP390_STATUS_MASK_PRES_DATA_READY | BMP390_STATUS_MASK_TEMP_DATA_READY)) == (BMP390_STATUS_MASK_PRES_DATA_READY | BMP390_STATUS_MASK_TEMP_DATA_READY);
}

static bool prv_get_sample(bmp390_readings_t *sample) {
  // Check if sensor enabled
  if (s_sampling_mode == BMP390_SAMPLING_DISABLED) {
    return false;
  }

  // Check if data is ready
  if (!s_measurement_ready) {
    if (prv_is_data_ready()) {
      s_measurement_ready = true;
    }
  }
  if (!s_measurement_ready) {
    PBL_LOG_ERR("BMP390: No new measurements");
    return false;
  }
  s_measurement_ready = false;

  // Data ready: read registers
  uint8_t raw_data[6];
  if (!prv_read_register(BMP390_REG_DATA, sizeof(raw_data), raw_data)) {
    PBL_LOG_ERR("BMP390: Unable to read new measurements");
    return false;
  }

  // Refer to the datasheet linked in bmp390_reg.h for more details
  // 16-bit values, with 5 bits of possible oversampling
  uint32_t raw_pressure = ((uint32_t)raw_data[0] << 13) + ((uint32_t)raw_data[1] << 5) + (raw_data[2] >> 3);
  uint32_t raw_temperature = ((uint32_t)raw_data[3] << 13) + ((uint32_t)raw_data[4] << 5) + (raw_data[5] >> 3);

  // Convert
  sample->pressure = (int32_t)(raw_pressure * 8.5);
  sample->temperature = (int32_t)(raw_temperature * 0.15);

  return true;
}