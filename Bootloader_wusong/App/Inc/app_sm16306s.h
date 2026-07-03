#ifndef __APP_SM16306S_H__
#define __APP_SM16306S_H__

#include "main.h"

#define OE_SET() HAL_GPIO_WritePin(SM16306S_OE_GPIO_Port,SM16306S_OE_Pin,GPIO_PIN_SET);
#define OE_RESET() HAL_GPIO_WritePin(SM16306S_OE_GPIO_Port,SM16306S_OE_Pin,GPIO_PIN_RESET);

#define LE_SET() HAL_GPIO_WritePin(SM16306S_LE_GPIO_Port,SM16306S_LE_Pin,GPIO_PIN_SET);
#define LE_RESET() HAL_GPIO_WritePin(SM16306S_LE_GPIO_Port,SM16306S_LE_Pin,GPIO_PIN_RESET);

uint8_t _corl(uint8_t data);
void SM16306S_SetLight(uint8_t *data);

#endif