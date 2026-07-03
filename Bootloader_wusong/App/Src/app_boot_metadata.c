#include "app_bootloader.h"
#include "app_crc32.h"
#include <stddef.h>
#include <string.h>

static uint32_t Metadata_CalculateCrc(const OtaMetadata_t *metadata)
{
    uint32_t fields[4];

    fields[0] = metadata->magic;
    fields[1] = metadata->version_code;
    fields[2] = metadata->image_size;
    fields[3] = metadata->image_crc32;

    return CRC32_Calculate((const uint8_t *)fields, sizeof(fields));
}

bool Boot_ReadMetadata(OtaMetadata_t *metadata)
{
    uint32_t installing_marker;
    uint32_t installed_marker;

    if (metadata == NULL)
        return false;

    memcpy(metadata,
           (const void *)OTA_META_VALID_ADDR,
           sizeof(*metadata));

    if (metadata->magic != OTA_METADATA_MAGIC ||
        metadata->state != OTA_META_VALID)
    {
        return false;
    }

    if (metadata->image_size == 0U ||
        metadata->image_size > OTA_CACHE_MAX_SIZE ||
        metadata->image_size > APP_MAX_SIZE)
    {
        return false;
    }

    if (metadata->header_crc32 != Metadata_CalculateCrc(metadata))
        return false;

    installing_marker =
        *(volatile uint32_t *)OTA_META_INSTALLING_MARKER_ADDR;
    installed_marker =
        *(volatile uint32_t *)OTA_META_INSTALLED_MARKER_ADDR;

    if (installed_marker == OTA_META_MARKER_SET)
        metadata->state = OTA_META_INSTALLED;
    else if (installing_marker == OTA_META_MARKER_SET)
        metadata->state = OTA_META_INSTALLING;
    else
        metadata->state = OTA_META_VALID;

    return true;
}

HAL_StatusTypeDef Boot_WriteMetadata(uint32_t version_code,
                                     uint32_t image_size,
                                     uint32_t image_crc32)
{
    OtaMetadata_t metadata;

    metadata.magic = OTA_METADATA_MAGIC;
    metadata.state = OTA_META_VALID;
    metadata.version_code = version_code;
    metadata.image_size = image_size;
    metadata.image_crc32 = image_crc32;
    metadata.header_crc32 = Metadata_CalculateCrc(&metadata);

    if (*(volatile uint32_t *)OTA_META_INSTALLING_MARKER_ADDR !=
            OTA_META_MARKER_EMPTY ||
        *(volatile uint32_t *)OTA_META_INSTALLED_MARKER_ADDR !=
            OTA_META_MARKER_EMPTY)
    {
        return HAL_ERROR;
    }

    return Boot_FlashWrite(OTA_META_VALID_ADDR,
                           (const uint8_t *)&metadata,
                           sizeof(metadata));
}

HAL_StatusTypeDef Boot_SetMetadataState(uint32_t state)
{
    uint32_t address;
    uint32_t marker = OTA_META_MARKER_SET;
    HAL_StatusTypeDef status;

    if (state == OTA_META_INSTALLING)
        address = OTA_META_INSTALLING_MARKER_ADDR;
    else if (state == OTA_META_INSTALLED)
        address = OTA_META_INSTALLED_MARKER_ADDR;
    else
        return HAL_ERROR;

    if (*(volatile uint32_t *)address == OTA_META_MARKER_SET)
        return HAL_OK;

    if (*(volatile uint32_t *)address != OTA_META_MARKER_EMPTY)
        return HAL_ERROR;

    status = Boot_FlashWrite(address,
                             (const uint8_t *)&marker,
                             sizeof(marker));
    if (status != HAL_OK)
        return status;

    __DSB();
    __ISB();

    if (*(volatile uint32_t *)address != OTA_META_MARKER_SET)
        return HAL_ERROR;

    return HAL_OK;
}
