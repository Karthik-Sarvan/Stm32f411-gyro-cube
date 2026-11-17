#include "imu.h"
#include <math.h>

extern void i2c_write(uint8_t address, uint8_t reg, uint8_t value);
extern void i2c_read(uint8_t address, uint8_t reg, uint8_t *data, uint8_t count);
extern void spi_write(uint8_t reg, uint8_t value);
extern uint8_t spi_read(uint8_t reg);
extern void delay_ms(uint32_t milliseconds);

static float roll;
static float pitch;
static float yaw;
static float gyro_bias_x;
static float gyro_bias_y;
static float gyro_bias_z;

static int16_t signed_word(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[1] << 8) | data[0]);
}

static int16_t gyro_axis(uint8_t low_register)
{
    return (int16_t)((uint16_t)spi_read(low_register)
                   | ((uint16_t)spi_read((uint8_t)(low_register + 1u)) << 8));
}

void imu_init(void)
{
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;

    spi_write(0x20u, 0x0Fu); /* L3GD20/I3G4250D: normal mode, 95 Hz */
    spi_write(0x23u, 0x00u); /* +/-250 dps */
    i2c_write(0x19u, 0x20u, 0x57u); /* LSM303 accelerometer: 100 Hz */
    i2c_write(0x19u, 0x23u, 0x00u); /* +/-2 g */
    i2c_write(0x1Eu, 0x00u, 0x14u); /* LSM303 magnetometer: 30 Hz */
    i2c_write(0x1Eu, 0x02u, 0x00u); /* continuous conversion */
    delay_ms(100u);
    for (uint32_t sample = 0u; sample < 64u; ++sample) {
        sum_x += gyro_axis(0x28u);
        sum_y += gyro_axis(0x2Au);
        sum_z += gyro_axis(0x2Cu);
        delay_ms(5u);
    }
    gyro_bias_x = (float)sum_x / 64.0f;
    gyro_bias_y = (float)sum_y / 64.0f;
    gyro_bias_z = (float)sum_z / 64.0f;
}

uint8_t imu_update(float dt, orientation_t *out)
{
    uint8_t accel[6], mag[6];
    int16_t ax, ay, az, gx, gy, gz, mx, my, mz;
    float accel_roll, accel_pitch, gyro_x, gyro_y, gyro_z;
    float magnetic_roll, magnetic_pitch, heading_x, heading_y;

    i2c_read(0x19u, 0xA8u, accel, sizeof(accel));
    i2c_read(0x1Eu, 0x03u, mag, sizeof(mag));
    ax = signed_word(&accel[0]);
    ay = signed_word(&accel[2]);
    az = signed_word(&accel[4]);
    mx = (int16_t)(((uint16_t)mag[0] << 8) | mag[1]);
    mz = (int16_t)(((uint16_t)mag[2] << 8) | mag[3]);
    my = (int16_t)(((uint16_t)mag[4] << 8) | mag[5]);
    gx = gyro_axis(0x28u);
    gy = gyro_axis(0x2Au);
    gz = gyro_axis(0x2Cu);

    accel_roll = atan2f((float)ay, (float)az) * 57.29578f;
    accel_pitch = atan2f(-(float)ax, sqrtf((float)ay * ay + (float)az * az)) * 57.29578f;
    gyro_x = ((float)gx - gyro_bias_x) * 0.00875f;
    gyro_y = ((float)gy - gyro_bias_y) * 0.00875f;
    gyro_z = ((float)gz - gyro_bias_z) * 0.00875f;
    roll = 0.98f * (roll + gyro_x * dt) + 0.02f * accel_roll;
    pitch = 0.98f * (pitch + gyro_y * dt) + 0.02f * accel_pitch;
    magnetic_roll = roll * 0.01745329f;
    magnetic_pitch = pitch * 0.01745329f;
    heading_x = (float)mx * cosf(magnetic_pitch) + (float)mz * sinf(magnetic_pitch);
    heading_y = (float)mx * sinf(magnetic_roll) * sinf(magnetic_pitch)
              + (float)my * cosf(magnetic_roll)
              - (float)mz * sinf(magnetic_roll) * cosf(magnetic_pitch);
    if (mx != 0 || my != 0 || mz != 0) {
        yaw += gyro_z * dt;
        float magnetic_yaw = atan2f(-heading_y, heading_x) * 57.29578f;
        float yaw_error = magnetic_yaw - yaw;
        if (yaw_error > 180.0f) yaw_error -= 360.0f;
        if (yaw_error < -180.0f) yaw_error += 360.0f;
        yaw += 0.02f * yaw_error;
    } else {
        yaw = 0.0f;
    }
    if (yaw > 180.0f) yaw -= 360.0f;
    if (yaw < -180.0f) yaw += 360.0f;
    out->roll = roll;
    out->pitch = pitch;
    out->yaw = yaw;
    return 1u;
}
