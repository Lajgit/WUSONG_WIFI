#include "app_bootloader.h"
#include "app_ota.h"
#include "usart.h"
#include <string.h>

extern FATFS fs;
extern uint8_t r_buffer[1024];
extern UART_HandleTypeDef huart1;

typedef enum
{
    SD_UPGRADE_NOT_FOUND = 0,
    SD_UPGRADE_SUCCESS,
    SD_UPGRADE_FAILED,
} SdUpgradeResult_t;

static bool Boot_ConsumeOtaRequest(void)
{
    uint32_t magic;
    uint32_t timeout;
    uint32_t retry;
    bool ota_requested;

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

    magic = RTC->BKP0R;
    ota_requested = (magic == OTA_REQUEST_MAGIC);

    for (retry = 0U; retry < 3U; retry++)
    {
        RTC->BKP0R = 0U;
        __DSB();
        __ISB();

        if (RTC->BKP0R == 0U)
            break;
    }

    HAL_PWR_DisableBkUpAccess();

    return ota_requested;
}

static SdUpgradeResult_t Boot_InstallFromSd(void)
{
    FIL firmware_file;
    UINT bytes_read;
    uint32_t file_size;
    uint32_t offset = 0U;
    uint32_t stack_pointer;
    uint32_t reset_handler;
    SdUpgradeResult_t result = SD_UPGRADE_FAILED;

    if (mount_disk(&fs, "", 0) != FR_OK)
    {
        f_mount(NULL, "", 0);
        return SD_UPGRADE_NOT_FOUND;
    }

    if (f_open(&firmware_file, FileName_Bin, FA_READ) != FR_OK)
    {
        f_mount(NULL, "", 0);
        return SD_UPGRADE_NOT_FOUND;
    }

    file_size = f_size(&firmware_file);

    if (file_size < 8U || file_size > APP_MAX_SIZE)
        goto finish;

    if (f_read(&firmware_file, r_buffer, 8U, &bytes_read) != FR_OK ||
        bytes_read != 8U)
    {
        goto finish;
    }

    memcpy(&stack_pointer, &r_buffer[0], sizeof(stack_pointer));
    memcpy(&reset_handler, &r_buffer[4], sizeof(reset_handler));

    if (!Boot_VectorIsValid(stack_pointer, reset_handler))
        goto finish;

    if (f_lseek(&firmware_file, 0U) != FR_OK)
        goto finish;

    if (Boot_EraseApplication(file_size) != HAL_OK)
        goto finish;

    while (offset < file_size)
    {
        uint32_t remain = file_size - offset;
        UINT read_size = remain > sizeof(r_buffer) ?
                         sizeof(r_buffer) : (UINT)remain;

        if (read_file(&firmware_file,
                      r_buffer,
                      read_size,
                      &bytes_read) != FR_OK ||
            bytes_read == 0U)
        {
            goto finish;
        }

        if (Boot_FlashWrite(APP_ADDR + offset,
                            r_buffer,
                            bytes_read) != HAL_OK ||
            !Boot_FlashCompare(APP_ADDR + offset,
                               r_buffer,
                               bytes_read))
        {
            goto finish;
        }

        offset += bytes_read;
    }

    if (offset == file_size && Boot_AppIsValid())
        result = SD_UPGRADE_SUCCESS;

finish:
    close_file(&firmware_file);
    f_mount(NULL, "", 0);

    if (result == SD_UPGRADE_SUCCESS)
    {
        uint8_t i;

        for (i = 0U; i < 6U; i++)
        {
            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
            HAL_Delay(100U);
        }

        HAL_Delay(500U);
    }

    return result;
}

void JumpToApplication(void)
{
    typedef void (*AppEntry_t)(void);

    uint32_t app_stack;
    uint32_t app_reset;
    AppEntry_t entry;
    uint32_t i;

    if (!Boot_AppIsValid())
        return;

    app_stack = *(volatile uint32_t *)APP_ADDR;
    app_reset = *(volatile uint32_t *)(APP_ADDR + 4U);
    entry = (AppEntry_t)app_reset;

    __disable_irq();

    HAL_UART_DeInit(&huart1);
    HAL_DeInit();

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    for (i = 0U; i < 8U; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    SCB->VTOR = APP_ADDR;
    __set_CONTROL(0U);
    __set_MSP(app_stack);
    __DSB();
    __ISB();
    __enable_irq();

    entry();

    while (1)
    {
    }
}

void App_Bootloader(void)
{
    OtaMetadata_t metadata;
    SdUpgradeResult_t sd_result;
    bool ota_requested;
    bool recovery_pending = false;

    ota_requested = Boot_ConsumeOtaRequest();

    if (ota_requested)
        OTA_Run();

    if (Boot_ReadMetadata(&metadata) &&
        (metadata.state == OTA_META_VALID ||
         metadata.state == OTA_META_INSTALLING ||
         (metadata.state == OTA_META_INSTALLED &&
          !Boot_AppIsValid())))
    {
        recovery_pending = true;

        if (Boot_InstallCachedImage() == HAL_OK)
        {
            HAL_Delay(100U);
            JumpToApplication();
        }
    }

    if (!recovery_pending)
    {
        sd_result = Boot_InstallFromSd();

        if (sd_result == SD_UPGRADE_SUCCESS)
            JumpToApplication();
    }

    if (Boot_AppIsValid())
        JumpToApplication();

    OTA_Run();
}
