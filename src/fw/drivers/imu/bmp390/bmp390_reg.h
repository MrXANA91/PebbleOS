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

// Status masks - for BMP390_REG_STATUS
#define BMP390_STATUS_MASK_CMD_READY            (1 << 4)
#define BMP390_STATUS_MASK_PRES_DATA_READY      (1 << 5)
#define BMP390_STATUS_MASK_TEMP_DATA_READY      (1 << 6)

// Modes - for BMP390_REG_PWR_CTRL
#define BMP390_MODE_SLEEP   0x00
#define BMP390_MODE_FORCED  0x10 // or 0x01
#define BMP390_MODE_NORMAL  0x11

// Oversampling - for BMP390_REG_OSR 
#define BMP390_OVERSAMP_ULTRA_LOW_POWER_X1      0x00
#define BMP390_OVERSAMP_LOW_POWER_X2            0x01
#define BMP390_OVERSAMP_STANDARD_RES_X4         0x02
#define BMP390_OVERSAMP_HIGH_RES_X8             0x03
#define BMP390_OVERSAMP_ULTRA_HIGH_RES_X16      0x04
#define BMP390_OVERSAMP_HIGHEST_RES_X32         0x05

// Sampling frequency - for BMP390_REG_ODR
#define BMP390_SAMP_FREQ_200        0x00
#define BMP390_SAMP_FREQ_100        0x01
#define BMP390_SAMP_FREQ_50         0x02
#define BMP390_SAMP_FREQ_25         0x03
#define BMP390_SAMP_FREQ_12p5       0x04
#define BMP390_SAMP_FREQ_6p25       0x05
#define BMP390_SAMP_FREQ_3p1        0x06
#define BMP390_SAMP_FREQ_1p5        0x07
#define BMP390_SAMP_FREQ_0p78       0x08
#define BMP390_SAMP_FREQ_0p39       0x09
#define BMP390_SAMP_FREQ_0p2        0x0A
#define BMP390_SAMP_FREQ_0p1        0x0B
#define BMP390_SAMP_FREQ_0p05       0x0C
#define BMP390_SAMP_FREQ_0p02       0x0D
#define BMP390_SAMP_FREQ_0p01       0x0E
#define BMP390_SAMP_FREQ_0p006      0x0F
#define BMP390_SAMP_FREQ_0p003      0x10
#define BMP390_SAMP_FREQ_0p0015     0x11

// IIR filter coef - for BMP390_REG_CONFIG
#define BMP390_FILTER_COEF_OFF  0x0
#define BMP390_FILTER_COEF_2    0x1
#define BMP390_FILTER_COEF_4    0x2
#define BMP390_FILTER_COEF_8    0x3
#define BMP390_FILTER_COEF_16   0x4
#define BMP390_FILTER_COEF_32   0x5
#define BMP390_FILTER_COEF_64   0x6
#define BMP390_FILTER_COEF_128  0x7

// Commands - for BMP390_REG_CMD
#define BMP_CMD_FIFO_FLUSH      0xB0
#define BMP_CMD_SOFT_RESET      0xB6