#ifndef __LIGHTTASK_H__
#define __LIGHTTASK_H__

#include "port_lighteffect.h"

#define Light_RGBbuffer_SIZE 159
#define Light_CRRbuffer_SIZE ((Light_RGBbuffer_SIZE + 7) * 24)

void Light_Init(void);
void Light_Task(void);

#endif
