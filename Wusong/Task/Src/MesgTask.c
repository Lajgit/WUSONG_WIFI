#include "MainTask.h"
#include "MesgTask.h"
#include "CommTask.h"
#include "CtrlTask.h"
#include "port_communicate.h"
#include "port_event.h"

extern Tx_HandleTypeDef Tx1;
extern Rx_HandleTypeDef Rx1;
extern Tx_HandleTypeDef Tx3;
extern Rx_HandleTypeDef Rx3;

extern Motor_Hoolle Motor_Hoolle1;
extern Motor_Card Card;
extern Switch_Valve Lock_Valve, Valve;

Event_Handle_t Mesg_event;
// 发送给NFC开锁消息
static uint8_t NFCUnlock_mesg[11] = {0xaa, 0x0f, 0x01, 0x01, 0x01, 0x01, 0x5c, 0x77, 0x08, 0x7f, 0x55};
void Mesg_Task(void)
{
    // 按键进入设置
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_ButtonEnterSetting))
    {
        CommTransmitFillData(&Tx1, 0x13, 0x01, 0x03);
        EventGroupClearBits(&Mesg_event, MesgEvent_ButtonEnterSetting);
    }
    // 开锁
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_Unlock) == true)
    {
        Tx3.Transimit(&Tx3, NFCUnlock_mesg, sizeof(NFCUnlock_mesg)); // 向NFC发送开锁
        CommTransmitFillData(&Tx1, 0x19, 0x03, 0x01);                // 向安卓发送已开锁
        Valve_Start(&Lock_Valve,1);
        EventGroupClearBits(&Mesg_event, MesgEvent_Unlock);
    }
    // 投珠
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_HoolleInput) == true)
    {
        CommTransmitFillData(&Tx1, 0x11, 0x01, 0x02);
        EventGroupClearBits(&Mesg_event, MesgEvent_HoolleInput);
    }
    // 投币
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_CoinInput) == true)
    {
        CommTransmitFillData(&Tx1, 0x11, 0x01, 0x03);
        EventGroupClearBits(&Mesg_event, MesgEvent_CoinInput);
    }
#ifndef BEIJING_NXH_VERSION
    // 吐珠超时
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_HoolleOutputTimeout))
    {
        CommTransmitFillData(&Tx1, 0x16, Motor_Hoolle1.Hoolle_num >> 8, Motor_Hoolle1.Hoolle_num & 0xFF);
        EventGroupClearBits(&Mesg_event, MesgEvent_HoolleOutputTimeout);
    }
#endif
    // 吐卡超时
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_CardOutputTimeout))
    {
        CommTransmitFillData(&Tx1, 0x17, Card.Card_num >> 8, Card.Card_num & 0xFF);
        EventGroupClearBits(&Mesg_event, MesgEvent_CardOutputTimeout);
    }
    // 剩余珠子数
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_RemainingHoolle) == true)
    {
        CommTransmitFillData(&Tx1, 0x1C, Motor_Hoolle1.Hoolle_num >> 8, Motor_Hoolle1.Hoolle_num & 0xFF);
        EventGroupClearBits(&Mesg_event, MesgEvent_RemainingHoolle);
    }
    // NFC进入设置
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_NFCEnterSetting) == true)
    {
        CommTransmitFillData(&Tx1, 0x18, 0x01, 0x01);
        EventGroupClearBits(&Mesg_event, MesgEvent_NFCEnterSetting);
    }
    // 吐卡一次
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_CardOutputOnce) == true)
    {
        CommTransmitFillData(&Tx1, 0x15, 0x01, 0x04);
        EventGroupClearBits(&Mesg_event, MesgEvent_CardOutputOnce);
    }
    // 吐卡完成
    if (EventGroupCheckBits(&Mesg_event, MesgEvent_CardOutputFinish) == true)
    {
        CommTransmitFillData(&Tx1, 0x15, 0x01, 0x05);
        EventGroupClearBits(&Mesg_event, MesgEvent_CardOutputFinish);
    }
}