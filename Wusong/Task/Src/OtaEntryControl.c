#include "OtaEntry.h"
#include "MainTask.h"
#include "usart.h"
#include "app_crc.h"

void OTA_EnterBootloader(void)
{
    uint8_t response[7] = {0xAA, 0xF0, 0x42, 0x4F, 0x00, 0x00, 0x55};
    uint16_t crc16;

    if (OTA_SetBootRequest() == false)
        return;

    crc16 = CRC16_calculate(response, 4);
    response[4] = (uint8_t)(crc16 >> 8);
    response[5] = (uint8_t)crc16;

    HAL_UART_Transmit(&huart1,
                      response,
                      sizeof(response),
                      100U);

    HAL_Delay(100U);
    System_Reset();
}
