#include "FreeRTOS.h"
#include "semphr.h"

#include "TasksSettings.h"
#include "SystemRtos.h"
#include "Buttons.h"

static TaskHandle_t buttonsTaskH;
static bool buttonsState[BUTTON_COUNT];

static void buttonsTask(void *userData)
{
    /* Read initial buttons state */
    buttonsReadAll(buttonsState);

    while (1) {
        vTaskDelay(10);

        bool newButtonsState[BUTTON_COUNT];
        buttonsReadAll(newButtonsState);

        for (int i = 0; i < BUTTON_COUNT; i++) {
            if (newButtonsState[i] != buttonsState[i]) {
                /* State changed */
                if (newButtonsState[i] == false) {
                    /* User released button, send action to display task */

                }

                buttonsState[i] = newButtonsState[i];
            }
        }
    }
}

int buttonsTaskInit(void)
{
    buttonsInit();

    if (xTaskCreate(buttonsTask,
                       TASK_BUTTONS_ASCII_NAME,
                       TASK_BUTTONS_STACK_SIZE,
                       NULL,
                       TASK_BUTTONS_PRIORITY,
                       &buttonsTaskH) != pdPASS) {
        return 1; // fail
    }

    return 0;
}
