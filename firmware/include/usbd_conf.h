#ifndef USBD_CONF_H
#define USBD_CONF_H

#include "stm32f4xx_hal.h"
#include <string.h>

#define USBD_MAX_NUM_INTERFACES 2U
#define USBD_MAX_NUM_CONFIGURATION 1U
#define USBD_MAX_STR_DESC_SIZ 64U
#define USBD_SELF_POWERED 1U
#define USBD_CDC_INTERVAL 2000U
#define USBD_SUPPORT_USER_STRING_DESC 0U
#define USBD_CLASS_USER_STRING_DESC 0U

#define USBD_malloc(size) USBD_static_malloc(size)
#define USBD_free(pointer) USBD_static_free(pointer)
#define USBD_memset memset
#define USBD_memcpy memcpy
#define USBD_Delay HAL_Delay
#define USBD_UsrLog(...) do {} while (0)
#define USBD_ErrLog(...) do {} while (0)
#define USBD_DbgLog(...) do {} while (0)

void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *pointer);

#endif
