#ifndef __APP_BOOTLOADER_H__
#define __APP_BOOTLOADER_H__

#include "main.h"
#include "app_sd.h"
#include <stdbool.h>

#define BOOTLOADER_ADDR        0x08000000U
#define BOOTLOADER_MAX_SIZE    0x0000C000U

#define APP_ADDR               0x0800C000U
#define APP_END_ADDR           0x08080000U
#define APP_MAX_SIZE           (APP_END_ADDR - APP_ADDR)

#define OTA_CACHE_ADDR         0x08080000U
#define OTA_META_ADDR          0x080DFF00U
#define OTA_CACHE_MAX_SIZE     (OTA_META_ADDR - OTA_CACHE_ADDR)

#define SETTINGS_ADDR          0x080E0000U

HAL_StatusTypeDef Flash_Program(uint32_t StartAddress, uint8_t *Data, uint32_t Size);
void JumpToApplication(void);
void App_Bootloader(void);

#endif
