#ifndef __BUTTON_PROCESSING_H__
#define __BUTTON_PROCESSING_H__

#include <stdint.h>
#include <stdbool.h>

bool buttonInit(void);
bool buttonUpdate(uint16_t videoComutatorSwitch, uint16_t videoChSwitch);

#endif