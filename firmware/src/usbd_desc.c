#include "usbd_core.h"
#include "usbd_desc.h"

static uint8_t device_descriptor[] = {
    0x12, USB_DESC_TYPE_DEVICE, 0x00, 0x02, 0x02, 0x02, 0x00, 0x40,
    0x83, 0x04, 0x40, 0x57, 0x00, 0x02, 1, 2, 3, 1
};
static uint8_t lang_id_descriptor[] = { 4, USB_DESC_TYPE_STRING, 0x09, 0x04 };
static uint8_t manufacturer_string[] = { 10, USB_DESC_TYPE_STRING, 'G',0,'y',0,'r',0,'o',0 };
static uint8_t product_string[] = { 18, USB_DESC_TYPE_STRING, 'F',0,'4',0,'1',0,'1',0,' ',0,'C',0,'D',0,'C',0 };
static uint8_t serial_string[] = { 14, USB_DESC_TYPE_STRING, 'G',0,'Y',0,'R',0,'O',0,'0',0,'1',0 };

static uint8_t *device(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; *length = sizeof(device_descriptor); return device_descriptor; }
static uint8_t *language(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; *length = sizeof(lang_id_descriptor); return lang_id_descriptor; }
static uint8_t *manufacturer(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; *length = sizeof(manufacturer_string); return manufacturer_string; }
static uint8_t *product(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; *length = sizeof(product_string); return product_string; }
static uint8_t *serial(USBD_SpeedTypeDef speed, uint16_t *length)
{ (void)speed; *length = sizeof(serial_string); return serial_string; }

USBD_DescriptorsTypeDef CDC_Desc = {
    device, language, manufacturer, product, serial, language, language
};
