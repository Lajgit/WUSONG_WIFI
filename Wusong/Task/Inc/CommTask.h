#ifndef __COMM_TASK_H__
#define __COMM_TASK_H__

#include "stdint.h"
#include "port_communicate.h"
#include "app_list.h"

// 串口1 消息帧结构体
typedef struct
{
    uint8_t Head;
    uint8_t Code;
    uint8_t Data1;
    uint8_t Data2;
    uint8_t CRC16_H;
    uint8_t CRC16_L;
    uint8_t Tail;
} USART1_MesgTypeDef;

// 串口3 消息帧结构体
typedef struct
{
    uint8_t Head;
    uint8_t Code;
    uint8_t Data1;
    uint8_t Data2;
    uint8_t Data3;
    uint8_t Data4;
    uint8_t CRC32_1;
    uint8_t CRC32_2;
    uint8_t CRC32_3;
    uint8_t CRC32_4;
    uint8_t Tail;
} USART3_MesgTypeDef;

void CommInit(void);
void CommTask(void);
void CommTransmitFillData(Tx_HandleTypeDef *Tx, uint8_t code, uint8_t data1, uint8_t data2);

#endif
