#include "OtaEntry.h"
#include "MainTask.h"
#include "usart.h"

#define OTA_ENTRY_FRAME_LENGTH 7U

static const uint8_t OTA_EntryFrame[OTA_ENTRY_FRAME_LENGTH] =
{
    0xF0, 0x42, 0x4F, 0x54, 0x41, 0x01, 0x00
};

void OTA_EnterBootloader(void)
{
    if (OTA_SetBootRequest() == false)
        return;

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)OTA_EntryFrame,
                      OTA_ENTRY_FRAME_LENGTH,
                      100U);

    HAL_Delay(100U);
    System_Reset();
}
