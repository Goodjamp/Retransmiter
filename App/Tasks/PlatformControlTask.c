#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

#include "DebugServices.h"
#include "I2c.h"
#include "ExtEv.h"
#include "Lsm303dlhc.h"
#include "PlatformControlTask.h"

#include "TasksSettings.h"
#include "stm32f4xx_ll_tim.h"
#include "stm32f4xx_ll_gpio.h"
#include "Services.h"
#include "BSP.h"

#define HORIZONTAL                   90
#define VERTICAL                     45
#define MIN_SER                      400
#define MAX_SER                      2300
#define NULL_SER            	     (MIN_SER + (MAX_SER - MIN_SER) / 2)
#define TO_DEGR_SERVO                ((MAX_SER - MIN_SER) / 180.0f)

#define LED_BLINK_PERIOD_CALIB_0     50
#define LED_BLINK_PERIOD_CALIB_90    200
#define LED_BLINK_PERIOD_WORK        500

void i2cTransactionComplete(bool txSuccess);
void extEvDrdy(void);
volatile bool txCInProcess = false;
Lsm303dlhcHandler lsm3030glhcHandler;
volatile bool isInit = false;
static TimerHandle_t ledTimer;
static volatile bool mesInProcess = false;

static const uint32_t blingPeriodList[] = {
    pdMS_TO_TICKS(LED_BLINK_PERIOD_CALIB_0),
    pdMS_TO_TICKS(LED_BLINK_PERIOD_CALIB_90),
    pdMS_TO_TICKS(LED_BLINK_PERIOD_WORK),
};

I2cSettings i2cLsm303dlhcSettings = {
    .lsm303dlhc.cb = {i2cTransactionComplete},
};
ExtEvSettings extEvLsm303dlhcSettings = {
    .lsm303dlhc.extEvCb = extEvDrdy
};

PlatformState platformState = PLATFORM_STATE_WORK;

#define MOVE_WINDOW_SIZE    8
typedef struct {
    int16_t buff[MOVE_WINDOW_SIZE];
    uint8_t pos;
    int16_t everageSumm;
} MovWindowH;

volatile struct {
    int16_t xCalib0;
    int16_t xCalib90;
    MovWindowH xWindow;
    MovWindowH yWindow;
    MovWindowH zWindow;
    bool isReady;
    uint8_t measCnt;
} measureState = {
    .isReady = false,
    .measCnt = 0,
};

static void setServoDegres (float DegresM1, float DegresM2)
{
    uint16_t servoM1;
    uint16_t servoM2;

    if(DegresM1 < -HORIZONTAL) {
        DegresM1 = -HORIZONTAL;
    } else if(DegresM1 > HORIZONTAL) {
        DegresM1 = HORIZONTAL;
    }
    servoM1 = NULL_SER - DegresM1 *TO_DEGR_SERVO ;

    if(DegresM2 < -VERTICAL) {
        DegresM2 = -VERTICAL;
    } else if(DegresM2 > VERTICAL) {
        DegresM2 = VERTICAL;
    }
    servoM2 = NULL_SER - DegresM2 *TO_DEGR_SERVO;

    if(servoM1 != SERVO_CONTROL_TIMER->CCR1) {
        if(servoM1 < MIN_SER) {
            servoM1 = MIN_SER;
        } else if(servoM1 > MAX_SER) {
            servoM1 = MAX_SER;
        }
        SERVO_CONTROL_TIMER->CCR1 = servoM1;
    }

    if(servoM2 != SERVO_CONTROL_TIMER->CCR2) {

        if(servoM2 < MIN_SER) {
            servoM2 = MIN_SER;
        } else if(servoM2 > MAX_SER) {
            servoM2 = MAX_SER;
        }
        SERVO_CONTROL_TIMER->CCR2 = servoM2;
    }
}

