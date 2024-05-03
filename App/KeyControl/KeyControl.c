#include <stdint.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "stm32f4xx_ll_gpio.h"

#include "DebugServices.h"
#include "services.h"
#include "CrsfTask.h"
#include "BSP.h"

#define UP_TIME(X)    (xTaskGetTickCount() - X)

#define KEY_CONTROL_GISTERESIS_MS             100
#define KEY_CONTROL_UP_PERIOD                 5
#define KEY_CONTROL_START_TIMER_BLOCK_TIME    10

typedef enum {
    KEY_CONTROL_VIDEO_CH_58,
    KEY_CONTROL_VIDEO_SOURCE,
    KEY_CONTROL_VIDEO_CH_12,
} KeyControlState;

typedef enum {
   KEY_CONTROL_BUTTON_PRESS,
   KEY_CONTROL_BUTTON_RELEASE,
} KeyControlButtonState;

typedef struct {
    GPIO_TypeDef *gpio;
    uint32_t pin;
    void (*keycontrolCb)(GPIO_TypeDef *GPIOx, uint32_t PinMask);
    bool needresetPin;
    uint32_t gistStartTime;
    TimerHandle_t timer;
    bool prevPress;
} KeyControlItem;
KeyControlItem keyControlList[] = {
    [KEY_CONTROL_VIDEO_CH_58] = {KEY_CONTROL_VIDEO_CH_58_PORT, KEY_CONTROL_VIDEO_CH_58_PIN, LL_GPIO_SetOutputPin, true},
    [KEY_CONTROL_VIDEO_CH_12] = {KEY_CONTROL_VIDEO_CH_12_PORT, KEY_CONTROL_VIDEO_CH_12_PIN, LL_GPIO_SetOutputPin, true},
    [KEY_CONTROL_VIDEO_SOURCE] = {KEY_CONTROL_VIDEO_SOURCE_PORT, KEY_CONTROL_VIDEO_SOURCE_PIN, LL_GPIO_TogglePin, false},
};

static void keyPeriodDone(TimerHandle_t xTimer)
{
    KeyControlItem *item = (KeyControlItem *)pvTimerGetTimerID(xTimer);
    LL_GPIO_ResetOutputPin(item->gpio, item->pin);
}

bool keyControlInit(void)
{
    LL_GPIO_InitTypeDef gpioSettings;

    for (uint32_t k = 0 ; k < sizeof(keyControlList) / sizeof(keyControlList[0]); k++) {
        gpioSettings.Mode = LL_GPIO_MODE_OUTPUT;
        gpioSettings.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
        gpioSettings.Pin = keyControlList[k].pin;
        gpioSettings.Pull = LL_GPIO_PULL_NO;
        gpioSettings.Speed = LL_GPIO_SPEED_FREQ_LOW;
        LL_GPIO_Init(keyControlList[k].gpio, &gpioSettings);

        /*
         * Init timer for count UP period
         */
        keyControlList[k].timer = xTimerCreate("Key UP period counter",
                                               pdMS_TO_TICKS(KEY_CONTROL_UP_PERIOD),
                                               pdFALSE, &keyControlList[k], keyPeriodDone);
        if (keyControlList[k].timer == NULL) {
            return false;
        }
    }

    return true;
}

static void keyControl(KeyControlItem *item, KeyControlButtonState buttonState)
{
    if (buttonState == KEY_CONTROL_BUTTON_PRESS) {
        if (item->prevPress == false) {
            if (UP_TIME(item->gistStartTime) > KEY_CONTROL_GISTERESIS_MS) {
                item->prevPress = true;
                item->keycontrolCb(item->gpio, item->pin);
                if (item->needresetPin == true) {
                    xTimerStart(item->timer, KEY_CONTROL_START_TIMER_BLOCK_TIME);
                }
            }
        }
    } else {
        item->prevPress = false;
    }
}

bool keyControlUpdate(uint8_t comutatorSwitch, uint8_t videoChSwitch)
{
    bool result = true;


    if (videoChSwitch == KEY_CONTROL_BUTTON_PRESS) {
        if (keyControlList[comutatorSwitch].prevPress == false) {
            if (UP_TIME(keyControlList[comutatorSwitch].gistStartTime) > KEY_CONTROL_GISTERESIS_MS) {
                keyControlList[comutatorSwitch].prevPress = true;
                switch (comutatorSwitch) {
                case KEY_CONTROL_VIDEO_CH_58:
                case KEY_CONTROL_VIDEO_CH_12:
                    SEGGER_RTT_WriteString(0, "But press\n");
                    LL_GPIO_SetOutputPin(keyControlList[comutatorSwitch].gpio, keyControlList[comutatorSwitch].pin);
                    break;

                case KEY_CONTROL_VIDEO_SOURCE:
                    SEGGER_RTT_WriteString(0, "But toogle\n");
                    LL_GPIO_TogglePin(keyControlList[comutatorSwitch].gpio, keyControlList[comutatorSwitch].pin);
                    break;

                default:
                    result = false;
                }
                xTimerStart(keyControlList[comutatorSwitch].timer, KEY_CONTROL_START_TIMER_BLOCK_TIME);
            }
        }
    } else {
        keyControlList[comutatorSwitch].prevPress = false;
    }

    return result;
}