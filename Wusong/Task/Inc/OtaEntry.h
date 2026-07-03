#ifndef __OTA_ENTRY_H__
#define __OTA_ENTRY_H__

#include "stdint.h"
#include "stdbool.h"

bool OTA_SetBootRequest(void);
void OTA_EnterBootloader(void);
void OTA_EntryReceive(void *self, void *mesg, uint8_t mesg_len);

#endif
