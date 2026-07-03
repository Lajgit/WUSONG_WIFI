#include "MainTask.h"
#include "CommTask.h"
#include "MesgTask.h"
#include "CtrlTask.h"
#include "FlashTask.h"
#include "LightTask.h"
#include "port_communicate.h"
#include "usart.h"
#include "app_crc.h"
#include "app_sm16306s.h"
#include "string.h"

#define Mesg_Head 0xAA
#define Mesg_Tail 0x55

#define ANTI_SHAKE_CODE 0x20
#define ANTI_SHAKE_TRIGGER 0x00
#define ANTI_SHAKE_RELEASE 0x01
#define ANTI_SHAKE_SCAN_PERIOD 10U
#define ANTI_SHAKE_DEBOUNCE_COUNT 3U
#define ANTI_SHAKE_RELEASE_TIME 5000U

static uint8_t rx1_buffer[512];
static uint8_t rx3_buffer[512];

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

Tx_HandleTypeDef Tx1;
Rx_HandleTypeDef Rx1;
Tx_HandleTypeDef Tx3;
Rx_HandleTypeDef Rx3;

USART1_MesgTypeDef USART1_Mesg;
USART3_MesgTypeDef USART3_Mesg;

extern Motor_Hoolle Motor_Hoolle1;
extern Motor_Card Card;
extern Switch_Valve Lock_Valve, Valve;
extern Switch_Valve Left_Valve, Right_Valve;

extern Event_Handle_t Mesg_event;
extern Event_Handle_t Event;
extern Scene_t Scene;

extern uint8_t sm16306s_data[2];
extern uint8_t ButtonLight_Position;
extern Light_t Light;
extern BreathLight_t J20, J21, J22, J23, J24, J26;
extern BreathLight_t *BreathList[];

static bool AntiShake_AlarmActive = false;
static bool AntiShake_InputLow = false;
static uint8_t AntiShake_LowCount = 0;
static uint8_t AntiShake_HighCount = 0;
static uint32_t AntiShake_LastScanTick = 0;
static uint32_t AntiShake_LastTriggerTick = 0;

static void AntiShake_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/* 使用现有7字节协议发送防摇消息，状态放在低位数据Data2 */
static void AntiShake_Transmit(uint8_t state)
{
    CommTransmitFillData(&Tx1, ANTI_SHAKE_CODE, 0x00, state);
}

static void AntiShake_Task(void)
{
    uint32_t now = HAL_GetTick();

    if ((uint32_t)(now - AntiShake_LastScanTick) < ANTI_SHAKE_SCAN_PERIOD)
        return;

    AntiShake_LastScanTick = now;

    if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_6) == GPIO_PIN_RESET)
    {
        AntiShake_HighCount = 0;

        if (AntiShake_LowCount < ANTI_SHAKE_DEBOUNCE_COUNT)
            AntiShake_LowCount++;

        if (AntiShake_LowCount >= ANTI_SHAKE_DEBOUNCE_COUNT &&
            AntiShake_InputLow == false)
        {
            AntiShake_InputLow = true;
            AntiShake_LastTriggerTick = now;

            if (AntiShake_AlarmActive == false)
            {
                AntiShake_Transmit(ANTI_SHAKE_TRIGGER);
                AntiShake_AlarmActive = true;
            }
        }
    }
    else
    {
        AntiShake_LowCount = 0;

        if (AntiShake_HighCount < ANTI_SHAKE_DEBOUNCE_COUNT)
            AntiShake_HighCount++;

        if (AntiShake_HighCount >= ANTI_SHAKE_DEBOUNCE_COUNT)
            AntiShake_InputLow = false;
    }

    if (AntiShake_AlarmActive == true &&
        AntiShake_InputLow == false &&
        (uint32_t)(now - AntiShake_LastTriggerTick) >= ANTI_SHAKE_RELEASE_TIME)
    {
        AntiShake_Transmit(ANTI_SHAKE_RELEASE);
        AntiShake_AlarmActive = false;
    }
}

