/* SPDX-FileCopyrightText: 2026 Paul Chanvin */
/* SPDX-License-Identifier: Apache-2.0 */

#pragma once

// From https://www.mouser.fr/datasheet/3/1046/1/bst_bmp390_ds002.pdf

#define BMP390_REG_CHIP_ID              0x00
#define BMP390_REG_REV_ID               0x01
#define BMP390_REG_ERR_REG              0x02
#define BMP390_REG_STATUS               0x03
#define BMP390_REG_PRESSURE_DATA_0      0x04
#define BMP390_REG_PRESSURE_DATA_1      0x05
#define BMP390_REG_PRESSURE_DATA_2      0x06
#define BMP390_REG_TEMPERATURE_DATA_0   0x07
#define BMP390_REG_TEMPERATURE_DATA_1   0x08
#define BMP390_REG_TEMPERATURE_DATA_2   0x09
#define BMP390_REG_SENSOR_TIME_0        0x0C
#define BMP390_REG_SENSOR_TIME_1        0x0D
#define BMP390_REG_SENSOR_TIME_2        0x0E
#define BMP390_REG_EVENT                0x10
#define BMP390_REG_INT_STATUS           0x11
#define BMP390_REG_FIFO_LENGTH_0        0x12
#define BMP390_REG_FIFO_LENGTH_1        0x13
#define BMP390_REG_FIFO_DATA            0x14
#define BMP390_REG_FIFO_WATERMARK_0     0x15
#define BMP390_REG_FIFO_WATERMARK_1     0x16
#define BMP390_REG_FIFO_CONFIG_1        0x17
#define BMP390_REG_FIFO_CONFIG_2        0x18
#define BMP390_REG_INT_CTRL             0x19
#define BMP390_REG_IF_CONF              0x1A
#define BMP390_REG_PWR_CTRL             0x1B
#define BMP390_REG_OSR                  0x1C
#define BMP390_REG_ODR                  0x1D
#define BMP390_REG_CONFIG               0x1F
#define BMP390_REG_CALIBRATION_DATA     0x31
#define BMP390_REG_CMD                  0x7E

#define BMP390_CHIP_ID_VALUE        0x60
#define BMP390_REV_ID_VALUE         0x01