#include "MainTask.h"
#include "CtrlTask.h"
#include "MesgTask.h"
#include "tim.h"
#include "port_device.h"
#include "port_event.h"

Motor_Hoolle Motor_Hoolle1;
Motor_Card Card;
Switch_Valve Lock_Valve, Valve;
Switch_Valve Left_Valve, Right_Valve;
uint8_t ButtonLight_Position;

static GPIO_TypeDef *GPIO_Port[5] = {Button1_LED_GPIO_Port, Button2_LED_GPIO_Port, Button3_LED_GPIO_Port, Button4_LED_GPIO_Port, Button5_LED_GPIO_Port};
static uint16_t GPIO_Pin[5] = {Button1_LED_Pin, Button2_LED_Pin, Button3_LED_Pin, Button4_LED_Pin, Button5_LED_Pin};

extern Event_Handle_t Mesg_event;

/*
 *==============时间基准===============
 */
static inline uint32_t Get_SysTime(void)
{
    return HAL_GetTick();
}
/*
 *==============吐珠电机控制===============
 */
static void Ctrl_HoolleMotor(Motor_Hoolle *Motor, uint16_t speed, uint8_t dir, uint32_t timeout, uint32_t reverse_time, uint8_t retry_times, void (*Timeout_callbcak)(void))
{
    // 开机吐珠电机
    if (Motor->Motor.state == DEVICE_STATE_START)
    {
        Motor->Motor.SetSpeed(&Motor->Motor, speed, dir);
        Motor->Motor.state = DEVICE_STATE_BUSY;
    }
    // 停止吐珠电机
    if (Motor->Motor.state == DEVICE_STATE_STOP)
    {
        Motor->Motor.Stop(&Motor->Motor);
        Motor->Motor.state = DEVICE_STATE_IDLE;
        Motor->Hoolle_num = 0;
    }
    // 吐珠电机超时
    if (Motor->Motor.state == DEVICE_STATE_TIMEOUT)
    {
        // 反转时间到
        if (Motor->Motor.GetRuntime(&Motor->Motor) > HoolleMotorReverse_Time)
        {
            // 翻转次数不够，重新吐出
            if (Motor->RetryCount < retry_times)
            {
                Motor->Motor.state = DEVICE_STATE_START;
                Motor->RetryCount++;
            }
            else
            {
                Motor->Motor.state = DEVICE_STATE_IDLE;
                Motor->Motor.Stop(&Motor->Motor);
                // 超时停转后的反应
                if (Timeout_callbcak != NULL)
                    Timeout_callbcak();
            }
        }
    }
    if (Motor->Motor.state == DEVICE_STATE_PAUSE)
    {
        Motor->Motor.ResetRuntime(&Motor->Motor);
    }
    // 吐珠电机超时
    if (Motor->Motor.GetRuntime(&Motor->Motor) > timeout && Motor->Motor.state != DEVICE_STATE_IDLE)
    {
        Motor->Motor.state = DEVICE_STATE_TIMEOUT;
        Motor->Motor.LosePower(&Motor->Motor);
        HAL_Delay(1);
        // 反转
        Motor->Motor.SetSpeed(&Motor->Motor, speed, !dir);
    }
}

/*
 *==============卡片机控制===============
 */
static void Ctrl_CardMotor(Motor_Card *Card, uint32_t timeout, void (*Timeout_callbcak)(void))
{
    // 开机吐卡
    if (Card->Switch.state == DEVICE_STATE_START)
    {
        Card->Switch.on(&Card->Switch);
        Card->Switch.state = DEVICE_STATE_BUSY;
    }
    // 停止吐卡
    if (Card->Switch.state == DEVICE_STATE_STOP)
    {
        Card->Switch.off(&Card->Switch);
        Card->Switch.state = DEVICE_STATE_IDLE;
        Card->Card_num = 0;
    }
    // 吐卡超时
    if (Card->Switch.state == DEVICE_STATE_TIMEOUT)
    {
        Card->Switch.off(&Card->Switch);
        Card->Switch.state = DEVICE_STATE_IDLE;
        // 吐卡超时反应
        if (Timeout_callbcak != NULL)
            Timeout_callbcak();
    }
    // 吐卡超时判断
    if (Card->Switch.GetRuntime(&Card->Switch) > CardMotorTimeout_time && Card->Switch.state != DEVICE_STATE_IDLE)
    {
        Card->Switch.state = DEVICE_STATE_TIMEOUT;
    }
}

