#include "Buttons.h"
#include "BSP.h"
#include "stm32f4xx_ll_gpio.h"
#include "services.h"

typedef struct {
    uint32_t pin;
    GPIO_TypeDef *port;
} GpioConfig;

static const GpioConfig buttonsPins[BUTTON_COUNT] = {
    [BUTTON_MENU] = {
        .port = BUTTONS_MENU_PORT,
        .pin = BUTTONS_MENU_PIN
    },
    [BUTTON_UP] = {
        .port = BUTTONS_UP_PORT,
        .pin = BUTTONS_UP_PIN
    },
    [BUTTON_DOWN] = {
        .port = BUTTONS_DOWN_PORT,
        .pin = BUTTONS_DOWN_PIN
    },
    [BUTTON_ENTER] = {
        .port = BUTTONS_ENTER_PORT,
        .pin = BUTTONS_ENTER_PIN
    },
};

void buttonsInit(void)
{
    LL_GPIO_InitTypeDef gpioInit = {
        .Mode = LL_GPIO_MODE_INPUT,
        .OutputType = LL_GPIO_OUTPUT_PUSHPULL,
        .Pull = LL_GPIO_PULL_UP,
        .Speed = LL_GPIO_SPEED_FREQ_LOW,
    };

    for (int i = 0; i < BUTTON_COUNT; i++) {
        gpioInit.Pin = buttonsPins[i].pin;

        servicesEnablePerephr(buttonsPins[i].port);
        LL_GPIO_Init(buttonsPins[i].port, &gpioInit);
    }
}

bool buttonsGetState(Button button)
{
    return LL_GPIO_IsInputPinSet(buttonsPins[button].port, buttonsPins[button].pin);
}

void buttonsReadAll(bool state[BUTTON_COUNT])
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        state[i] = buttonsGetState(i);
    }
}