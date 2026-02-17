/* SPDX-FileCopyrightText: 2026 Core Devices LLC - 2026 Paul CHANVIN */
/* SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include <stdint.h>

//! Initialize the pressure sensor driver. Call this once at startup.
void pressure_init(void);

//! Get the pressure in milli-Pascal
int32_t pressure_read(void);
