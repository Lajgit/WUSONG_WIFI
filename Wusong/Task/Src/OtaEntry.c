#include "OtaEntry.h"

/*
 * 升级入口已并入原USART1固定7字节协议：
 * AA F0 42 4F CRC16_H CRC16_L 55。
 * 实际接收和CRC16校验继续使用CommTask原有通用流程。
 */
