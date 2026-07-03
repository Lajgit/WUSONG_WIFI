#include "KeyTask.h"
#include "MesgTask.h"
#include "MainTask.h"
#include "CommTask.h"
#include "CtrlTask.h"
#include "port_key.h"
#include "port_event.h"
#include "app_sm16306s.h"

static GPIO_TypeDef *Switch_GPIOPort[12] = {Switch0_GPIO_Port, Switch1_GPIO_Port, Switch2_GPIO_Port,
                                            Switch3_GPIO_Port, Switch4_GPIO_Port, Switch5_GPIO_Port, Switch6_GPIO_Port, Switch7_GPIO_Port,
                                            Switch8_GPIO_Port, Switch9_GPIO_Port, Switch10_GPIO_Port, Switch11_GPIO_Port};
static uint16_t Switch_Pin[12] = {Switch0_Pin, Switch1_Pin, Switch2_Pin,
                                  Switch3_Pin, Switch4_Pin, Switch5_Pin, Switch6_Pin, Switch7_Pin,
                                  Switch8_Pin, Switch9_Pin, Switch10_Pin, Switch11_Pin};
static GPIO_TypeDef *Button_GPIOPort[5] = {Button1_GPIO_Port, Button2_GPIO_Port, Button3_GPIO_Port, Button4_GPIO_Port, Button5_GPIO_Port};
static uint16_t Button_Pin[5] = {Button1_Pin, Button2_Pin, Button3_Pin, Button4_Pin, Button5_Pin};

static Key_HandleTypeDef Switch[12];
static Key_HandleTypeDef *Switch_list[12];
static Key_HandleTypeDef Key[5];
static Key_HandleTypeDef *Key_list[5];
static Key_HandleTypeDef KeyBoard[3];
static Key_HandleTypeDef *KeyBoard_list[3];
static Key_HandleTypeDef Hole[2];
static Key_HandleTypeDef *Hole_list[2];

static Event_Handle_t Key_event;
extern Event_Handle_t Mesg_event;
extern Event_Handle_t Event;

extern Tx_HandleTypeDef Tx1;
extern Tx_HandleTypeDef Tx3;

extern Switch_Valve Left_Valve, Right_Valve;
extern uint8_t sm16306s_data[2];
extern bool WinState[12];
extern Scene_t Scene;

#define Event_SettingButtonPress (1u << 0)

/*
 * ----------微动初始化----------
 */
static void Switch_ShortCallback(uint16_t id)
{
    if (Scene == SCENE_PLAYING || Scene == SCENE_ADDITIONAL)
    {
        if (WinState[id] == true)
            CommTransmitFillData(&Tx1, 0x15, 0x01, 0x06);   //赢
        else
            CommTransmitFillData(&Tx1, 0x15, 0x01, 0x07);   //输
    }
    CommTransmitFillData(&Tx3, 0x1f, 0x00, id); //微动编号
}

static void Switch_Init(void)
{
    Key_InitTypeDef Key_InitStruct;
    Key_InitStruct.debounce_time = KEY_DEBOUNCE_TIME;
    Key_InitStruct.longpress_time = KEY_LONG_PRESS_TIME;
    Key_InitStruct.trigger_frequnecy = KEY_LONG_TRIGGER_FREQUENCY;
    Key_InitStruct.short_callback = Switch_ShortCallback;
    Key_InitStruct.long_callback = NULL;
    Key_InitStruct.release_callback = NULL;
    Key_InitStruct.trigger_level = GPIO_PIN_RESET;

    for (uint8_t i = 0; i < 12; i++)
    {
        Key_InitStruct.key_id = i;
        Key_InitStruct.port = Switch_GPIOPort[i];
        Key_InitStruct.pin = Switch_Pin[i];
        Key_Init(&Switch[i], Key_InitStruct);
        Switch_list[i] = &Switch[i];
    }
}

/*
 * ----------小键盘初始化----------
 */

static void KeyBoard_ShortCallback(uint16_t id)
{
    if (id <= 1)
        CommTransmitFillData(&Tx1, 0x13, 0x01, id + 1);
    if (id == 2)
        EventGroupSetBits(&Mesg_event, MesgEvent_Unlock);
}

