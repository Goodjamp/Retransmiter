#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

#include "CrsfTask.h"
#include "TasksSettings.h"
#include "ApplicationTask.h"
#include "SystemRtos.h"
#include "services.h"

#include "SEGGER_RTT.h"
#include "DebugServices.h"


#define STR_F_POS(STR, F)                                  ((uint32_t)&(((STR *)0)->F))
#define APPLICATION_TASK_COMMANDS_QUEUE_ITEMS              8
#define APPLICATION_TASK_COMMANDS_NETWORK_MAX_BUFF_SIZE    64
#define APPLICATION_TASK_COMMANDS_SEND_TIMEOUTE            pdMS_TO_TICKS(10)
#define APPLICATION_TASK_MEM_BUFF_MUT_TIMEOUTE             pdMS_TO_TICKS(10)
#define APPLICATION_TASK_CRSF_STATE_UPDATE_PERIOD          pdMS_TO_TICKS(100)
#define APPLICATION_TASK_DEVICE_PING_PERIOD                pdMS_TO_TICKS(400)

typedef enum {
    APPLICATION_TASK_COMMAND_CRSF_TX_TO_NETWORK,
    APPLICATION_TASK_COMMAND_DEVICE_PING,
} ApplicationTaskCommand;

typedef struct {
    ApplicationTaskCommand command;
    union {
        struct {
            uint8_t *data;
            uint32_t size;
        } rxNetwork;
        struct {
            uint16_t videoComutatorSwitch;
            uint32_t videoChSwitch;
        } rxRc;
    };
} ApplicationTaskCommandItem;

static xQueueHandle applicationTaskCommandQueue = NULL;
static TaskHandle_t applicationTaskH;
static SemaphoreHandle_t memBuffMut;
static TimerHandle_t updateCrsfStateTimer = NULL;
static TimerHandle_t devicePingTimer = NULL;
static struct {
    uint8_t buff[APPLICATION_TASK_COMMANDS_NETWORK_MAX_BUFF_SIZE];
    bool isFree;
} memBuff[APPLICATION_TASK_COMMANDS_QUEUE_ITEMS];

void aplicationTaskLogMessage(const char *logStr)
{
#ifdef APPLICATION_TASK_LOG_ENABLE
    SEGGER_RTT_WriteString(0, logStr);
#endif
}

#include "stm32f4xx_ll_gpio.h"
TimerHandle_t ledTimer;

#define LED_PORT            GPIOC
#define LED_PIN             LL_GPIO_PIN_13
#define LED_BLINK_PERIOD    pdMS_TO_TICKS(100)

