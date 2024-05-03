#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

#include "TasksSettings.h"
#include "SystemRtos.h"
#include "DebugServices.h"
#include "Usart.h"
#include "CrsfTask.h"
#include "Crsf.h"
#include "CrsfFramesCache.h"
#include "KeyControl.h"



#define CRSF_TASK_RC_MAX                         1800

#define CRSF_TASK_RC_CH_KEY_TRIGER               5
#define CRSF_TASK_RC_CH_KEY                      6

#define CRSF_TASK_COMMANDS_QUEUE_ITEMS           4

/*
 * The obsetete timeouts list
 */
#define LINK_STATISTICS_OBSOLETE_TIMEOUTE        pdMS_TO_TICKS(1000)
#define RADIO_ID_OBSOLETE_TIMEOUTE               pdMS_TO_TICKS(1000)
#define RC_CHANNEL_OBSOLETE_TIMEOUTE             pdMS_TO_TICKS(1000)

/*
 * The maximum number of serial corrupted packet to switch communication
 * to the adjust state
 */
#define BEGIN_SWITCH_BR_CORRUPT_PACKET_NUMBER    50
#define SWITCH_BR_CORRUPT_PACKET_NUMBER          30
#define STORE_BR_CORRUPT_PACKET_NUMBER           10
#define DATA_MIRROR_MUTEX_TIMEOUTE               pdMS_TO_TICKS(100)
#define UPDATE_TRANSMITER_STATE_PERIOD           pdMS_TO_TICKS(200)
#define CRSF_TASK_QUEUE_SEND_COMMAND_TIMEOUTE    pdMS_TO_TICKS(10)

#define IS_BIT_SET(VALUE, BIT_POS)               ((VALUE) & (1 << (BIT_POS)))
#define BIT_CLEAR(VALUE, BIT_POS)                ((VALUE) &= ~(1 << (BIT_POS)))
#define BIT_SET(VALUE, BIT_POS)                  ((VALUE) |= (1 << (BIT_POS)))
#define LINK_STATISTICS_BIT_POS                  0
#define RADIO_ID_BIT_POS                         1
#define RC_CHANNEL_BIT_POS                       2


typedef enum {
    CRSF_TASK_RX_CRSF_FRAME,
    CRSF_TASK_COMMAND_RX_TIMEOUT,

    /*
     * Notify the task that we need to transmit to the RC current state of the transmitter
     * and fly controller (telemetry information)
     */
    CRSF_TASK_UPDATE_TRANSMITER_STATE,
} CrsfTaskCommandId;

static const char *brLisStr[] = {"115200", "400000", "921600", "1870000", "3750000", "5250000"};
static const uint32_t brList[] = { 115200, 410000, 921600, 1870000, 3750000, 5250000 };
typedef struct {
    CrsfTaskCommandId command;
    union {
        struct {
            uint8_t *data;
            uint32_t size;
        } rxFrame;
    };
} CrsfTaskCommandItem;
static struct {
    uint32_t cnt;
    uint32_t brIndex;
} communicaiotnState;
static SemaphoreHandle_t txCompleteSem = NULL;
static SemaphoreHandle_t crsfFrameCacheMut = NULL;
static QueueHandle_t crsfTaskCommandQueue = NULL;
static TaskHandle_t xTaskCreateH;
//static TimerHandle_t sendTransmiterStateTimer = NULL;
static CrsfH crsfProto;
static bool isCrsfFrameRx = false;
static CrsfFrameCacheH crsfFrameCache;

void crsfTaskLogMessage(const char *logStr)
{
#ifdef CRSF_TASK_LOG_ENABLE
    SEGGER_RTT_WriteString(0, logStr);
#endif
}

static void usartTxCompleteCb(void)
{
    systemRtosSemaphoreGive(txCompleteSem);
}

static void usartRxCb(uint8_t *buff, uint32_t size)
{
    CrsfTaskCommandItem commandItem = {
        .command = CRSF_TASK_RX_CRSF_FRAME,
        .rxFrame = {buff, size}
    };

    systemRtosQueueSend(crsfTaskCommandQueue,
                        &commandItem, 0);
}

static uint8_t crsfTaskDecode3StateSw(uint16_t value)
{
    if (value <= CRSF_TASK_RC_MAX / 4) {
        return 0;
    } else if (value > CRSF_TASK_RC_MAX / 4 && value <= (3 * CRSF_TASK_RC_MAX) / 4) {
        return 1;
    } else {
        return 2;
    }
}

static uint8_t crsfTaskDecode2StateSw(uint16_t value)
{
    return (value >= CRSF_TASK_RC_MAX / 2 ) ? 0 : 1;
}

static void rxCrsfFrameCb(CrsfAddress address, CrsfFrame *frame)
{
    isCrsfFrameRx = true;

    switch (frame->type) {
    case CRSF_RC_CHANNELS_PACKED:
        crsfFrameCachePush(&crsfFrameCache, address, CRSF_RC_CHANNELS_PACKED,
                           &frame->payload.rcChannelsPacked);

        keyControlUpdate(crsfTaskDecode3StateSw(frame->payload.rcChannelsPacked.chValue[CRSF_TASK_RC_CH_KEY_TRIGER]),
                         crsfTaskDecode2StateSw(frame->payload.rcChannelsPacked.chValue[CRSF_TASK_RC_CH_KEY]));
        break;

    default:
        break;
    }
}

