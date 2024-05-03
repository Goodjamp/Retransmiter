#ifndef __KEY_CONTROL_H__
#define __KEY_CONTROL_H__

#include <stdint.h>
#include <stdbool.h>

bool keyControlInit(void);
bool keyControlUpdate(uint16_t videoComutatorSwitch, uint16_t videoChSwitch);

#endif