#include "app_bootloader.h"
#include <stddef.h>
#include <string.h>

static void Flash_ClearFlags(void)
{
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

static uint32_t Flash_GetSector(uint32_t address)
{
    if (address < 0x08004000U) return FLASH_SECTOR_0;
    if (address < 0x08008000U) return FLASH_SECTOR_1;
    if (address < 0x0800C000U) return FLASH_SECTOR_2;
    if (address < 0x08010000U) return FLASH_SECTOR_3;
    if (address < 0x08020000U) return FLASH_SECTOR_4;
    if (address < 0x08040000U) return FLASH_SECTOR_5;
    if (address < 0x08060000U) return FLASH_SECTOR_6;
    if (address < 0x08080000U) return FLASH_SECTOR_7;
    if (address < 0x080A0000U) return FLASH_SECTOR_8;
    if (address < 0x080C0000U) return FLASH_SECTOR_9;
    if (address < 0x080E0000U) return FLASH_SECTOR_10;
    return FLASH_SECTOR_11;
}

static HAL_StatusTypeDef Boot_EraseSectors(uint32_t first_sector,
                                           uint32_t sector_count)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t sector_error = 0U;
    HAL_StatusTypeDef status;

    if (sector_count == 0U)
        return HAL_ERROR;

    if (HAL_FLASH_Unlock() != HAL_OK)
        return HAL_ERROR;

    Flash_ClearFlags();

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector = first_sector;
    erase.NbSectors = sector_count;

    status = HAL_FLASHEx_Erase(&erase, &sector_error);
    HAL_FLASH_Lock();

    return status;
}

HAL_StatusTypeDef Boot_EraseApplication(uint32_t image_size)
{
    uint32_t last_address;
    uint32_t first_sector;
    uint32_t last_sector;

    if (image_size == 0U || image_size > APP_MAX_SIZE)
        return HAL_ERROR;

    last_address = APP_ADDR + image_size - 1U;
    first_sector = Flash_GetSector(APP_ADDR);
    last_sector = Flash_GetSector(last_address);

    if (last_sector > FLASH_SECTOR_7)
        return HAL_ERROR;

    return Boot_EraseSectors(first_sector, last_sector - first_sector + 1U);
}

HAL_StatusTypeDef Boot_EraseCache(void)
{
    return Boot_EraseSectors(FLASH_SECTOR_8, 3U);
}

bool Boot_FlashCompare(uint32_t address,
                       const uint8_t *data,
                       uint32_t size)
{
    if (data == NULL)
        return false;

    return memcmp((const void *)address, data, size) == 0;
}

bool Boot_VectorIsValid(uint32_t stack_pointer, uint32_t reset_handler)
{
    uint32_t address;

    if (stack_pointer < 0x20000000U || stack_pointer >= 0x20020000U)
        return false;

    if ((reset_handler & 1U) == 0U)
        return false;

    address = reset_handler & ~1U;
    return address >= APP_ADDR && address < APP_END_ADDR;
}

bool Boot_AppIsValid(void)
{
    uint32_t stack_pointer = *(volatile uint32_t *)APP_ADDR;
    uint32_t reset_handler = *(volatile uint32_t *)(APP_ADDR + 4U);

    return Boot_VectorIsValid(stack_pointer, reset_handler);
}

bool Boot_CachedImageVectorIsValid(uint32_t image_size)
{
    uint32_t stack_pointer;
    uint32_t reset_handler;

    if (image_size < 8U ||
        image_size > OTA_CACHE_MAX_SIZE ||
        image_size > APP_MAX_SIZE)
    {
        return false;
    }

    stack_pointer = *(volatile uint32_t *)OTA_CACHE_ADDR;
    reset_handler = *(volatile uint32_t *)(OTA_CACHE_ADDR + 4U);

    return Boot_VectorIsValid(stack_pointer, reset_handler);
}
