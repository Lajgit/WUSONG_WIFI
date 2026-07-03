#include "LightTask.h"
#include "MainTask.h"
#include "FlashTask.h"
#include "CtrlTask.h"
#include "app_sm16306s.h"
#include "port_event.h"
#include "tim.h"
#include "stdlib.h"

#define PlayingRefresh_Time 15
#define IdleSceneRefresh_Time 35

RGB_t Light_RGBbuffer[Light_RGBbuffer_SIZE];
uint16_t Light_CRRbuffer[Light_CRRbuffer_SIZE];
Semaphore_t Light_Semaphore = {1};
Light_t Light;
BreathLight_t J20, J21, J22, J23, J24, J26;
BreathLight_t *BreathList[] = {&J20, &J21, &J22, &J23, &J24, &J26};

extern Setting_TypeDef Setting;
extern uint8_t ButtonLight_Position;
extern Switch_Valve Lock_Valve, Valve;
extern Switch_Valve Left_Valve, Right_Valve;
extern uint8_t sm16306s_data[2];
extern Event_Handle_t Event;
extern Scene_t Scene;
void Light_Init(void)
{
    RGB_Init(&Light, &htim3, TIM_CHANNEL_4, Light_RGBbuffer_SIZE, Light_RGBbuffer, Light_CRRbuffer, &Light_Semaphore, BRG);
    BreathLight_Init(&J20, &htim2, TIM_CHANNEL_1, GPIOA, GPIO_PIN_15);
    BreathLight_Init(&J21, &htim2, TIM_CHANNEL_2, GPIOB, GPIO_PIN_3);
    BreathLight_Init(&J22, &htim5, TIM_CHANNEL_4, GPIOA, GPIO_PIN_3);
    BreathLight_Init(&J23, &htim5, TIM_CHANNEL_3, GPIOA, GPIO_PIN_2);
    BreathLight_Init(&J24, &htim9, TIM_CHANNEL_2, GPIOE, GPIO_PIN_6);
    BreathLight_Init(&J26, &htim9, TIM_CHANNEL_1, GPIOE, GPIO_PIN_5);
    RegisterLight(ColorLight, &Light);
    RegisterLight(BreathLight, &J20);
    RegisterLight(BreathLight, &J21);
    RegisterLight(BreathLight, &J22);
    RegisterLight(BreathLight, &J23);
    RegisterLight(BreathLight, &J24);
    RegisterLight(BreathLight, &J26);
    SemaphoreGive(&Light_Semaphore);
    EventGroupSetBits(&Event, Event_SceneChange);
    ButtonLight_Position = 0x00;
}

static void SettingSceneLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        sm16306s_data[0] = 0x00;
        sm16306s_data[1] = 0x00;
        SM16306S_SetLight(sm16306s_data);
        EventGroupClearBits(&Event, Event_SceneChange);
    }
    LightEffect_Unblock_SetColor(&Light, 0, Light_RGBbuffer_SIZE, WHITE, Setting.Board_Lightness, 255, true);
    BreathLight_SetLightKeep(&J20, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J21, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J22, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J23, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J24, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J26, 0, Setting.LightBelt_Lightness, 255);
}

static void IdleSceneLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        sm16306s_data[0] = 0x00;
        sm16306s_data[1] = 0x00;
        SM16306S_SetLight(sm16306s_data);
        Light.FirstStepCount = 0;
        EventGroupClearBits(&Event, Event_SceneChange);
    }
    if (Light.TimerCount > IdleSceneRefresh_Time)
    {
        if (Light.FirstStepCount < 28)
        {
            RGB_SetAllColor(&Light, NONE, Setting.Board_Lightness, 255);
            RGB_SetOneColor(&Light, Light.FirstStepCount, WHITE, Setting.Board_Lightness, 255);
        }
        else if (Light.FirstStepCount == 28)
        {
            RGB_SetAllColor(&Light, NONE, Setting.Board_Lightness, 255);
            RGB_SetOneColor(&Light, 41, WHITE, Setting.Board_Lightness, 255);
        }
        else if (Light.FirstStepCount == 29)
        {
            RGB_SetAllColor(&Light, NONE, Setting.Board_Lightness, 255);
            RGB_SetOneColor(&Light, 50, WHITE, Setting.Board_Lightness, 255);
        }
        else if (Light.FirstStepCount == 30)
        {
            RGB_SetAllColor(&Light, NONE, Setting.Board_Lightness, 255);
            RGB_SetOneColor(&Light, 64, WHITE, Setting.Board_Lightness, 255);
        }
        else if (Light.FirstStepCount == 31)
        {
            RGB_SetAllColor(&Light, NONE, Setting.Board_Lightness, 255);
            RGB_SetOneColor(&Light, 91, WHITE, Setting.Board_Lightness, 255);
        }
        else if (Light.FirstStepCount >= 32 && Light.FirstStepCount <= 99)
        {
            RGB_SetOneColor(&Light, 91 - (Light.FirstStepCount - 32), RGB_Color(rand() % 256, rand() % 256, rand() % 256), Setting.Board_Lightness, 255);
            RGB_SetOneColor(&Light, 91 + (Light.FirstStepCount - 32), RGB_Color(rand() % 256, rand() % 256, rand() % 256), Setting.Board_Lightness, 255);
        }
        RGB_LocalRefresh(&Light, 0, Light_RGBbuffer_SIZE);
        Light.TimerCount = 0;
        Light.FirstStepCount++;
        if (Light.FirstStepCount >= 100)
            Light.FirstStepCount = 0;
    }
    BreathLight_SetLightKeep(&J20, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J21, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J22, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J23, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J24, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J26, 0, Setting.LightBelt_Lightness, 255);
}

