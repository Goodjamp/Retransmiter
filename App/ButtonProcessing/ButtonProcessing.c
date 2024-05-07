#include <stdint.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "stm32f4xx_ll_gpio.h"

#include "DebugServices.h"
#include "services.h"
#include "CrsfTask.h"
#include "BSP.h"
#include "PlatformControlTask.h"

#define UP_TIME(X)                    (xTaskGetTickCount() - X)
#define BUTTON_GISTERESIS_MS          pdMS_TO_TICKS(110)

typedef enum {
    SWITCHER_STATE_CH_58_RX,
    SWITCHER_STATE_CH_13_RX,
    SWITCHER_STATE_CH_58_TX,
} SwitcherState;
SwitcherState switcherState = SWITCHER_STATE_CH_58_RX;

#define KEY_HIGHT_PERIOD              pdMS_TO_TICKS(100)
#define KEY_START_TIMER_BLOCK_TIME    pdMS_TO_TICKS(10)

typedef enum {
    KEY_SWITCH_CH_58_RX,
    KEY_SWITCH_CH_13_RX,
    KEY_SWITCH_CH_58_TX,
    KEY_SET_VIDEO_SOURCE_58,
    KEY_SET_VIDEO_SOURCE_13,
} KeyAction;

typedef struct {
    GPIO_TypeDef *port;
    uint32_t pin;
    void (*cb)(GPIO_TypeDef *GPIOx, uint32_t PinMask);
    bool needresetPin;
    TimerHandle_t timer;
} KeyItem;

static void setPin(GPIO_TypeDef *gpio, uint32_t pin);
static void clearPin(GPIO_TypeDef *gpio, uint32_t pin);

KeyItem keyList[] = {
    [KEY_SWITCH_CH_58_RX] = {KEY_CONTROL_VIDEO_CH_58_RX_PORT, KEY_CONTROL_VIDEO_CH_58_RX_PIN, setPin, true},
    [KEY_SWITCH_CH_13_RX] = {KEY_CONTROL_VIDEO_CH_13_RX_PORT, KEY_CONTROL_VIDEO_CH_13_RX_PIN, setPin, true},
    [KEY_SWITCH_CH_58_TX] = {KEY_CONTROL_VIDEO_CH_58_TX_PORT, KEY_CONTROL_VIDEO_CH_58_TX_PIN, setPin, true},
    [KEY_SET_VIDEO_SOURCE_58] = {KEY_CONTROL_VIDEO_SOURCE_PORT, KEY_CONTROL_VIDEO_SOURCE_PIN, setPin, false},
    [KEY_SET_VIDEO_SOURCE_13] = {KEY_CONTROL_VIDEO_SOURCE_PORT, KEY_CONTROL_VIDEO_SOURCE_PIN, clearPin, false},
};

static void setPin(GPIO_TypeDef *gpio, uint32_t pin)
{
    LL_GPIO_SetOutputPin(gpio, pin);
}

static void clearPin(GPIO_TypeDef *gpio, uint32_t pin)
{
    LL_GPIO_ResetOutputPin(gpio, pin);
}

void keyAction(KeyAction action)
{
    keyList[action].cb(keyList[action].port, keyList[action].pin);
    if (keyList[action].needresetPin) {
        xTimerStart(keyList[action].timer, KEY_START_TIMER_BLOCK_TIME);
    }
}

static void pinHightStateTime(TimerHandle_t xTimer)
{
    KeyItem *item = (KeyItem *)pvTimerGetTimerID(xTimer);
    LL_GPIO_ResetOutputPin(item->port, item->pin);
}

static bool keiInit(void)
{
    LL_GPIO_InitTypeDef gpioSettings;

    for (uint32_t k = 0 ; k < sizeof(keyList) / sizeof(keyList[0]); k++) {
        servicesEnablePerephr(keyList[k].port);

        gpioSettings.Mode = LL_GPIO_MODE_OUTPUT;
        gpioSettings.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpioSettings.Pin = keyList[k].pin;
        gpioSettings.Pull = LL_GPIO_PULL_NO;
        gpioSettings.Speed = LL_GPIO_SPEED_FREQ_LOW;
        LL_GPIO_Init(keyList[k].port, &gpioSettings);

        /*
         * Init timer for count hight pin state time
         */
        keyList[k].timer = xTimerCreate("Pin hight state counter",
                                        pdMS_TO_TICKS(KEY_HIGHT_PERIOD),
                                        pdFALSE, &keyList[k], pinHightStateTime);
        if (keyList[k].timer == NULL) {
            return false;
        }
    }

    return true;
}

bool buttonInit(void)
{
    if (keiInit() == false) {
        return false;
    };

    return true;
}

void buttonUpdate(uint8_t switcherState1, uint8_t buttonState1, uint8_t buttonState2)
{
    static TickType_t buttonGist1 = 0;
    static uint8_t buttonPrevState1 = 255;
    static TickType_t buttonGist2 = 0;
    static uint8_t buttonPrevState2 = 255;
    static uint8_t switcherPrevState1 = 255;

    if (switcherPrevState1 != switcherState1) {
        switcherPrevState1 = switcherState1;
        switch (switcherState1) {
        case 0:
            SEGGER_RTT_WriteString(0, "Set SWITCHER_STATE_CH_58_RX\n");
            keyAction(KEY_SET_VIDEO_SOURCE_58);
            switcherState = SWITCHER_STATE_CH_58_RX;
            break;

        case 1:
            SEGGER_RTT_WriteString(0, "Set SWITCHER_STATE_CH_58_TX\n");
            switcherState = SWITCHER_STATE_CH_58_TX;
            break;

        case 2:
            SEGGER_RTT_WriteString(0, "Set SWITCHER_STATE_CH_13_RX\n");
            keyAction(KEY_SET_VIDEO_SOURCE_13);
            switcherState = SWITCHER_STATE_CH_13_RX;
            break;
        }
    }

    if (buttonPrevState1 != buttonState1) {
        buttonPrevState1 = buttonState1;
        if (buttonState1 == 1) {
            if (UP_TIME(buttonGist1) > BUTTON_GISTERESIS_MS) {
                buttonGist1 = xTaskGetTickCount();
                switch (switcherState) {
                case SWITCHER_STATE_CH_58_RX:
                    SEGGER_RTT_WriteString(0, "Switch CH_58_RX\n");
                    keyAction(KEY_SWITCH_CH_58_RX);
                    break;

                case SWITCHER_STATE_CH_13_RX:
                    SEGGER_RTT_WriteString(0, "Switch CH_13_RX\n");
                    keyAction(KEY_SWITCH_CH_13_RX);
                    break;

                case SWITCHER_STATE_CH_58_TX:
                    SEGGER_RTT_WriteString(0, "Switch CH_58_TX\n");
                    keyAction(KEY_SWITCH_CH_58_TX);
                    break;
                }
            }
        }
    }

    if (buttonPrevState2 != buttonState2) {
        buttonPrevState2 = buttonState2;
        if (buttonState2 == 1) {
            if (UP_TIME(buttonGist2) > BUTTON_GISTERESIS_MS) {
                buttonGist2 = xTaskGetTickCount();
                platformControlUpdateState();
            }
        }
    }
}