static int16_t moveWindowUpdate(volatile MovWindowH *item, int16_t value)
{
    item->everageSumm += value;
    item->everageSumm -= item->buff[item->pos];
    item->buff[item->pos] = value;
    item->pos++;
    item->pos = (item->pos & (MOVE_WINDOW_SIZE - 1));


    return item->everageSumm / MOVE_WINDOW_SIZE;
}

static inline int16_t lineInterpol(float x0, float y0, float x1, float y1, float val) {
    float k = (y1 - y0) / (x1- x0);
    return (int16_t) (k * (val - x0) + y0);
}

static inline void plarformControlUpdate(int16_t mX, int16_t mY, int16_t mZ)
{
    static volatile int16_t angle;
    int16_t averageX;

    switch (platformState) {
    case PLATFORM_STATE_CALIBRATE_0:
        measureState.xCalib0 = moveWindowUpdate(&measureState.xWindow, mX);
        break;

    case PLATFORM_STATE_CALIBRATE_90:
        measureState.xCalib90 = moveWindowUpdate(&measureState.xWindow, mX);
        break;

    case PLATFORM_STATE_WORK:
        averageX = moveWindowUpdate(&measureState.xWindow, mX);
        angle = lineInterpol(measureState.xCalib0, 0, measureState.xCalib90, 90, averageX);
        setServoDegres(0, angle);
        break;

    }
    mesInProcess = true;
}

