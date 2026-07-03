#include "app_bootloader.h"
#include "app_crc32.h"

HAL_StatusTypeDef Boot_InstallCachedImage(void)
{
    OtaMetadata_t metadata;
    uint32_t offset = 0U;

    if (!Boot_ReadMetadata(&metadata))
        return HAL_ERROR;

    if (CRC32_CalculateFlash(OTA_CACHE_ADDR, metadata.image_size) !=
        metadata.image_crc32)
    {
        return HAL_ERROR;
    }

    if (!Boot_CachedImageVectorIsValid(metadata.image_size))
        return HAL_ERROR;

    if (metadata.state != OTA_META_INSTALLING)
    {
        if (Boot_SetMetadataState(OTA_META_INSTALLING) != HAL_OK)
            return HAL_ERROR;
    }

    if (Boot_EraseApplication(metadata.image_size) != HAL_OK)
        return HAL_ERROR;

    while (offset < metadata.image_size)
    {
        uint32_t remain = metadata.image_size - offset;
        uint32_t block_size = remain > 1024U ? 1024U : remain;

        if (Boot_FlashWrite(APP_ADDR + offset,
                            (const uint8_t *)(OTA_CACHE_ADDR + offset),
                            block_size) != HAL_OK)
        {
            return HAL_ERROR;
        }

        offset += block_size;
    }

    if (CRC32_CalculateFlash(APP_ADDR, metadata.image_size) !=
        metadata.image_crc32)
    {
        return HAL_ERROR;
    }

    if (!Boot_AppIsValid())
        return HAL_ERROR;

    if (Boot_SetMetadataState(OTA_META_INSTALLED) != HAL_OK)
        return HAL_ERROR;

    if (!Boot_ReadMetadata(&metadata) ||
        metadata.state != OTA_META_INSTALLED)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}
