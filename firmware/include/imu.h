#ifndef IMU_H
#define IMU_H

#include <stdint.h>

typedef struct {
    float roll;
    float pitch;
    float yaw;
} orientation_t;

void imu_init(void);
uint8_t imu_update(float dt, orientation_t *out);

#endif