/*
 *==============电磁阀控制===============
 */
static void Ctrl_Valve(Switch_Valve *Valve, uint32_t timeout, void (*Timeout_callbcak)(void))
{
    // 电磁阀启动
    if (Valve->Switch.state == DEVICE_STATE_START)
    {
        Valve->Switch.on(&Valve->Switch);
        Valve->Switch.state = DEVICE_STATE_BUSY;
    }
    // 电磁阀停止
    if (Valve->Switch.state == DEVICE_STATE_STOP)
    {
        Valve->Switch.off(&Valve->Switch);
        Valve->Switch.state = DEVICE_STATE_IDLE;
    }
    // 电磁阀超时
    if (Valve->Switch.state == DEVICE_STATE_TIMEOUT)
    {
        Valve->Switch.state = DEVICE_STATE_IDLE;
        Valve->Switch.off(&Valve->Switch);
        if (Timeout_callbcak != NULL)
            Timeout_callbcak();
    }
    // 电磁阀超时判断
    if (Valve->Switch.GetRuntime(&Valve->Switch) > timeout && Valve->Switch.state != DEVICE_STATE_IDLE)
    {
        Valve->Switch.state = DEVICE_STATE_TIMEOUT;
    }
}
/*
 * ==========超时处理==========
 */
static void HoolleMotorTimeout_callback(void)
{
#ifndef BEIJING_NXH_VERSION
    EventGroupSetBits(&Mesg_event, MesgEvent_HoolleOutputTimeout);
#endif
}

static void CardMotorTimeout_callback(void)
{
    EventGroupSetBits(&Mesg_event, MesgEvent_CardOutputTimeout);
}
static void LeftValveTimeout_callback(void)
{
    if (Left_Valve.TriggerCount > 0)
    {
        if (Left_Valve.Switch.GetRuntime(&Left_Valve.Switch) > ValveTimeout_time + 300)
        {
            Left_Valve.TriggerCount--;
            Valve_Start(&Left_Valve, Left_Valve.TriggerCount);
        }
        else
        {
            Left_Valve.Switch.state = DEVICE_STATE_TIMEOUT;
        }
    }
}
static void RightValveTimeout_callback(void)
{

    if (Right_Valve.TriggerCount > 0)
    {
        if (Right_Valve.Switch.GetRuntime(&Right_Valve.Switch) > ValveTimeout_time + 300)
        {

            Right_Valve.TriggerCount--;
            Valve_Start(&Right_Valve, Right_Valve.TriggerCount);
        }
        else
        {
            Right_Valve.Switch.state = DEVICE_STATE_TIMEOUT;
        }
    }
}

/*
 * ==========设备初始化==========
 */
void Device_Init(void)
{
    Device_Motor_Init(&Motor_Hoolle1.Motor, &htim1, TIM_CHANNEL_3, &htim1, TIM_CHANNEL_4);
    Device_Switch_Init(&Card.Switch, CardMotor_GPIO_Port, CardMotor_Pin, GPIO_PIN_SET);
    Device_Switch_Init(&Lock_Valve.Switch, LockValve_GPIO_Port, LockValve_Pin, GPIO_PIN_SET);
    Device_Switch_Init(&Valve.Switch, Valve_Ctrl_GPIO_Port, Valve_Ctrl_Pin, GPIO_PIN_SET);
    Device_Switch_Init(&Left_Valve.Switch, Valve_Left_GPIO_Port, Valve_Left_Pin, GPIO_PIN_SET);
    Device_Switch_Init(&Right_Valve.Switch, Valve_Right_GPIO_Port, Valve_Right_Pin, GPIO_PIN_SET);
    HAL_TIM_Base_Start(&htim7);
    Motor_Hoolle1.Hoolle_num = 0;
    Motor_Hoolle1.RetryCount = 0;
    Valve.TriggerCount = 0;
    Left_Valve.TriggerCount = 0;
    Right_Valve.TriggerCount = 0;
    Lock_Valve.TriggerCount = 0;
    Card.Card_num = 0;
}
/*
 * ==========控制任务==========
 */