static void initServoControl(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};
    LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /*
     * Init timer: PWM mode.
     * CH1 - control servo horizontal
     * CH2 - control servo vertical
     */
    servicesEnablePerephr(SERVO_CONTROL_TIMER);
    TIM_InitStruct.Prescaler = 99;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 10000;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(SERVO_CONTROL_TIMER, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(SERVO_CONTROL_TIMER);
    LL_TIM_SetClockSource(SERVO_CONTROL_TIMER, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_OC_EnablePreload(SERVO_CONTROL_TIMER, LL_TIM_CHANNEL_CH1);
    TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
    TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
    TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.CompareValue = 0;

    TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
    LL_TIM_OC_Init(SERVO_CONTROL_TIMER, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
    LL_TIM_OC_DisableFast(SERVO_CONTROL_TIMER, LL_TIM_CHANNEL_CH1);
    LL_TIM_OC_EnablePreload(SERVO_CONTROL_TIMER, LL_TIM_CHANNEL_CH2);

    LL_TIM_OC_Init(SERVO_CONTROL_TIMER, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
    LL_TIM_OC_DisableFast(SERVO_CONTROL_TIMER, LL_TIM_CHANNEL_CH2);
    LL_TIM_SetTriggerOutput(SERVO_CONTROL_TIMER, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(SERVO_CONTROL_TIMER);

    /*
     * Init GPIO
     */
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_2;

    servicesEnablePerephr(SERVO_H_GPIO_PORT);
    GPIO_InitStruct.Pin = SERVO_H_GPIO_PIN;
    LL_GPIO_Init(SERVO_H_GPIO_PORT, &GPIO_InitStruct);

    servicesEnablePerephr(SERVO_V_GPIO_PORT);
    GPIO_InitStruct.Pin = SERVO_V_GPIO_PIN;
    LL_GPIO_Init(SERVO_V_GPIO_PORT, &GPIO_InitStruct);

    LL_TIM_OC_SetCompareCH1(SERVO_CONTROL_TIMER, 800);
    LL_TIM_OC_SetCompareCH2(SERVO_CONTROL_TIMER, 800);
    LL_TIM_EnableCounter(SERVO_CONTROL_TIMER);
}

void i2cTransactionComplete(bool txSuccess)
{
    if (txSuccess == false) {
        return;
    }
    txCInProcess = false;
    lsm303dlhcI2cComplete(lsm3030glhcHandler);
}

void extEvDrdy(void)
{
    if (isInit == false) {
        return;
    }
    lsm303dlhcDrdy(lsm3030glhcHandler);
}

static bool lsm3030dlhcI2cTx(uint8_t devAdd, uint8_t *data, uint8_t dataNumber)
{
    I2cResult result;

    result = i2cTx(I2C_TARGET_LSM303DLHC, devAdd, data, dataNumber);
    return result == I2C_OK;
}

static bool lsm3030dlhcI2cRx(uint8_t devAdd, uint8_t *txData, uint8_t *data, uint8_t dataNumber)
{
    I2cResult result;

    result = i2cRx(I2C_TARGET_LSM303DLHC, devAdd, txData, 1, data, dataNumber);
    return result == I2C_OK;
}

static void lsm303dlhcMMesCompleteCb(Lsm303dlhcMagnetic rawMagnetic, uint16_t angle)
{
    static volatile int16_t mX;
    static volatile int16_t mY;
    static volatile int16_t mZ;

    mX = rawMagnetic.x;
    mY = rawMagnetic.y;
    mZ = rawMagnetic.z;

    plarformControlUpdate( mX, mY, mZ);
}

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
                            pdMS_TO_TICKS(LED_BLINK_PERIOD_CALIB_0),
                            pdTRUE, NULL, ledTimerCb);
    if (ledTimer == NULL) {
        return false;
    }
    xTimerStart(ledTimer, 10);

    return true;
}

void platformControlUpdateState(void)
{
    switch (platformState) {
    case PLATFORM_STATE_CALIBRATE_0:
        platformState = PLATFORM_STATE_CALIBRATE_90;
        break;

    case PLATFORM_STATE_CALIBRATE_90:
        platformState = PLATFORM_STATE_WORK;
        break;

    case PLATFORM_STATE_WORK:
        platformState = PLATFORM_STATE_CALIBRATE_0;
        break;
    }

    xTimerChangePeriod(ledTimer, blingPeriodList[platformState], 10);
}

static bool platformControlRunSensor(void)
{
    Lsm303dlhcStatus sensorResult;

    sensorResult = lsm303dlhcMSetRate(lsm3030glhcHandler, LSM303DLHC_M_RATE_30);
    if (sensorResult != LSM303DLHC_STATUS_OK) {
        return false;
    }
    sensorResult = lsm303dlhcMSetGain(lsm3030glhcHandler, LSM303DLHC_M_GAIN_0);
    if (sensorResult != LSM303DLHC_STATUS_OK) {
        return false;
    }

    sensorResult = lsm303dlhcMesMStart(lsm3030glhcHandler, lsm303dlhcMMesCompleteCb);
    if (sensorResult != LSM303DLHC_STATUS_OK) {
        return false;
    }

    return true;
}

static void platformControlTask(void *userData)
{
    while(true) {
        vTaskDelay(500);
        if (mesInProcess == false) {
            platformControlRunSensor();
        }
        mesInProcess = false;
    }
}

bool platformControlTaskInit(void)
{
    I2cResult i2cInitResult;
    ExtEvResult extInitEvResult;

    i2cInitResult = i2cInit(I2C_TARGET_LSM303DLHC, i2cLsm303dlhcSettings);
    if (i2cInitResult != I2C_OK){
        return false;
    }

    extInitEvResult = extEvInit(EXT_EV_TARGET_LSM303DLHC, extEvLsm303dlhcSettings);
    if (extInitEvResult != EXT_EV_OK){
        return false;
    }

    volatile uint32_t cnt = 40000;
    while(cnt--){}

    lsm3030glhcHandler = lsm303dlhcMInit(lsm3030dlhcI2cTx, lsm3030dlhcI2cRx);
    if (lsm3030glhcHandler == NULL) {
        return false;
    }
    platformControlRunSensor();

    initServoControl();
    initLedIndication();


    if (xTaskCreate(platformControlTask,
                    TASK_PLATFORMCONTROL_ASCII_NAME,
                    TASK_PLATFORMCONTROL_STACK_SIZE,
                    NULL,
                    TASK_PLATFORMCONTROL_PRIORITY,
                    NULL) != pdPASS) {
        return false;
    }

    isInit = true;

    return true;
}
