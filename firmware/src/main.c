#include "imu.h"
#include "usb_device.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

uint32_t SystemCoreClock = 16000000u;

#define REG32(address) (*(volatile uint32_t *)(address))
#define RCC_AHB1ENR REG32(0x40023830u)
#define RCC_APB1ENR REG32(0x40023840u)
#define RCC_APB2ENR REG32(0x40023844u)
#define GPIOB_MODER REG32(0x40020400u)
#define GPIOB_OTYPER REG32(0x40020404u)
#define GPIOB_PUPDR REG32(0x4002040Cu)
#define GPIOB_AFRL REG32(0x40020420u)
#define GPIOA_MODER REG32(0x40020000u)
#define GPIOA_AFRL REG32(0x40020020u)
#define GPIOA_ODR REG32(0x40020014u)
#define GPIOE_MODER REG32(0x40021000u)
#define GPIOE_ODR REG32(0x40021014u)
#define GPIOE_PUPDR REG32(0x4002100Cu)
#define SPI1_CR1 REG32(0x40013000u)
#define SPI1_SR REG32(0x40013008u)
#define SPI1_DR REG32(0x4001300Cu)
#define I2C1_CR1 REG32(0x40005400u)
#define I2C1_CR2 REG32(0x40005404u)
#define I2C1_CCR REG32(0x4000541Cu)
#define I2C1_TRISE REG32(0x40005420u)
#define I2C1_DR REG32(0x40005410u)
#define I2C1_SR1 REG32(0x40005414u)
#define I2C1_SR2 REG32(0x40005418u)
#define I2C_TIMEOUT 20000u
static uint8_t output[64];
static uint16_t output_length;

void delay_ms(uint32_t milliseconds) { HAL_Delay(milliseconds); }

static void stream_putc(char value)
{
    if (output_length < sizeof(output)) output[output_length++] = (uint8_t)value;
}

static void stream_text(const char *text)
{
    while (*text != '\0') stream_putc(*text++);
}

static void stream_decimal(int32_t value)
{
    char buffer[12];
    uint32_t i = 0u;
    uint32_t magnitude;
    if (value < 0) { stream_putc('-'); magnitude = (uint32_t)(-value); }
    else magnitude = (uint32_t)value;
    do { buffer[i++] = (char)('0' + magnitude % 10u); magnitude /= 10u; } while (magnitude != 0u);
    while (i != 0u) stream_putc(buffer[--i]);
}

static void stream_angle(float angle)
{
    int32_t centidegrees = (int32_t)(angle * 100.0f);
    stream_decimal(centidegrees / 100);
    stream_putc('.');
    stream_decimal((centidegrees < 0 ? -centidegrees : centidegrees) % 100);
}

static uint8_t spi_transfer(uint8_t value)
{
    while ((SPI1_SR & 2u) == 0u) {}
    SPI1_DR = value;
    while ((SPI1_SR & 1u) == 0u) {}
    return (uint8_t)SPI1_DR;
}

void spi_write(uint8_t reg, uint8_t value)
{
    GPIOE_ODR &= ~(1u << 3);
    spi_transfer(reg & 0x7Fu);
    spi_transfer(value);
    GPIOE_ODR |= (1u << 3);
}

uint8_t spi_read(uint8_t reg)
{
    uint8_t value;
    GPIOE_ODR &= ~(1u << 3);
    spi_transfer(reg | 0x80u);
    value = spi_transfer(0u);
    GPIOE_ODR |= (1u << 3);
    return value;
}

static void board_init(void)
{
    RCC_AHB1ENR |= (1u << 0) | (1u << 1);
    RCC_AHB1ENR |= (1u << 4);
    RCC_APB1ENR |= (1u << 21);
    RCC_APB2ENR |= (1u << 12);
    GPIOB_MODER = (GPIOB_MODER & ~((3u << 12) | (3u << 14))) | (2u << 12) | (2u << 14);
    GPIOB_OTYPER |= (1u << 6) | (1u << 7);
    GPIOB_PUPDR = (GPIOB_PUPDR & ~((3u << 12) | (3u << 14))) | (1u << 12) | (1u << 14);
    GPIOB_AFRL = (GPIOB_AFRL & ~((15u << 24) | (15u << 28))) | (4u << 24) | (4u << 28);
    GPIOA_MODER = (GPIOA_MODER & ~((3u << 10) | (3u << 12) | (3u << 14)))
                | (2u << 10) | (2u << 12) | (2u << 14);
    GPIOA_AFRL = (GPIOA_AFRL & ~((15u << 20) | (15u << 24) | (15u << 28)))
               | (5u << 20) | (5u << 24) | (5u << 28);
    GPIOE_MODER = (GPIOE_MODER & ~(3u << 6)) | (1u << 6);
    GPIOE_ODR |= (1u << 3);
    SPI1_CR1 = (1u << 2) | (1u << 9) | (1u << 8) | (1u << 6);
    I2C1_CR2 = 48u;
    I2C1_CCR = 240u;
    I2C1_TRISE = 49u;
    I2C1_CR1 = 1u;
}

