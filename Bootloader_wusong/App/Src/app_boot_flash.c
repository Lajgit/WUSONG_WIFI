#include "app_bootloader.h"
#include <stddef.h>
#include <string.h>

static void Flash_ClearFlags(void)
{
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
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
