#include "app_softspi.h"

static inline void SPI_Delay(void)
{
    for (uint16_t i = 0; i < 4; i++)
        __NOP();
}

/*
 * @brief  Transmit a byte of data through the SPI peripheral
 * @param  hspi: SPI handle
 * @param  data: The data to transmit
 * @retval None
 */
static void SoftwareSPI_TransmitByte(void *SPI, uint8_t data)
{
    if (SPI == NULL)
        return;
    SoftwareSPI_HandleTypeDef *hspi = (SoftwareSPI_HandleTypeDef *)SPI;
    HAL_GPIO_WritePin(hspi->SPI_CS_GPIOPORT, hspi->SPI_CS_GPIOPIN, hspi->CS_Level);
    SPI_Delay();
    for (uint8_t i = 0; i < 8; i++)
    {
        HAL_GPIO_WritePin(hspi->SPI_SDA_GPIOPORT, hspi->SPI_SDA_GPIOPIN, (data & (0x80 >> i)));
        SPI_Delay();
        HAL_GPIO_WritePin(hspi->SPI_CLK_GPIOPORT, hspi->SPI_CLK_GPIOPIN, (hspi->CLK_CPOL == 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        SPI_Delay();
        HAL_GPIO_WritePin(hspi->SPI_CLK_GPIOPORT, hspi->SPI_CLK_GPIOPIN, (hspi->CLK_CPOL == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
    HAL_GPIO_WritePin(hspi->SPI_CS_GPIOPORT, hspi->SPI_CS_GPIOPIN, !hspi->CS_Level);
    HAL_GPIO_WritePin(hspi->SPI_SDA_GPIOPORT, hspi->SPI_SDA_GPIOPIN, GPIO_PIN_RESET);
}

/*
 * @brief  Transmit an array of bytes through the SPI peripheral
 * @param  hspi: SPI handle
 * @param  pData: The data to transmit
 * @param  Size: The size of the data to transmit
 * @retval None
 */
static void SoftwareSPI_Transmit(void *SPI, uint8_t *pData, uint16_t Size)
{
    if (SPI == NULL || pData == NULL || Size == 0)
        return;
    SoftwareSPI_HandleTypeDef *hspi = (SoftwareSPI_HandleTypeDef *)SPI;
    HAL_GPIO_WritePin(hspi->SPI_CS_GPIOPORT, hspi->SPI_CS_GPIOPIN, hspi->CS_Level);
    SPI_Delay();
    for (uint8_t i = 0; i < Size; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            HAL_GPIO_WritePin(hspi->SPI_SDA_GPIOPORT, hspi->SPI_SDA_GPIOPIN, (pData[i] & (0x80 >> j)));
            SPI_Delay();
            HAL_GPIO_WritePin(hspi->SPI_CLK_GPIOPORT, hspi->SPI_CLK_GPIOPIN, (hspi->CLK_CPOL == 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            SPI_Delay();
            HAL_GPIO_WritePin(hspi->SPI_CLK_GPIOPORT, hspi->SPI_CLK_GPIOPIN, (hspi->CLK_CPOL == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
        }
    }
    HAL_GPIO_WritePin(hspi->SPI_CS_GPIOPORT, hspi->SPI_CS_GPIOPIN, !hspi->CS_Level);
    HAL_GPIO_WritePin(hspi->SPI_SDA_GPIOPORT, hspi->SPI_SDA_GPIOPIN, GPIO_PIN_RESET);
}

void SoftwareSPI_Init(SoftwareSPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *SDA_GPIOPort, uint16_t SDA_GPIOPin,
                      GPIO_TypeDef *CLK_GPIOPort, uint16_t CLK_GPIOPin,
                      GPIO_TypeDef *CS_GPIOPort, uint16_t CS_GPIOPin,
                      uint8_t clk_cpol, GPIO_PinState cs_level)
{
    hspi->SPI_SDA_GPIOPORT = SDA_GPIOPort;
    hspi->SPI_SDA_GPIOPIN = SDA_GPIOPin;
    hspi->SPI_CLK_GPIOPORT = CLK_GPIOPort;
    hspi->SPI_CLK_GPIOPIN = CLK_GPIOPin;
    hspi->SPI_CS_GPIOPORT = CS_GPIOPort;
    hspi->SPI_CS_GPIOPIN = CS_GPIOPin;
    hspi->CLK_CPOL = clk_cpol;
    hspi->CS_Level = cs_level;
    hspi->TransmitByte = SoftwareSPI_TransmitByte;
    hspi->Transmit = SoftwareSPI_Transmit;

    // Initialize GPIO pins
    HAL_GPIO_WritePin(hspi->SPI_CS_GPIOPORT, hspi->SPI_CS_GPIOPIN, (hspi->CS_Level == 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(hspi->SPI_CLK_GPIOPORT, hspi->SPI_CLK_GPIOPIN, (hspi->CLK_CPOL == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(hspi->SPI_SDA_GPIOPORT, hspi->SPI_SDA_GPIOPIN, GPIO_PIN_RESET);
}