/*
 * The monitoring of communication over the USART (CRSF). Is there are no receive data during timoute
 * - restart BR detectin.
 */
static void updateCommunicaiotnState(bool isCrsfFrameRx)
{
    UsartResult usartResult;

    if (isCrsfFrameRx == true) {
        if (communicaiotnState.cnt > 0) {
            if (communicaiotnState.cnt-- == STORE_BR_CORRUPT_PACKET_NUMBER) {
                /*
                 * Send notificaiotn to the Upper layer
                 * about BR to write new BR to the flash
                 */
                SEGGER_RTT_WriteString(0, "Write br ");
                SEGGER_RTT_WriteString(0, brLisStr[communicaiotnState.brIndex]);
                SEGGER_RTT_WriteString(0, "\n");
            }
        }
    } else {
        if (++communicaiotnState.cnt >= BEGIN_SWITCH_BR_CORRUPT_PACKET_NUMBER) {
            communicaiotnState.cnt = SWITCH_BR_CORRUPT_PACKET_NUMBER;

            if (++communicaiotnState.brIndex >= sizeof(brList) / sizeof(brList[0])) {
                communicaiotnState.brIndex = 0;
            }
            usartResult = usartSetBr(USART_TARGET_CRSF,
                                     brList[communicaiotnState.brIndex]);
            if (usartResult != USART_OK) {
                /*
                * log or print the error message
                */
            }
        }
    }
}

/*
static void updateTransmiterStateTimerCb(TimerHandle_t xTimer)
{
    CrsfTaskCommandItem commandItem = {CRSF_TASK_UPDATE_TRANSMITER_STATE};

    if(systemRtosQueueSend(crsfTaskCommandQueue,
                           &commandItem, CRSF_TASK_QUEUE_SEND_COMMAND_TIMEOUTE)
       == pdFALSE) {
        crsfTaskLogMessage("ERROR: crsf send command\n");
    }
}
*/

static void sendCrsfFramesToRc(uint32_t *updateFrameBf)
{
    static uint8_t crsfFrameBuff[CSFR_MAX_PACKET_SIZE];
    CrsfFrameCacheResult result;
    CrsfAddress address;
    uint32_t timeStamp;
    uint32_t crsfFrameSize;
    bool sendPacket = false;

    if (*updateFrameBf == 0) {
        return;
    }

    crsfFrameSize = sizeof(crsfFrameBuff);
    if (IS_BIT_SET(*updateFrameBf, LINK_STATISTICS_BIT_POS)) {
        CrsfLinkStatistic payload;

        BIT_CLEAR(*updateFrameBf, LINK_STATISTICS_BIT_POS);
        result = crsfFrameCachePop(&crsfFrameCache, &address,
                                   &timeStamp, CRSF_LINK_STATISTICS, &payload);
        if ((result == CRSF_FRAME_CACHE_OK)
            && ((xTaskGetTickCount() - timeStamp) <= LINK_STATISTICS_OBSOLETE_TIMEOUTE )) {
            crsfSerialiaseLinkStatistics(&crsfProto, crsfFrameBuff, &crsfFrameSize,
                                         address, &payload);
            sendPacket = true;
        }
    } else if (IS_BIT_SET(*updateFrameBf, RADIO_ID_BIT_POS)) {
        BIT_CLEAR(*updateFrameBf, RADIO_ID_BIT_POS);
    } else if (IS_BIT_SET(*updateFrameBf, RC_CHANNEL_BIT_POS)) {
        /*
         * Make this transfer only for the test purposes
         */
        CrsfRcChannelsPacked payload;

        BIT_CLEAR(*updateFrameBf, RC_CHANNEL_BIT_POS);
        result = crsfFrameCachePop(&crsfFrameCache, &address,
                                    &timeStamp, CRSF_RC_CHANNELS_PACKED, &payload);
        if ((result == CRSF_FRAME_CACHE_OK)
            && ((xTaskGetTickCount() - timeStamp) <= RC_CHANNEL_OBSOLETE_TIMEOUTE)) {
            crsfSerialiaseRcChannelsPacked(&crsfProto, crsfFrameBuff, &crsfFrameSize,
                                            address, &payload);
            sendPacket = true;
        }
    }

    if (sendPacket == true) {
#warning The next timeout must be clarified in the future. Maybe it will be better to use a HW timer.
        vTaskDelay(1);
        usartTx(USART_TARGET_CRSF, crsfFrameBuff, crsfFrameSize);
    } else {
        crsfTaskLogMessage("ERROR: Transmiter emulator get cache\n");
    }
}

