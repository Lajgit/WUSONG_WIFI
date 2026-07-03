#include "OtaEntry.h"
#include "main.h"
#include "stdbool.h"

#define OTA_REQUEST_MAGIC 0x424F5441U

bool OTA_SetBootRequest(void)
{
    uint32_t timeout;
    uint32_t retry;
    bool request_set = false;

    __HAL_RCC_PWR_CLK_ENABLE();
    __DSB();
    (void)RCC->APB1ENR;

    HAL_PWR_EnableBkUpAccess();

    timeout = 100000U;
    while (((PWR->CR & PWR_CR_DBP) == 0U) && (timeout > 0U))
        timeout--;

    if (timeout == 0U)
    {
        HAL_PWR_DisableBkUpAccess();
        return false;
    }

    __HAL_RCC_RTC_ENABLE();
    __DSB();
    (void)RCC->BDCR;

    for (retry = 0U; retry < 3U; retry++)
    {
        RTC->BKP0R = OTA_REQUEST_MAGIC;
        __DSB();
        __ISB();

        if (RTC->BKP0R == OTA_REQUEST_MAGIC)
        {
            request_set = true;
            break;
        }
    }

    HAL_PWR_DisableBkUpAccess();

    return request_set;
}