static void KeyBoard_LongCallback(uint16_t id)
{
    if (id <= 1)
        CommTransmitFillData(&Tx1, 0x13, 0x02, id + 1);
    if (id == 2)
    {
        if (EventGroupCheckBits(&Key_event, Event_SettingButtonPress) == false)
        {
            EventGroupSetBits(&Mesg_event, MesgEvent_ButtonEnterSetting);
            EventGroupSetBits(&Key_event, Event_SettingButtonPress);
        }
    }
}

static void KeyBoard_ReleaseCallback(uint16_t id)
{
    if (id == 2)
        EventGroupClearBits(&Key_event, Event_SettingButtonPress);
}

static void KeyBoard_Init(void)
{
    Key_InitTypeDef Key_InitStruct;
    Key_InitStruct.debounce_time = KEY_DEBOUNCE_TIME;
    Key_InitStruct.longpress_time = 1500;
    Key_InitStruct.trigger_frequnecy = KEY_LONG_TRIGGER_FREQUENCY;
    Key_InitStruct.short_callback = KeyBoard_ShortCallback;
    Key_InitStruct.long_callback = KeyBoard_LongCallback;
    Key_InitStruct.release_callback = KeyBoard_ReleaseCallback;
    Key_InitStruct.trigger_level = GPIO_PIN_RESET;
    // 小键盘1（无功能）
    Key_InitStruct.key_id = 0;
    Key_InitStruct.port = KeyBoard0_GPIO_Port;
    Key_InitStruct.pin = KeyBoard0_Pin;
    Key_Init(&KeyBoard[0], Key_InitStruct);
    // 小键盘3（后台按键）
    Key_InitStruct.key_id = 2;
    Key_InitStruct.port = KeyBoard2_GPIO_Port;
    Key_InitStruct.pin = KeyBoard2_Pin;
    Key_Init(&KeyBoard[2], Key_InitStruct);
    // 小键盘2（内循环加注按键）
    Key_InitStruct.longpress_time = 4000;
    Key_InitStruct.key_id = 1;
    Key_InitStruct.port = KeyBoard1_GPIO_Port;
    Key_InitStruct.pin = KeyBoard1_Pin;
    Key_Init(&KeyBoard[1], Key_InitStruct);

    for (uint8_t i = 0; i < 3; i++)
        KeyBoard_list[i] = &KeyBoard[i];
    EventGroupClearBits(&Mesg_event, Event_SettingButtonPress);
}

/*
 * ----------洞口初始化----------
 */
static void Hole_LongCallback(uint16_t id)
{
    if (id == 0x00)
    {
        if (EventGroupCheckBits(&Event, Event_LeftHoleInside) == false)
        {
            CommTransmitFillData(&Tx1, 0x14, 0x01, 0x00);
            if (Scene != SCENE_ADDITIONAL && Scene != SCENE_PLAYING && Scene != SCENE_HAPPY1 && Scene != SCENE_HAPPY2 && Scene != SCENE_SETTING)
            {
                MiniGameBall_EjectIfOccupied();
            }
            EventGroupSetBits(&Event, Event_LeftHoleInside);
        }
    }
    else if (id == 0x01)
    {
        if (EventGroupCheckBits(&Event, Event_RightHoleInside) == false)
        {
            CommTransmitFillData(&Tx1, 0x14, 0x01, 0x01);
            if (Scene != SCENE_ADDITIONAL && Scene != SCENE_PLAYING && Scene != SCENE_HAPPY1 && Scene != SCENE_HAPPY2 && Scene != SCENE_SETTING)
            {
                MiniGameBall_EjectIfOccupied();
            }
            EventGroupSetBits(&Event, Event_RightHoleInside);
        }
    }
}

static void Hole_ReleaseCallback(uint16_t id)
{
    if (id == 0x00)
        EventGroupClearBits(&Event, Event_LeftHoleInside);
    else if (id == 0x01)
        EventGroupClearBits(&Event, Event_RightHoleInside);
}