static void crsfTask(void *userData)
{
    CrsfTaskCommandItem commandItem;
    uint32_t updateFrameBitField = 0;

    //xTimerStart(sendTransmiterStateTimer, 10);

    while (1) {
        xQueueReceive(crsfTaskCommandQueue,
                      &commandItem, portMAX_DELAY);

        switch (commandItem.command) {
        case CRSF_TASK_RX_CRSF_FRAME:
            /*
             * Try deserialiase CRSF frame.
             */
            isCrsfFrameRx = false;
            crsfDeserialiase(&crsfProto, commandItem.rxFrame.data,
                             commandItem.rxFrame.size);

            updateCommunicaiotnState(isCrsfFrameRx);

            /*
             * The sendCrsfFramesToRc function invokes all the time because CRSF
             * communication uses the half-duplex transaction manner. That is
             * why the Task starts to transmit CRSF data to the RC (if needed)
             * immediately after receiving new CRSF data.
             */
            sendCrsfFramesToRc(&updateFrameBitField);

            break;

        case CRSF_TASK_COMMAND_RX_TIMEOUT:
            /*
             * There are no active RX during timeoute. Switch communication to update BR state.
             */
            break;

        case CRSF_TASK_UPDATE_TRANSMITER_STATE:
            updateFrameBitField = 0;
            BIT_SET(updateFrameBitField, LINK_STATISTICS_BIT_POS);
            //BIT_SET(updateFrameBitField, RADIO_ID_BIT_POS);

            /*
             * Enable transfre RC Chann0el frame only for the test purposes
             */
            //BIT_SET(updateFrameBitField, RC_CHANNEL_BIT_POS);
            break;
        }
    }
}

static bool crsfFrameCacheAtomicCb(bool isTake)
{
    bool result = true;

    if (isTake == true) {
        result = systemRtosSemaphoreTake(crsfFrameCacheMut, DATA_MIRROR_MUTEX_TIMEOUTE)
                 == pdTRUE;
    } else {
        systemRtosSemaphoreGive(crsfFrameCacheMut);
    }

    return result;
}

int crsfTaskSetControlState(CrsfTaskInformationState *controlState)
{
    crsfFrameCachePush(&crsfFrameCache, controlState->linkStatistic.address, CRSF_LINK_STATISTICS,
                       &controlState->linkStatistic.payload);

    return CRSF_TASK_OK;
}

int crsfTaskGetControlState(CrsfTaskControlState *controlState)
{
    uint32_t timeStamp;

    crsfFrameCachePop(&crsfFrameCache, &controlState->rcChannelsPacked.address,
                      &timeStamp, CRSF_RC_CHANNELS_PACKED,
                      &controlState->rcChannelsPacked.payload);

    return ((xTaskGetTickCount() - timeStamp) <= RC_CHANNEL_OBSOLETE_TIMEOUTE)
           ? CRSF_TASK_OK
           : CRSF_TASK_CRS_DATA_OBSOLET_WARNING;
}

int crsfTaskInit(void)
{
    UsartSettings usartCrsfSettings = {
        .crsf.br = brList[0],
    };

    UsartCb usartCrsfCb = {
        .crsf = {usartRxCb, usartTxCompleteCb},
    };

    CrsfCb crsfCb = {
        .rxFrame = rxCrsfFrameCb,
        .crc = NULL,
    };

    if (usartInit(USART_TARGET_CRSF, usartCrsfSettings, usartCrsfCb)
        !=  USART_OK) {
        return CRSF_TASK_USART_INIT_ERROR;
    }

    txCompleteSem = xSemaphoreCreateBinary();
    if (txCompleteSem == NULL) {
        return CRSF_TASK_CREATE_TX_COMPL_SEM_ERROR;
    }

    crsfTaskCommandQueue = xQueueCreate(CRSF_TASK_COMMANDS_QUEUE_ITEMS,
                                        sizeof(CrsfTaskCommandItem));
    if (crsfTaskCommandQueue == NULL) {
        return CRSF_TASK_CREATE_EVENT_QUEUE_ERROR;
    }

    crsfFrameCacheMut = xSemaphoreCreateMutex();
    if (crsfFrameCacheMut == NULL) {
        return CRSF_TASK_CREATE_DATAMIRROR_MUT_ERROR;
    }

/*
    sendTransmiterStateTimer = xTimerCreate("Transmiter state timer",
                                            UPDATE_TRANSMITER_STATE_PERIOD,
                                            pdTRUE,
                                            NULL,
                                            updateTransmiterStateTimerCb);
    if (sendTransmiterStateTimer == NULL) {
        return false;
    }
*/
    if (crsfInit(&crsfProto, crsfCb) != CRSF_OK) {
        return CRSF_TASK_INIT_CRSF_ERROR;
    }

    /*
     * This function strongly invloke after create the crsfFrameCacheMut
     */
    crsfFrameCacheInit(&crsfFrameCache, crsfFrameCacheAtomicCb);

    if (xTaskCreate (crsfTask,
                     TASK_CRSF_ASCII_NAME,
                     TASK_CRSF_STACK_SIZE,
                     NULL,
                     TASK_CRSF_PRIORITY,
                     &xTaskCreateH) != pdPASS) {
        return CRSF_TASK_CREATE_TASK_ERROR;
    }

    keyControlInit();

    return CRSF_TASK_OK;
}