static void clock_init(void)
{
    uint32_t pll = 16u | (192u << 6) | (1u << 16) | (4u << 24);
    REG32(0x40023804u) = pll;
    REG32(0x40023800u) |= (1u << 0) | (1u << 24);
    while ((REG32(0x40023800u) & (1u << 25)) == 0u) {}
    REG32(0x40023C00u) = 1u | (1u << 8);
    REG32(0x40023808u) = 2u;
    while ((REG32(0x40023808u) & 0xCu) != 8u) {}
    SystemCoreClock = 48000000u;
}

void i2c_write(uint8_t address, uint8_t reg, uint8_t value)
{
    uint32_t timeout;

    I2C1_CR1 |= (1u << 8);
    for (timeout = I2C_TIMEOUT; (I2C1_SR1 & 1u) == 0u && timeout != 0u; --timeout) {}
    if (timeout == 0u) { I2C1_CR1 |= (1u << 9); return; }
    I2C1_DR = (uint32_t)(address << 1);
    for (timeout = I2C_TIMEOUT; (I2C1_SR1 & 2u) == 0u && timeout != 0u; --timeout) {}
    if (timeout == 0u) { I2C1_CR1 |= (1u << 9); return; }
    (void)I2C1_SR2;
    I2C1_DR = reg;
    for (timeout = I2C_TIMEOUT; (I2C1_SR1 & 4u) == 0u && timeout != 0u; --timeout) {}
    if (timeout == 0u) { I2C1_CR1 |= (1u << 9); return; }
    I2C1_DR = value;
    for (timeout = I2C_TIMEOUT; (I2C1_SR1 & 4u) == 0u && timeout != 0u; --timeout) {}
    I2C1_CR1 |= (1u << 9);
}
void i2c_read(uint8_t address, uint8_t reg, uint8_t *data, uint8_t count)
{
    uint8_t index;
    uint32_t timeout;

    for (index = 0u; index < count; ++index) data[index] = 0u;
    i2c_write(address, reg, 0u);
    I2C1_CR1 |= (1u << 8);
    for (timeout = I2C_TIMEOUT; (I2C1_SR1 & 1u) == 0u && timeout != 0u; --timeout) {}
    if (timeout == 0u) { I2C1_CR1 |= (1u << 9); return; }
    I2C1_DR = (uint32_t)((address << 1) | 1u);
    for (timeout = I2C_TIMEOUT; (I2C1_SR1 & 2u) == 0u && timeout != 0u; --timeout) {}
    if (timeout == 0u) { I2C1_CR1 |= (1u << 9); return; }
    (void)I2C1_SR2;
    for (index = 0u; index < count; ++index) {
        if (index + 1u == count) I2C1_CR1 &= ~(1u << 10);
        else I2C1_CR1 |= (1u << 10);
        for (timeout = I2C_TIMEOUT; (I2C1_SR1 & (1u << 6)) == 0u && timeout != 0u; --timeout) {}
        if (timeout == 0u) break;
        data[index] = (uint8_t)I2C1_DR;
    }
    I2C1_CR1 |= (1u << 9) | (1u << 10);
}

int main(void)
{
    orientation_t orientation;
    uint32_t previous_tick;
    clock_init();
    HAL_Init();
    board_init();
    usb_device_init();
    imu_init();
    previous_tick = HAL_GetTick();
    for (;;) {
        uint32_t current_tick = HAL_GetTick();
        float dt = (float)(current_tick - previous_tick) * 0.001f;
        previous_tick = current_tick;
        if (dt < 0.001f) dt = 0.001f;
        if (dt > 0.1f) dt = 0.1f;
        imu_update(dt, &orientation);
        stream_text("GYRO,"); stream_angle(orientation.roll); stream_putc(',');
        stream_angle(orientation.pitch); stream_putc(',');
        stream_angle(orientation.yaw); stream_text("\r\n");
        (void)usb_cdc_write(output, output_length);
        output_length = 0u;
        HAL_Delay(10u);
    }
}
