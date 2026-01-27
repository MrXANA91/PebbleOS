/* SPDX-FileCopyrightText: 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#include "drivers/i2c.h"
#include "system/logging.h"

#include "bmp390.h"
#include "bmp390_reg.h"

static bool prv_read_register(uint8_t register_address, uint8_t *result) {
  i2c_use(I2C_BMP390);
  bool rv = i2c_write_block(I2C_BMP390, 1, &register_address);
  if (rv)
    rv = i2c_read_block(I2C_BMP390, 1, result);
  i2c_release(I2C_BMP390);
  return rv;
}

static bool prv_write_register(uint8_t register_address, uint8_t datum) {
  i2c_use(I2C_BMP390);
  uint8_t d[2] = { register_address, datum };
  bool rv = i2c_write_block(I2C_BMP390, 2, d);
  i2c_release(I2C_BMP390);
  return rv;
}

void bmp390_init() {
  bool rv;
  uint8_t result;

  rv = prv_read_register(BMP390_REG_CHIP_ID, &result);
  if (!rv || result != BMP390_CHIP_ID_VALUE) {
    PBL_LOG(LOG_LEVEL_DEBUG, "BMP390 probe failed; rv %d, result 0x%02x", rv, result);
  } else {
    PBL_LOG(LOG_LEVEL_DEBUG, "found the BMP390, setting to low power");
    (void) prv_write_register(BMP390_REG_PWR_CTRL, 0);
  }
}