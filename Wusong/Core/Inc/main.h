/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Hole_Left_Pin GPIO_PIN_2
#define Hole_Left_GPIO_Port GPIOE
#define Hole_Left_EXTI_IRQn EXTI2_IRQn
#define Hole_Right_Pin GPIO_PIN_3
#define Hole_Right_GPIO_Port GPIOE
#define Hole_Right_EXTI_IRQn EXTI3_IRQn
#define User_LED_Pin GPIO_PIN_13
#define User_LED_GPIO_Port GPIOC
#define Valve_Left_Pin GPIO_PIN_2
#define Valve_Left_GPIO_Port GPIOC
#define Valve_Right_Pin GPIO_PIN_3
#define Valve_Right_GPIO_Port GPIOC
#define SDCard_CS_Pin GPIO_PIN_4
#define SDCard_CS_GPIO_Port GPIOA
#define SD_CLK_Pin GPIO_PIN_5
#define SD_CLK_GPIO_Port GPIOA
#define SD_MISO_Pin GPIO_PIN_6
#define SD_MISO_GPIO_Port GPIOA
#define SD_MOSI_Pin GPIO_PIN_7
#define SD_MOSI_GPIO_Port GPIOA
#define Button2_Pin GPIO_PIN_4
#define Button2_GPIO_Port GPIOC
#define Button2_EXTI_IRQn EXTI4_IRQn
#define Button3_Pin GPIO_PIN_5
#define Button3_GPIO_Port GPIOC
#define Button3_EXTI_IRQn EXTI9_5_IRQn
#define Button2_LED_Pin GPIO_PIN_0
#define Button2_LED_GPIO_Port GPIOB
#define Button3_LED_Pin GPIO_PIN_1
#define Button3_LED_GPIO_Port GPIOB
#define LockValve_Pin GPIO_PIN_2
#define LockValve_GPIO_Port GPIOB
#define Light4_Pin GPIO_PIN_7
#define Light4_GPIO_Port GPIOE
#define Button4_Pin GPIO_PIN_8
#define Button4_GPIO_Port GPIOE
#define Button4_EXTI_IRQn EXTI9_5_IRQn
#define Button5_Pin GPIO_PIN_9
#define Button5_GPIO_Port GPIOE
#define Button5_EXTI_IRQn EXTI9_5_IRQn
#define Button4_LED_Pin GPIO_PIN_10
#define Button4_LED_GPIO_Port GPIOE
#define Button5_LED_Pin GPIO_PIN_11
#define Button5_LED_GPIO_Port GPIOE
#define Valve_Ctrl_Pin GPIO_PIN_12
#define Valve_Ctrl_GPIO_Port GPIOE
#define HoolleMotorA_Pin GPIO_PIN_13
#define HoolleMotorA_GPIO_Port GPIOE
#define HoolleMotorB_Pin GPIO_PIN_14
#define HoolleMotorB_GPIO_Port GPIOE
#define Light3_Pin GPIO_PIN_15
#define Light3_GPIO_Port GPIOE
#define SM16306S_LE_Pin GPIO_PIN_12
#define SM16306S_LE_GPIO_Port GPIOB
#define SM16306S_SCK_Pin GPIO_PIN_13
#define SM16306S_SCK_GPIO_Port GPIOB
#define SM16306S_OE_Pin GPIO_PIN_14
#define SM16306S_OE_GPIO_Port GPIOB
#define SM16306S_MOSI_Pin GPIO_PIN_15
#define SM16306S_MOSI_GPIO_Port GPIOB
#define Light2_Pin GPIO_PIN_8
#define Light2_GPIO_Port GPIOD
#define CardMotor_Pin GPIO_PIN_9
#define CardMotor_GPIO_Port GPIOD
#define CardFeedback_Pin GPIO_PIN_10
#define CardFeedback_GPIO_Port GPIOD
#define CardFeedback_EXTI_IRQn EXTI15_10_IRQn
#define CoinInput_Pin GPIO_PIN_11
#define CoinInput_GPIO_Port GPIOD
#define CoinInput_EXTI_IRQn EXTI15_10_IRQn
#define HoolleOutput_Pin GPIO_PIN_12
#define HoolleOutput_GPIO_Port GPIOD
#define HoolleOutput_EXTI_IRQn EXTI15_10_IRQn
#define HoolleInput_Pin GPIO_PIN_13
#define HoolleInput_GPIO_Port GPIOD
#define HoolleInput_EXTI_IRQn EXTI15_10_IRQn
#define KeyBoard2_Pin GPIO_PIN_14
#define KeyBoard2_GPIO_Port GPIOD
#define KeyBoard2_EXTI_IRQn EXTI15_10_IRQn
#define KeyBoard1_Pin GPIO_PIN_15
#define KeyBoard1_GPIO_Port GPIOD
#define KeyBoard1_EXTI_IRQn EXTI15_10_IRQn
#define KeyBoard0_Pin GPIO_PIN_6
#define KeyBoard0_GPIO_Port GPIOC
#define KeyBoard0_EXTI_IRQn EXTI9_5_IRQn
#define Button1_Pin GPIO_PIN_7
#define Button1_GPIO_Port GPIOC
#define Button1_EXTI_IRQn EXTI9_5_IRQn
#define Button1_LED_Pin GPIO_PIN_8
#define Button1_LED_GPIO_Port GPIOC
#define LED_PWM_Pin GPIO_PIN_9
#define LED_PWM_GPIO_Port GPIOC
#define Light1_Pin GPIO_PIN_8
#define Light1_GPIO_Port GPIOA
#define Switch0_Pin GPIO_PIN_0
#define Switch0_GPIO_Port GPIOD
#define Switch1_Pin GPIO_PIN_1
#define Switch1_GPIO_Port GPIOD
#define Switch2_Pin GPIO_PIN_3
#define Switch2_GPIO_Port GPIOD
#define Switch3_Pin GPIO_PIN_4
#define Switch3_GPIO_Port GPIOD
#define Switch4_Pin GPIO_PIN_4
#define Switch4_GPIO_Port GPIOB
#define Switch5_Pin GPIO_PIN_5
#define Switch5_GPIO_Port GPIOB
#define Switch6_Pin GPIO_PIN_6
#define Switch6_GPIO_Port GPIOB
#define Switch7_Pin GPIO_PIN_7
#define Switch7_GPIO_Port GPIOB
#define Switch8_Pin GPIO_PIN_8
#define Switch8_GPIO_Port GPIOB
#define Switch9_Pin GPIO_PIN_9
#define Switch9_GPIO_Port GPIOB
#define Switch10_Pin GPIO_PIN_0
#define Switch10_GPIO_Port GPIOE
#define Switch11_Pin GPIO_PIN_1
#define Switch11_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