/* 串口1解包与处理*/
static bool USART1_ReceiveMesg_Verify(void *self, void *mesg)
{
    Rx_HandleTypeDef *rx = (Rx_HandleTypeDef *)self;
    USART1_MesgTypeDef *Rx_mesg = (USART1_MesgTypeDef *)mesg;
    uint16_t crc16, mesg_crc16;
    crc16 = CRC16_calculate(rx->Queue.Buf, 4);
    mesg_crc16 = Rx_mesg->CRC16_H << 8 | Rx_mesg->CRC16_L;
    if (crc16 == mesg_crc16)
        return true;
    return false;
}

static void USART1_Deal(void *Rx_mesg)
{
    USART1_MesgTypeDef *mesg = (USART1_MesgTypeDef *)Rx_mesg;
    uint16_t data;
    switch (mesg->Code)
    {
    case 0x01: // 吐珠
        data = (mesg->Data1 << 8) | mesg->Data2;
        Hoolle_Output(&Motor_Hoolle1, data);
        EventGroupSetBits(&Mesg_event, MesgEvent_RemainingHoolle);
        break;
    case 0x02: // 出卡
        data = (mesg->Data1 << 8) | mesg->Data2;
        Card_Output(&Card, data);
        EventGroupSetBits(&Mesg_event, MesgEvent_RemainingHoolle);
        break;
    case 0x03: // 挡珠电磁阀
        Valve_Start(&Valve, 1);
        break;
    case 0x04: // 亮度调节
        if (mesg->Data1 == 0x01)
        {
            Setting.Board_Lightness = mesg->Data2;
            Light.Init = true;
        }
        else if (mesg->Data1 == 0x05)
        {
            Setting.LightBelt_Lightness = mesg->Data2;
            BreathLight_RefreshState(BreathList, 6);
        }
        break;
    case 0x05: // 设置场景
        if (Scene == SCENE_PLAYING && mesg->Data2 == 6)
            return;
        Scene = mesg->Data2;
        Light.Init = true;
        EventGroupSetBits(&Event, Event_SceneChange);
        BreathLight_RefreshState(BreathList, 6);
        SemaphoreGive(Light.Semaphore);
        break;
    case 0x06: // 设置中奖通道
        sm16306s_data[0] = mesg->Data1;
        sm16306s_data[1] = mesg->Data2;
        SM16306S_SetLight(sm16306s_data);   // 设置中奖灯
        SM16306S_SetChannel(sm16306s_data); // 设置中奖通道
        break;
    case 0x07: // 接收开心30s游戏中奖状态
        if (mesg->Data2 == 0x01)
            EventGroupSetBits(&Event, Event_Happy30s_Win);
        else if (mesg->Data2 == 0x00)
            EventGroupSetBits(&Event, Event_Happy30s_Lose);
        break;
    case 0x08: // 设置拍拍按键灯光
        ButtonLight_Position = mesg->Data2;
        break;
    case 0x09:
        // 清珠
        if (mesg->Data1 == 0x01)
        {
            Hoolle_Output(&Motor_Hoolle1, 0xFFFF - Motor_Hoolle1.Hoolle_num);
        }
        // 吐出剩余
        else if (mesg->Data1 == 0x03)
        {
            Hoolle_Output(&Motor_Hoolle1, 0);
            Card_Output(&Card, 0);
            // Clear a ball still held in either mini-game hole.
            MiniGameBall_EjectIfOccupied();
        }
        break;
    case 0x0A: // 保存设置
        EventGroupSetBits(&Event, Event_SaveSetting);
        break;
    case 0x0B: // 恢复默认设置
        ResumeSetting();
        break;
    case 0x0C: // 左右电磁阀触发
        Valve_Start(&Left_Valve, Valve_TriggerCount);
        Valve_Start(&Right_Valve, Valve_TriggerCount);
        break;
    case 0x0D: // 重新綁卡

    case 0x0E: // 球盘重启
        System_Reset();
        break;
    case 0x0F: // 开锁
        EventGroupSetBits(&Mesg_event, MesgEvent_Unlock);
        break;
    case 0xFF: // 停止所有输出
        Device_Stop();
        EventGroupSetBits(&Mesg_event, MesgEvent_CardOutputFinish); // 吐卡完成
        break;
    }
}
/* 串口3解包与处理*/
static bool USART3_ReceiveMesg_Verify(void *self, void *mesg)
{
    Rx_HandleTypeDef *rx = (Rx_HandleTypeDef *)self;
    USART3_MesgTypeDef *Rx_mesg = (USART3_MesgTypeDef *)mesg;
    uint32_t crc32, mesg_crc32;
    crc32 = CRC32_calculate(rx->Queue.Buf, 6);
    mesg_crc32 = Rx_mesg->CRC32_1 << 24 | Rx_mesg->CRC32_2 << 16 | Rx_mesg->CRC32_3 << 8 | Rx_mesg->CRC32_4;
    if (crc32 == mesg_crc32)
        return true;
    return false;
}

