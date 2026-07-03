#ifndef __APP_VERSION_H__
#define __APP_VERSION_H__

#include "stdint.h"

/*
 * APP版本编码：0xMMmmppbb
 * MM：主版本，mm：次版本，pp：修订版本，bb：预留版本。
 * 当前版本：V1.0.0。
 */
#define APP_VERSION_CODE       0x01000000U
#define APP_VERSION_MAJOR      ((uint8_t)((APP_VERSION_CODE >> 24) & 0xFFU))
#define APP_VERSION_MINOR      ((uint8_t)((APP_VERSION_CODE >> 16) & 0xFFU))
#define APP_VERSION_PATCH      ((uint8_t)((APP_VERSION_CODE >> 8) & 0xFFU))
#define APP_VERSION_RESERVED   ((uint8_t)(APP_VERSION_CODE & 0xFFU))

#endif