static void RandomSceneLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        sm16306s_data[0] = 0x00;
        sm16306s_data[1] = 0x00;
        SM16306S_SetLight(sm16306s_data);
        EventGroupClearBits(&Event, Event_SceneChange);
    }
    LightEffect_Unblock_SetRand(&Light, 0, Light_RGBbuffer_SIZE, Setting.Board_Lightness, 255, 500);
    BreathLight_SetLightKeep(&J20, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J21, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J22, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J23, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J24, 0, Setting.LightBelt_Lightness, 255);
    BreathLight_SetLightKeep(&J26, 0, Setting.LightBelt_Lightness, 255);
}

static void HoolleInputSceneLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        EventGroupClearBits(&Event, Event_SceneChange);
    }
    LightEffect_Unblock_Breath(&Light, 0, Light_RGBbuffer_SIZE, WHITE, Setting.Board_Lightness, 5, 5, Breath_LinearlyFovea, 0);
    if (EventGroupCheckBits(&Event, Event_HoolleInput) == true)
    {
        Light.Init = true;
        EventGroupClearBits(&Event, Event_HoolleInput);
    }
}

static void PublicRatioLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        sm16306s_data[0] = 0x00;
        sm16306s_data[1] = 0x00;
        SM16306S_SetLight(sm16306s_data);
        EventGroupClearBits(&Event, Event_SceneChange);
        Left_Valve.Switch.state = DEVICE_STATE_STOP;
        Right_Valve.Switch.state = DEVICE_STATE_STOP;
    }
    LightEffect_Unblock_SetRand(&Light, 0, Light_RGBbuffer_SIZE, Setting.Board_Lightness, 255, 30);
    BreathLight_SetLightEffect(&J20, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyFovea, 1);
    BreathLight_SetLightEffect(&J21, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyFovea, 1);
    BreathLight_SetLightEffect(&J22, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyFovea, 1);
    BreathLight_SetLightEffect(&J23, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyFovea, 1);
    BreathLight_SetLightEffect(&J24, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyFovea, 1);
    BreathLight_SetLightEffect(&J26, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyFovea, 1);
}

static void AdditionalLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        EventGroupClearBits(&Event, Event_SceneChange);
    }
    LightEffect_Unblock_Breath(&Light, 0, Light_RGBbuffer_SIZE, WHITE, Setting.Board_Lightness, 5, 5, Breath_LinearlyFovea, 0);
    if (EventGroupCheckBits(&Event, Event_HoolleInput) == true)
    {
        Light.Init = true;
        EventGroupClearBits(&Event, Event_HoolleInput);
    }
}

static void PlayingSceneLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        EventGroupClearBits(&Event, Event_SceneChange);
        RGB_SetMoreColor(&Light, 22, Light_RGBbuffer_SIZE, SKYBLUE, Setting.Board_Lightness, 255);
        RGB_LocalRefresh(&Light, 22, Light_RGBbuffer_SIZE);
        //Valve_Start(&Valve);
    }
    if (Light.TimerCount > PlayingRefresh_Time)
    {
        RGB_SetMoreColor(&Light, 0, 22, NONE, Setting.Board_Lightness, 255);
        RGB_SetOneColor(&Light, Light.FirstStepCount, SKYBLUE, Setting.Board_Lightness, 255);
        if (Light.FirstStepCount < 22)
            Light.FirstStepCount++;
        else
            Light.FirstStepCount = 0;
        RGB_LocalRefresh(&Light, 0, 22);
        Light.TimerCount = 0;
    }
}
static void VictorySceneLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        sm16306s_data[0] = 0x00;
        sm16306s_data[1] = 0x00;
        SM16306S_SetLight(sm16306s_data);
        EventGroupClearBits(&Event, Event_SceneChange);
        Valve.Switch.state = DEVICE_STATE_STOP;
    }
    BreathLight_SetBlink(&J20, 200, Setting.LightBelt_Lightness, 100, 255, 0);
    BreathLight_SetBlink(&J21, 200, Setting.LightBelt_Lightness, 100, 255, 0);
    BreathLight_SetBlink(&J22, 200, Setting.LightBelt_Lightness, 100, 255, 0);
    BreathLight_SetBlink(&J23, 200, Setting.LightBelt_Lightness, 100, 255, 0);
    BreathLight_SetBlink(&J24, 200, Setting.LightBelt_Lightness, 100, 255, 0);
    BreathLight_SetBlink(&J26, 200, Setting.LightBelt_Lightness, 100, 255, 0);
    LightEffect_Unblock_Blink(&Light, 0, Light_RGBbuffer_SIZE, YELLOW, Setting.Board_Lightness, 255, 100);
}

