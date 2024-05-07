#ifndef __BUTTON_PROCESSING_H__
#define __BUTTON_PROCESSING_H__

#include <stdint.h>
#include <stdbool.h>

bool buttonInit(void);
void buttonUpdate(uint8_t switcherState1, uint8_t buttonState1, uint8_t buttonState2);

#endif