static void Hole_Init(void)
{
    Key_InitTypeDef Key_InitStruct;
    Key_InitStruct.debounce_time = KEY_DEBOUNCE_TIME;
    Key_InitStruct.longpress_time = 200;
    Key_InitStruct.trigger_frequnecy = KEY_LONG_TRIGGER_FREQUENCY;
    Key_InitStruct.short_callback = NULL;
    Key_InitStruct.long_callback = Hole_LongCallback;
    Key_InitStruct.release_callback = Hole_ReleaseCallback;
    Key_InitStruct.trigger_level = GPIO_PIN_SET;

    Key_InitStruct.key_id = 0x00;
    Key_InitStruct.port = Hole_Left_GPIO_Port;
    Key_InitStruct.pin = Hole_Left_Pin;
    Key_Init(&Hole[0], Key_InitStruct);

    Key_InitStruct.key_id = 0x01;
    Key_InitStruct.port = Hole_Right_GPIO_Port;
    Key_InitStruct.pin = Hole_Right_Pin;
    Key_Init(&Hole[1], Key_InitStruct);

    Hole_list[0] = &Hole[0];
    Hole_list[1] = &Hole[1];
}

/*
 * ----------按键初始化----------
 */

static void Key_ShortCallback(uint16_t id)
{
    if (id <= 4)
        CommTransmitFillData(&Tx1, 0x12, 0x01, id + 1);
}

static void Key_LongCallback(uint16_t id)
{
    if (id <= 4)
        CommTransmitFillData(&Tx1, 0x12, 0x02, id + 1);
}

static void Key_ReleaseCallback(uint16_t id)
{
}

static void Button_init(void)
{
    Key_InitTypeDef Key_InitStruct;
    Key_InitStruct.debounce_time = KEY_DEBOUNCE_TIME;
    Key_InitStruct.longpress_time = KEY_LONG_PRESS_TIME;
    Key_InitStruct.trigger_frequnecy = KEY_LONG_TRIGGER_FREQUENCY;
    Key_InitStruct.short_callback = Key_ShortCallback;
    Key_InitStruct.long_callback = Key_LongCallback;
    Key_InitStruct.release_callback = Key_ReleaseCallback;
    Key_InitStruct.trigger_level = GPIO_PIN_RESET;
 
    for (uint8_t i = 0; i < 5; i++)
    {
        Key_InitStruct.key_id = i;
        Key_InitStruct.port = Button_GPIOPort[i];
        Key_InitStruct.pin = Button_Pin[i];
        Key_Init(&Key[i], Key_InitStruct);
        Key_list[i] = &Key[i];
    }
}

static void Switch_Check(void)
{
    uint16_t light = 0x0000;
    uint8_t Light[2] = {0x00, 0x00};
    for (uint8_t i = 0; i < 12; i++)
    {
        if (HAL_GPIO_ReadPin(Switch_GPIOPort[i], Switch_Pin[i]) == GPIO_PIN_RESET) // 若有按键被按下
        {
            light |= (0x0001 << i);
        }
    }
    Light[0] = (uint8_t)(light >> 8);
    Light[1] = (uint8_t)(light & 0x00FF);
    SM16306S_SetLight(Light);
}

static void Switch_MatchCheck(void)
{
    for (uint8_t i = 0; i < 12; i++)
    {
        if (HAL_GPIO_ReadPin(Switch_GPIOPort[i], Switch_Pin[i]) == GPIO_PIN_RESET)
        {
            if (WinState[i] == true)
                CommTransmitFillData(&Tx1, 0x15, 0x01, 0x06);
            else
                CommTransmitFillData(&Tx1, 0x15, 0x01, 0x07);
            CommTransmitFillData(&Tx1, 0x1f, 0x00, i);
            break;
        }
    }
}
void KeyAll_Init(void)
{
    Switch_Init();
    KeyBoard_Init();
    Button_init();
    Hole_Init();
}                                                                                                                                                                    

void Key_Task(void)
{
    if (Scene == SCENE_PLAYING || Scene == SCENE_ADDITIONAL)
        Switch_MatchCheck();
    if (Scene == SCENE_SETTING)
        Switch_Check();
    Key_Scan(KeyBoard_list, 3);
    Key_Scan(Key_list, 5);
    Key_Scan(Hole_list, 2);
}
