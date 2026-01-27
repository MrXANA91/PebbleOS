#include "board/board.h"
#include "drivers/accel.h"
#include "drivers/i2c.h"
#include "drivers/imu.h"
#include "drivers/imu/bmp390/bmp390.h"
#include "drivers/imu/lsm6dso/lsm6dso.h"
#include "drivers/imu/mmc5603nj/mmc5603nj.h"
#include "drivers/spi.h"
#include "kernel/util/delay.h"

void imu_init(void) {
  bmp390_init();
  lsm6dso_init();
  mmc5603nj_init();
}

void imu_power_up(void) {
  lsm6dso_power_up();
}

void imu_power_down(void) {
  lsm6dso_power_down();
}