static void ledTimerCb(TimerHandle_t xTimer)
{
    LL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

static bool initLedIndication(void)
{
    LL_GPIO_InitTypeDef gpioSettings;

    servicesEnablePerephr(LED_PORT);

    gpioSettings.Mode = LL_GPIO_MODE_OUTPUT;
    gpioSettings.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpioSettings.Pin = LED_PIN;
    gpioSettings.Pull = LL_GPIO_PULL_NO;
    gpioSettings.Speed = LL_GPIO_SPEED_FREQ_LOW;
    LL_GPIO_Init(LED_PORT, &gpioSettings);

    ledTimer = xTimerCreate("Led timer",
                            LED_BLINK_PERIOD,
                            pdTRUE, NULL, ledTimerCb);
    if (ledTimer == NULL) {
        return false;
    }
    xTimerStart(ledTimer, 10);

    return true;
}

void memBuffReset(void)
{

    if (systemRtosSemaphoreTake(memBuffMut, APPLICATION_TASK_MEM_BUFF_MUT_TIMEOUTE)
        == pdTRUE) {
        for (uint32_t k = 0; k < APPLICATION_TASK_COMMANDS_QUEUE_ITEMS; k++) {
            memBuff[k].isFree = true;
        }

        systemRtosSemaphoreGive(memBuffMut);
    }
}

uint8_t *memBuffTake(void)
{
    uint8_t *buff = NULL;

    if (systemRtosSemaphoreTake(memBuffMut, APPLICATION_TASK_MEM_BUFF_MUT_TIMEOUTE)
        == pdTRUE) {
        for (uint32_t k = 0; k < APPLICATION_TASK_COMMANDS_QUEUE_ITEMS; k++) {
            if (memBuff[k].isFree == true) {
                buff = memBuff[k].buff;
                break;
            }
        }

        systemRtosSemaphoreGive(memBuffMut);
    }

    return buff;
}

void memBuffGive(uint8_t *buff)
{
    if (systemRtosSemaphoreTake(memBuffMut, APPLICATION_TASK_MEM_BUFF_MUT_TIMEOUTE)
        == pdTRUE) {
        for (uint32_t k = 0; k < APPLICATION_TASK_COMMANDS_QUEUE_ITEMS; k++) {
            if (buff == memBuff[k].buff) {
                memBuff[k].isFree = true;
                break;
            }
        }

        systemRtosSemaphoreGive(memBuffMut);
    }
}

static void crsfTxTimerCb(TimerHandle_t xTimer)
{
    ApplicationTaskCommandItem comamndItem = {APPLICATION_TASK_COMMAND_CRSF_TX_TO_NETWORK};

    if(systemRtosQueueSend(applicationTaskCommandQueue,
                           &comamndItem, APPLICATION_TASK_COMMANDS_SEND_TIMEOUTE)
       == pdFALSE) {
        aplicationTaskLogMessage("ERROR: App Task send command\n");
    }
}

static void dvicePingTimerCb(TimerHandle_t xTimer)
{
    ApplicationTaskCommandItem comamndItem = {APPLICATION_TASK_COMMAND_DEVICE_PING};

    if(systemRtosQueueSend(applicationTaskCommandQueue,
                           &comamndItem, APPLICATION_TASK_COMMANDS_SEND_TIMEOUTE)
        == pdFALSE) {
        aplicationTaskLogMessage("ERROR: App Task send command\n");
    }
}

static void applicationTask(void *userData)
{
    ApplicationTaskCommandItem commandItem;

    xTimerStart(updateCrsfStateTimer, 10);
    xTimerStart(devicePingTimer, 10);

    while (true) {
        if (systemRtosQueueReceive(applicationTaskCommandQueue, &commandItem, portMAX_DELAY)
            == pdFALSE) {
            continue;
        }

        switch (commandItem.command) {

        default:
            aplicationTaskLogMessage("ERROR: App task wrong command\n");
        }
    }
}

bool applicationTaskInit(void)
{
    applicationTaskCommandQueue = xQueueCreate(APPLICATION_TASK_COMMANDS_QUEUE_ITEMS, sizeof(ApplicationTaskCommandItem));
    if (applicationTaskCommandQueue == NULL) {
        return false;
    }

    memBuffMut = xSemaphoreCreateMutex();
    if (memBuffMut == NULL) {
        return false;
    }

    updateCrsfStateTimer = xTimerCreate("Update Crsf state",
                                        pdMS_TO_TICKS(APPLICATION_TASK_CRSF_STATE_UPDATE_PERIOD),
                                        pdTRUE, NULL, crsfTxTimerCb);
    if (updateCrsfStateTimer == NULL) {
        return false;
    }

    devicePingTimer = xTimerCreate("Device ping timer",
                                   APPLICATION_TASK_DEVICE_PING_PERIOD,
                                   pdTRUE, NULL, dvicePingTimerCb);
    if (updateCrsfStateTimer == NULL) {
        return false;
    }

    memBuffReset();

    initLedIndication();
    
    if (xTaskCreate(applicationTask,
                       TASK_APPLICATION_ASCII_NAME,
                       TASK_APPLICATION_STACK_SIZE,
                       NULL,
                       TASK_APPLICATION_PRIORITY,
                       &applicationTaskH) != pdPASS) {
        return false;
    }

    return true;
}
