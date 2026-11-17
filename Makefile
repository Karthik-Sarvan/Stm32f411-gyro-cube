TARGET := gyro
BUILD := build
PREFIX ?= arm-none-eabi-
CUBE ?= vendor/STM32CubeF4

CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
FLASH ?= auto

CPU := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
WARNINGS := -Wall -Wextra -Werror
CFLAGS := $(CPU) $(WARNINGS) -std=c11 -Os -g3 \
          -ffreestanding -ffunction-sections -fdata-sections \
          -DSTM32F411xE -DUSE_HAL_DRIVER \
          -Ifirmware/include \
          -I$(CUBE)/Drivers/CMSIS/Include \
          -I$(CUBE)/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
          -I$(CUBE)/Drivers/STM32F4xx_HAL_Driver/Inc \
          -I$(CUBE)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
          -I$(CUBE)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
LDFLAGS := -Tfirmware/STM32F411VETX_FLASH.ld -nostartfiles \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

SOURCES := \
  firmware/src/startup_stm32f411xe.s \
  firmware/src/main.c \
  firmware/src/imu.c \
  firmware/src/stm32f4xx_it.c \
  firmware/src/usb_device.c \
  firmware/src/usbd_desc.c \
  $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
  $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
  $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
  $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
  $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
  $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c \
  $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c \
  $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c \
  $(CUBE)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
  $(CUBE)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
  $(CUBE)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
  $(CUBE)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c \
  $(CUBE)/Projects/STM32F411E-Discovery/Demonstrations/Src/usbd_conf.c
OBJECTS := $(patsubst %.c,$(BUILD)/%.o,$(patsubst %.s,$(BUILD)/%.o,$(SOURCES)))

.PHONY: all clean flash size

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: %.c | $(BUILD)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.s | $(BUILD)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -lm -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(BUILD)/$(TARGET).bin
	@if [ "$(FLASH)" != "auto" ]; then \
		$(FLASH) write $< 0x08000000; \
	elif command -v st-flash >/dev/null 2>&1; then \
		st-flash write $< 0x08000000 && st-flash reset; \
	elif command -v STM32_Programmer_CLI >/dev/null 2>&1; then \
		STM32_Programmer_CLI -c port=SWD -w $< 0x08000000 -v -rst; \
	elif command -v openocd >/dev/null 2>&1; then \
		openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
			-c "program $< verify reset exit 0x08000000"; \
	else \
		echo "No STM32 programmer found."; \
		echo "Install stlink, OpenOCD, or STM32CubeProgrammer, then run make flash."; \
		echo "You can also set FLASH=/path/to/programmer."; \
		exit 127; \
	fi

size: $(BUILD)/$(TARGET).elf
	$(SIZE) $<

clean:
	rm -rf $(BUILD)

-include $(OBJECTS:.o=.d)