static void USART3_Deal(void *Rx_mesg)
{
    USART3_MesgTypeDef *mesg = (USART3_MesgTypeDef *)Rx_mesg;
    switch (mesg->Code)
    {
    case 0xA1:
        EventGroupSetBits(&Mesg_event, MesgEvent_Unlock);
        break;
    case 0xA2:
        EventGroupSetBits(&Mesg_event, MesgEvent_NFCEnterSetting);
        break;
    case 0xAF:
        System_Reset();
        break;
    }
}

void CommInit(void)
{
    Rx_InitTypeDef Rxinit;
    Rxinit.huart = &huart1;
    Rxinit.RingBuf = rx1_buffer;
    Rxinit.RingBuf_Size = sizeof(rx1_buffer);
    Rxinit.Frame_Head = Mesg_Head;
    Rxinit.Frame_Tail = Mesg_Tail;
    Rxinit.Receive = NULL;
    Rxinit.Verify = USART1_ReceiveMesg_Verify;
    Rxinit.Deal = USART1_Deal;

    Communicate_Rx_Init(&Rx1, Rxinit);

    Rxinit.huart = &huart3;
    Rxinit.RingBuf = rx3_buffer;
    Rxinit.RingBuf_Size = sizeof(rx3_buffer);
    Rxinit.Verify = USART3_ReceiveMesg_Verify;
    Rxinit.Deal = USART3_Deal;
    Communicate_Rx_Init(&Rx3, Rxinit);

    Tx_InitTypeDef Txinit;
    Txinit.huart = &huart1;
    Txinit.hdma = NULL;
    Txinit.TxBuf = NULL;
    Txinit.TxBuf_Size = 0;
    Communicate_Tx_Init(&Tx1, Txinit);

    Txinit.huart = &huart3;
    Communicate_Tx_Init(&Tx3, Txinit);

    AntiShake_Init();
}

void CommTask(void)
{
    Rx1.Receive(&Rx1, &USART1_Mesg, 7);
    Rx3.Receive(&Rx3, &USART3_Mesg, 11);
    AntiShake_Task();
}

void CommTransmitFillData(Tx_HandleTypeDef *Tx, uint8_t code, uint8_t data1, uint8_t data2)
{
    uint8_t data[7] = {Mesg_Head, 0x00, 0x00, 0x00, 0x00, 0x00, Mesg_Tail};
    data[1] = code;
    data[2] = data1;
    data[3] = data2;
    uint16_t crc16 = CRC16_calculate(data, 4);
    data[4] = (crc16 >> 8) & 0xFF;
    data[5] = crc16 & 0xFF;
    Tx->Transimit(Tx, data, 7);
}
