#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <stdint.h>

void usb_device_init(void);
uint8_t usb_cdc_write(const uint8_t *data, uint16_t length);

#endif
