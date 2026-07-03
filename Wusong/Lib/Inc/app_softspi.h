#ifndef __APP_SOFTSPI_H__
#define __APP_SOFTSPI_H__

#include "main.h"
#include "gpio.h"

typedef struct
{
    GPIO_TypeDef *SPI_SDA_GPIOPORT;
    uint16_t SPI_SDA_GPIOPIN;
    GPIO_TypeDef *SPI_CLK_GPIOPORT;
    uint16_t SPI_CLK_GPIOPIN;
    GPIO_TypeDef *SPI_CS_GPIOPORT;
    uint16_t SPI_CS_GPIOPIN;
    uint8_t CLK_CPOL;       // Clock polarity
    GPIO_PinState CS_Level; // Chip select level

    void (*TransmitByte)(void *self, uint8_t data);
    void (*Transmit)(void *self, uint8_t *pData, uint16_t Size);
} SoftwareSPI_HandleTypeDef;

void SoftwareSPI_Init(SoftwareSPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *SDA_GPIOPort, uint16_t SDA_GPIOPin,
                      GPIO_TypeDef *CLK_GPIOPort, uint16_t CLK_GPIOPin,
                      GPIO_TypeDef *CS_GPIOPort, uint16_t CS_GPIOPin,
                      uint8_t clk_cpol, GPIO_PinState cs_level);

#endif