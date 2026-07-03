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

static HAL_StatusTypeDef Flash_WriteUnlocked(uint32_t start_address,
                                             const uint8_t *data,
                                             uint32_t size)
{
    uint32_t address = start_address;
    uint32_t offset = 0U;

    if ((start_address & 3U) != 0U || data == NULL || size == 0U)
        return HAL_ERROR;

    while (offset < size)
    {
        uint32_t word = 0xFFFFFFFFU;
        uint32_t remain = size - offset;
        uint32_t copy_size = remain >= 4U ? 4U : remain;

        memcpy(&word, &data[offset], copy_size);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, word) != HAL_OK)
            return HAL_ERROR;

        address += 4U;
        offset += copy_size;
    }

    return HAL_OK;
}

HAL_StatusTypeDef Boot_FlashWrite(uint32_t start_address,
                                  const uint8_t *data,
                                  uint32_t size)
{
    HAL_StatusTypeDef status;

    if ((start_address & 3U) != 0U || data == NULL || size == 0U)
        return HAL_ERROR;

    if (HAL_FLASH_Unlock() != HAL_OK)
        return HAL_ERROR;

    Flash_ClearFlags();
    status = Flash_WriteUnlocked(start_address, data, size);
    HAL_FLASH_Lock();

    return status;
}

HAL_StatusTypeDef Flash_Program(uint32_t StartAddress, uint8_t *Data, uint32_t Size)
{
    return Boot_FlashWrite(StartAddress, Data, Size);
}
