/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, User_LED_Pin|Valve_Left_Pin|Valve_Right_Pin|Button1_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SDCard_CS_Pin|Light1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Button2_LED_Pin|Button3_LED_Pin|LockValve_Pin|SM16306S_LE_Pin
                          |SM16306S_OE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, Light4_Pin|Button4_LED_Pin|Button5_LED_Pin|Valve_Ctrl_Pin
                          |Light3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, Light2_Pin|CardMotor_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Hole_Left_Pin Hole_Right_Pin */
  GPIO_InitStruct.Pin = Hole_Left_Pin|Hole_Right_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : User_LED_Pin Valve_Left_Pin Valve_Right_Pin Button1_LED_Pin */
  GPIO_InitStruct.Pin = User_LED_Pin|Valve_Left_Pin|Valve_Right_Pin|Button1_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : SDCard_CS_Pin */
  GPIO_InitStruct.Pin = SDCard_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SDCard_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Button2_Pin Button3_Pin Button1_Pin */
  GPIO_InitStruct.Pin = Button2_Pin|Button3_Pin|Button1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : Button2_LED_Pin Button3_LED_Pin SM16306S_LE_Pin SM16306S_OE_Pin */
  GPIO_InitStruct.Pin = Button2_LED_Pin|Button3_LED_Pin|SM16306S_LE_Pin|SM16306S_OE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LockValve_Pin */
  GPIO_InitStruct.Pin = LockValve_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LockValve_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Light4_Pin Light3_Pin */
  GPIO_InitStruct.Pin = Light4_Pin|Light3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : Button4_Pin Button5_Pin */
  GPIO_InitStruct.Pin = Button4_Pin|Button5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : Button4_LED_Pin Button5_LED_Pin Valve_Ctrl_Pin */
  GPIO_InitStruct.Pin = Button4_LED_Pin|Button5_LED_Pin|Valve_Ctrl_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : Light2_Pin */
  GPIO_InitStruct.Pin = Light2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(Light2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CardMotor_Pin */
  GPIO_InitStruct.Pin = CardMotor_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CardMotor_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CardFeedback_Pin */
  GPIO_InitStruct.Pin = CardFeedback_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CardFeedback_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : CoinInput_Pin HoolleInput_Pin */
  GPIO_InitStruct.Pin = CoinInput_Pin|HoolleInput_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : HoolleOutput_Pin */
  GPIO_InitStruct.Pin = HoolleOutput_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HoolleOutput_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : KeyBoard2_Pin KeyBoard1_Pin */
  GPIO_InitStruct.Pin = KeyBoard2_Pin|KeyBoard1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : KeyBoard0_Pin */
  GPIO_InitStruct.Pin = KeyBoard0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KeyBoard0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Light1_Pin */
  GPIO_InitStruct.Pin = Light1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(Light1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Switch0_Pin Switch1_Pin Switch2_Pin Switch3_Pin */
  GPIO_InitStruct.Pin = Switch0_Pin|Switch1_Pin|Switch2_Pin|Switch3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : Switch4_Pin Switch5_Pin Switch6_Pin Switch7_Pin
                           Switch8_Pin Switch9_Pin */
  GPIO_InitStruct.Pin = Switch4_Pin|Switch5_Pin|Switch6_Pin|Switch7_Pin
                          |Switch8_Pin|Switch9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Switch10_Pin Switch11_Pin */
  GPIO_InitStruct.Pin = Switch10_Pin|Switch11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