static void DefeatSceneLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        sm16306s_data[0] = 0x00;
        sm16306s_data[1] = 0x00;
        SM16306S_SetLight(sm16306s_data);
        EventGroupClearBits(&Event, Event_SceneChange);
        Valve.Switch.state = DEVICE_STATE_STOP;
    }
    LightEffect_Unblock_Breath(&Light, 0, Light_RGBbuffer_SIZE, RED, Setting.Board_Lightness, 15, 2, Breath_LinearlyDiminish, false);
    BreathLight_SetLightEffect(&J20, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
    BreathLight_SetLightEffect(&J21, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
    BreathLight_SetLightEffect(&J22, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
    BreathLight_SetLightEffect(&J23, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
    BreathLight_SetLightEffect(&J24, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
    BreathLight_SetLightEffect(&J26, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
}

static void Happy30sLight(void)
{ 
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        sm16306s_data[0] = 0x00;
        sm16306s_data[1] = 0x00;
        SM16306S_SetLight(sm16306s_data);
        EventGroupClearBits(&Event, Event_SceneChange);
        Valve.Switch.state = DEVICE_STATE_STOP;
    }
    if (EventGroupCheckBits(&Event, Event_Happy30s_Win) == true)
    {
        LightEffect_Unblock_Blink(&Light, 0, Light_RGBbuffer_SIZE, YELLOW, Setting.Board_Lightness, 255, 100);
        BreathLight_SetBlink(&J20, 200, Setting.LightBelt_Lightness, 100, 255, 0);
        BreathLight_SetBlink(&J21, 200, Setting.LightBelt_Lightness, 100, 255, 0);
        BreathLight_SetBlink(&J22, 200, Setting.LightBelt_Lightness, 100, 255, 0);
        BreathLight_SetBlink(&J23, 200, Setting.LightBelt_Lightness, 100, 255, 0);
        BreathLight_SetBlink(&J24, 200, Setting.LightBelt_Lightness, 100, 255, 0);
        BreathLight_SetBlink(&J26, 200, Setting.LightBelt_Lightness, 100, 255, 0);
    }
    if (EventGroupCheckBits(&Event, Event_Happy30s_Lose) == true)
    {
        LightEffect_Unblock_Breath(&Light, 0, Light_RGBbuffer_SIZE, RED, Setting.Board_Lightness, 15, 2, Breath_LinearlyDiminish, false);
        BreathLight_SetLightEffect(&J20, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
        BreathLight_SetLightEffect(&J21, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
        BreathLight_SetLightEffect(&J22, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
        BreathLight_SetLightEffect(&J23, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
        BreathLight_SetLightEffect(&J24, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
        BreathLight_SetLightEffect(&J26, 15, 1, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyDiminish, 0);
    }
    LightEffect_Unblock_SetRand(&Light, 0, Light_RGBbuffer_SIZE, Setting.Board_Lightness, 255, 5);
}

static void EndLight(void)
{
    if (EventGroupCheckBits(&Event, Event_SceneChange) == true)
    {
        sm16306s_data[0] = 0x00;
        sm16306s_data[1] = 0x00;
        SM16306S_SetLight(sm16306s_data);
        EventGroupClearBits(&Event, Event_SceneChange);
    }
    EventGroupClearBits(&Event, Event_Happy30s_Win);
    EventGroupClearBits(&Event, Event_Happy30s_Lose);
    LightEffect_Unblock_SetColor(&Light, 0, Light_RGBbuffer_SIZE, SKYBLUE, Setting.Board_Lightness, 255, false);
    BreathLight_SetLightEffect(&J22, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyIncrease, 0);
    BreathLight_SetLightEffect(&J23, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyIncrease, 0);
    BreathLight_SetLightEffect(&J24, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyIncrease, 0);
    BreathLight_SetLightEffect(&J26, 5, 5, Setting.LightBelt_Lightness, 0, 0, Breath_LinearlyIncrease, 0);
}
void Light_Task(void)
{
    switch (Scene)
    {
    case SCENE_SETTING:
        SettingSceneLight();
        break;
    case SCENE_IDLE:
        IdleSceneLight();
        break;
    case SCENE_RANDOM:
        RandomSceneLight();
        break;
    case SCENE_HOOLLE_INPUT:
        HoolleInputSceneLight();
        break;
    case SCENE_PUBLIC_RATIO:
        PublicRatioLight();
        break;
    case SCENE_ADDITIONAL:
        AdditionalLight();
        break;
    case SCENE_PLAYING:
        PlayingSceneLight();
        break;
    case SCENE_VICTORY:
        VictorySceneLight();
        break;
    case SCENE_DEFEAT:
        DefeatSceneLight();
        break;
    case SCENE_HAPPY1:
        Happy30sLight();
        break;
    case SCENE_HAPPY2:
        Happy30sLight();
        break;
    case SCENE_END:
        EndLight();
        break;
    }
}