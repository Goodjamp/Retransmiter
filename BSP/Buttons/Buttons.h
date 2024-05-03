#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include <stdbool.h>

typedef enum {
    BUTTON_MENU,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_ENTER,
    BUTTON_COUNT,
} Button;

void buttonsInit(void); 
bool buttonsGetState(Button button);
void buttonsReadAll(bool state[BUTTON_COUNT]);

#endif