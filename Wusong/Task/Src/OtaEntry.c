#include "OtaEntry.h"
#include "port_communicate.h"
#include "string.h"

#define OTA_ENTRY_FRAME_LENGTH 7U

static const uint8_t OTA_EntryFrame[OTA_ENTRY_FRAME_LENGTH] =
{
    0xF0, 0x42, 0x4F, 0x54, 0x41, 0x01, 0x00
};

/*
 * 串口1同时接收：
 * 1. 原AA...55固定7字节业务帧；
 * 2. 独立升级入口帧 F0 42 4F 54 41 01 00。
 */
void OTA_EntryReceive(void *self, void *mesg, uint8_t mesg_len)
{
    Rx_HandleTypeDef *rx = (Rx_HandleTypeDef *)self;
    static uint8_t ota_index = 0U;

    while (rx->Handle.RingBuf.f_IsEmpty(&rx->Handle.RingBuf) == false)
    {
        if (rx->Handle.RingBuf.f_ReadByte(&rx->Handle.RingBuf,
                                          &rx->CurrData) == false)
        {
            continue;
        }

        if (rx->State == WAIT_HEAD)
        {
            if (rx->CurrData == OTA_EntryFrame[ota_index])
            {
                ota_index++;

                if (ota_index >= OTA_ENTRY_FRAME_LENGTH)
                {
                    ota_index = 0U;
                    OTA_EnterBootloader();
                    return;
                }
            }
            else
            {
                ota_index =
                    (rx->CurrData == OTA_EntryFrame[0]) ? 1U : 0U;
            }
        }
        else
        {
            ota_index = 0U;
        }

        switch (rx->State)
        {
        case WAIT_HEAD:
            if (rx->CurrData == rx->Frame_Head)
            {
                rx->Queue.Index = 0;
                rx->Queue.Buf[rx->Queue.Index] = rx->CurrData;
                rx->State = RECEIVE_DATA;
                rx->Queue.Index++;
            }
            break;

        case RECEIVE_DATA:
            if (rx->Queue.Index < rx->Queue.Buf_Size)
                rx->Queue.Buf[rx->Queue.Index] = rx->CurrData;
            else
                rx->State = WAIT_HEAD;

            if (rx->CurrData == rx->Frame_Tail &&
                rx->Queue.Index >= mesg_len - 1)
            {
                rx->State = WAIT_HEAD;
                memcpy(mesg, rx->Queue.Buf, mesg_len);

                if (rx->Verify != NULL)
                {
                    if (rx->Verify(rx, mesg) == true)
                    {
                        if (rx->Deal != NULL)
                            rx->Deal(mesg);
                    }
                }
                else
                {
                    if (rx->Deal != NULL)
                        rx->Deal(mesg);
                }
            }

            rx->Queue.Index++;
            break;
        }
    }
}