static void ButtonLight_Ctrl(uint8_t *Position, uint32_t BlinkTime)
{
    static uint32_t time = 0;
    uint8_t state = *Position;
    if (Get_SysTime() - time > BlinkTime)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            if (((state >> i) & 0x01) == 0x01)
                HAL_GPIO_TogglePin(GPIO_Port[i], GPIO_Pin[i]);
            else if (((state >> i) & 0x01) == 0x00)
                HAL_GPIO_WritePin(GPIO_Port[i], GPIO_Pin[i], GPIO_PIN_RESET);
        }
        time = Get_SysTime();
    }
}

void CtrlTask(void)
{
    /*==============吐珠电机控制===============*/
    Ctrl_HoolleMotor(&Motor_Hoolle1, HoolleMotor_Speed, HoolleMotor_Dir, HoolleMotorTimeout_time, HoolleMotorReverse_Time, HoolleMotorRetry_Times, HoolleMotorTimeout_callback);
    /*==============卡片机控制===============*/
    Ctrl_CardMotor(&Card, CardMotorTimeout_time, CardMotorTimeout_callback);
    /*==============电磁阀控制===============*/
    Ctrl_Valve(&Lock_Valve, LockValveTimeout_time, NULL);
    Ctrl_Valve(&Valve, ValveTimeout_time, NULL);
    Ctrl_Valve(&Left_Valve, ValveTimeout_time, LeftValveTimeout_callback);
    Ctrl_Valve(&Right_Valve, ValveTimeout_time, RightValveTimeout_callback);
    ButtonLight_Ctrl(&ButtonLight_Position, 350);
}

/*
 * ==========设备操控==========
 */
void Hoolle_Output(Motor_Hoolle *Motor, uint16_t num)
{
    Motor->Hoolle_num += num;
    if (Motor->Hoolle_num != 0)
    {
        Motor->Motor.state = DEVICE_STATE_START;
        Motor->RetryCount = 0;
        // Motor->Motor.runtick = Get_SysTime();
        Motor->Motor.ResetRuntime(&Motor->Motor);
    }
}

void Card_Output(Motor_Card *Switch, uint16_t num)
{
    Switch->Card_num += num;
    if (num == 0)
        EventGroupSetBits(&Mesg_event, MesgEvent_CardOutputFinish);
    if (Switch->Card_num != 0)
    {
        Switch->Switch.state = DEVICE_STATE_START;
        Switch->Switch.on(&Switch->Switch);
        // Switch->Switch.runtick = Get_SysTime();
        Switch->Switch.ResetRuntime(&Switch->Switch);
    }
}

void Valve_Start(Switch_Valve *Valve, uint8_t TriggerCount)
{
    Valve->Switch.state = DEVICE_STATE_START;
    Valve->Switch.ResetRuntime(&Valve->Switch);
    Valve->TriggerCount = TriggerCount;
}

/*
 * @brief Eject a ball that is still occupying either mini-game hole.
 * @note  Hole sensors are active high. Start only when both valves are idle.
 */
void MiniGameBall_EjectIfOccupied(void)
{
    bool left_occupied = (HAL_GPIO_ReadPin(Hole_Left_GPIO_Port, Hole_Left_Pin) == GPIO_PIN_SET);
    bool right_occupied = (HAL_GPIO_ReadPin(Hole_Right_GPIO_Port, Hole_Right_Pin) == GPIO_PIN_SET);

    if (left_occupied == false && right_occupied == false)
        return;

    if (Left_Valve.TriggerCount == 0 &&
        Right_Valve.TriggerCount == 0 &&
        Left_Valve.Switch.state == DEVICE_STATE_IDLE &&
        Right_Valve.Switch.state == DEVICE_STATE_IDLE)
    {
        Valve_Start(&Left_Valve, Valve_TriggerCount);
        Valve_Start(&Right_Valve, Valve_TriggerCount);
    }
}

void Device_Stop(void)
{
    Motor_Hoolle1.Motor.state = DEVICE_STATE_STOP;
    Card.Switch.state = DEVICE_STATE_STOP;
    Valve.Switch.state = DEVICE_STATE_STOP;
    Left_Valve.Switch.state = DEVICE_STATE_STOP;
    Right_Valve.Switch.state = DEVICE_STATE_STOP;
}