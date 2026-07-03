#include "app_sm16306s.h"
#include "spi.h"
#include "stdbool.h"

uint8_t sm16306s_data[2] = {0x00,0x00};

/**
 * @brief  将8位数据高低位顺序翻转
 * @param  data: 要处理的数据
 * @retval 返回处理后的数据
 */
uint8_t _corl(uint8_t data)
{
    uint8_t temp = 0;
    for(uint8_t i = 0;i < 8;i++)
    {
        if(((data >> i) & 0x01) == 0x01)
            temp = ((temp<<1) | 0x01);
        else
            temp = temp<<1;
    }   
    return temp;
}

/**
 * @brief  设置指定LED灯珠亮起
 * @param  data: 2个字节的数据，发光灯珠的数据位为1
 * @retval none
 */
void SM16306S_SetLight(uint8_t *data)
{
    //数据高低位互换
    uint8_t buffer[2];
    buffer[0] = _corl(data[1]);
    buffer[1] = _corl(data[0]);
    buffer[1] = buffer[1]>>4|buffer[0]<<4;
    buffer[0] = buffer[0]>>4;

    OE_SET();   //禁止输出防止灯闪烁
    LE_SET();   //解除数据锁存
    HAL_SPI_Transmit(&hspi2,buffer,2,1000);   
    LE_RESET(); //数据锁存
    OE_RESET(); //允许输出
}
