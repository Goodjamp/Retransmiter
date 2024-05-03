#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_usart.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"

#include "services.h"
#include "DebugServices.h"

#include "BSP.h"
#include "Usart.h"

/**
 * @brief This file provide board depend UART driver
 */

/*
 * Сalculation of the number of received data over the DMA uses reverse logic because
 * the value on the NDTR register decreases from the MAX to the 1 after each DMA
 * transaction.
 * As far as we use 4 bytes DMA transaction, the DMA copy 4 bytes even if we need only 1 byte.
 * The next relation between bytes to transaction and NDTR register must be implemented
 *
 * +-------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * | bytes | 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | 10 | 11 | 12 | 13 |
 * +-------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * | NDTR  | 1  | 1  | 1  | 1  | 1  | 2  | 2  | 2  | 2  | 3  | 3  | 3  | 3  | 4  |
 * +-------+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 */
#define USART_CRSF_M2M_TRANSACTION_NUMBER(X)    ((X == USART_CRSF_RX_BUFF_SIZE) \
                                                 ? 1                            \
                                                 : ((USART_CRSF_RX_BUFF_SIZE - X + 3) / 4))
#define USART_CRSF_RX_BUFF_SIZE                 (64 * 8)
#define USART_CRSF_RX_BUFF_SIZE_MASK            (USART_CRSF_RX_BUFF_SIZE - 1)
#define USART_CRSF_RX_TAIL_SIZE                 (64 * 4)
#define USART_TC_BUSY_WAITE                     10000

#ifndef USART_CRSF_RX_BUFF_SIZE
#error Define USART_CRSF_RX_BUFF_SIZE
#endif

#if ((USART_CRSF_RX_BUFF_SIZE - 1) & USART_CRSF_RX_BUFF_SIZE) != 0
#error USART_CRSF_RX_BUFF_SIZE must be pow of 2
#endif

#ifdef USART_ENABLE_LOG
static uint32_t corruptPacketsCount = 0;
#endif
static uint8_t usartCrsfRxBuff[USART_CRSF_RX_BUFF_SIZE + USART_CRSF_RX_TAIL_SIZE] __attribute__((aligned(sizeof(uint32_t))));
static struct {
    UsartCsrfCb cb;
    uint32_t prevRxPos;
    uint32_t currentRxPos;
} crsf = {
    .cb = {NULL, NULL},
    .currentRxPos = 0,
    .prevRxPos = USART_CRSF_RX_BUFF_SIZE,
};

void DMA2_Stream7_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC7(CRSF_USART_DMA)) {
        LL_DMA_DisableStream(CRSF_USART_DMA, CRSF_USART_TX_DMA_STREAM);
        LL_DMA_ClearFlag_TC7(CRSF_USART_DMA);
        if (crsf.cb.txComplete != NULL) {
            crsf.cb.txComplete();
        }
    } else {
        /*
         * Hardware or configuration error
         */
    }
}

/**
 * @brief This interrupt appeared at the and of receive CRSF protocol frame
 *        CRSF use the silent interval as a frame marker
 */
void USART1_IRQHandler(void)
{
    uint32_t taileLenght;

    /*
     * Copy the current receive position very first on the interrupt routine to
     * avoid overwriting the receive position, because we don't stop DMA transactions and USART Rx.
     */
    crsf.currentRxPos = LL_DMA_GetDataLength(CRSF_USART_DMA, CRSF_USART_RX_DMA_STREAM);

    if (LL_USART_IsActiveFlag_IDLE(USART_CRSF) == 1) {
        LL_USART_ClearFlag_IDLE(USART_CRSF);
        /*
         * Enable DMA M2M, to copy the data from the head of the receive buffer to the reserved area on
         * the tail of the receiver buffer. If there no data is on the head, the TC M2M appears immediately.
         */
        taileLenght = crsf.currentRxPos > crsf.prevRxPos
                             ? USART_CRSF_M2M_TRANSACTION_NUMBER(crsf.currentRxPos)
                             : 1;
        if (taileLenght < USART_CRSF_RX_TAIL_SIZE) {
            LL_DMA_SetDataLength(CRSF_USART_M2M_DMA, CRSF_USART_M2M_DMA_STREAM,
                                taileLenght);
        } else {
            /*
             * The size of the taile more than maximum frame size.
             * Frame corrupt. Reset receiving
             */
#ifdef USART_ENABLE_LOG
            corruptPacketsCount++;
#endif
            crsf.prevRxPos = USART_CRSF_RX_BUFF_SIZE;
        }

        LL_DMA_ClearFlag_DME1(CRSF_USART_M2M_DMA);
        LL_DMA_ClearFlag_TE1(CRSF_USART_M2M_DMA);
        LL_DMA_ClearFlag_TC1(CRSF_USART_M2M_DMA);
        LL_DMA_ClearFlag_HT1(CRSF_USART_M2M_DMA);
        LL_DMA_EnableStream(CRSF_USART_M2M_DMA, CRSF_USART_M2M_DMA_STREAM);
    } else {
        /*
         * Hardware or configuration error
         */
    }
}

void DMA2_Stream1_IRQHandler(void)
{
    uint32_t prevPos = crsf.prevRxPos;
    uint32_t currentPos = crsf.currentRxPos;

    crsf.prevRxPos = crsf.currentRxPos;

    if (LL_DMA_IsActiveFlag_TC1(CRSF_USART_M2M_DMA)) {
        LL_DMA_ClearFlag_TC1(CRSF_USART_M2M_DMA);
        LL_DMA_DisableStream(CRSF_USART_M2M_DMA, CRSF_USART_M2M_DMA_STREAM);
        if (crsf.cb.rx != NULL) {
            crsf.cb.rx(&usartCrsfRxBuff[USART_CRSF_RX_BUFF_SIZE - prevPos],
                       (prevPos - currentPos) & USART_CRSF_RX_BUFF_SIZE_MASK);
        }
    } else {
        /*
         * Hardware or configuration error
         */
    }
}

static bool usartIsTxComplete(USART_TypeDef *usart)
{
    volatile uint32_t waitBsyCnt = USART_TC_BUSY_WAITE;

    if (LL_USART_IsEnabled(usart) == 0) {
        return true;
    }
    //uint32_t isActive = LL_USART_IsActiveFlag_TC(usart) ;

    /*
     * Waite to complete transaction
     */
    while (LL_USART_IsActiveFlag_TC(usart) == 0
           && (--waitBsyCnt != 0)) {
    }

    return true;//waitBsyCnt != 0;
}

static UsartResult usartCrsfInit(uint32_t br)
{
    /*
     * To be sure that the previous transmission is complete we need to wait
     * while the USART TC flag is set
     */
    if(usartIsTxComplete(USART_CRSF) == false) {
        return USART_TX_BUSSY_ERROR;
    }

    LL_USART_DeInit(USART_CRSF);
    /*
     * Disable/Enable USART to reset it
     */
    /*
    if (servicesDisablePerephr(USART_CRSF) == false) {
        return USART_PERIPHERAL_ERROR;
    }
    */

    if (servicesEnablePerephr(USART_CRSF) == false) {
        return USART_PERIPHERAL_ERROR;
    }

    LL_DMA_DisableStream(CRSF_USART_DMA, CRSF_USART_TX_DMA_STREAM);
    LL_DMA_DisableStream(CRSF_USART_DMA, CRSF_USART_TX_DMA_STREAM);

    LL_USART_InitTypeDef usartInitStruct = {
        .BaudRate = br,
        .DataWidth = LL_USART_DATAWIDTH_8B,
        .StopBits = LL_USART_STOPBITS_1,
        .Parity = LL_USART_PARITY_NONE,
        .TransferDirection = LL_USART_DIRECTION_TX_RX,
        .HardwareFlowControl = LL_USART_HWCONTROL_NONE,
        .OverSampling = LL_USART_OVERSAMPLING_8,
    };

    if(LL_USART_Init(USART_CRSF, &usartInitStruct) == ERROR) {
        return  USART_INTERNAL_ERROR;
    }
    LL_USART_ConfigAsyncMode(USART_CRSF);
    LL_USART_EnableDMAReq_RX(USART_CRSF);
    LL_USART_EnableDMAReq_TX(USART_CRSF);
    LL_USART_EnableIT_IDLE(USART_CRSF);

    LL_USART_Enable(USART_CRSF);

    return USART_OK;
}

static UsartResult usartCrsfConfig(const UsartCrsfSettings *settings,
                                   const UsartCsrfCb *cb)
{
    struct {
        GPIO_TypeDef *port;
        uint32_t pin;
        uint32_t mode;
        uint32_t alternate;
    } usartGpio[] = {
        {CRSF_USART_GPIO_TX_PORT, CRSF_USART_GPIO_TX_PIN, LL_GPIO_MODE_ALTERNATE, LL_GPIO_AF_7},
        {CRSF_USART_GPIO_RX_PORT, CRSF_USART_GPIO_RX_PIN, LL_GPIO_MODE_ALTERNATE, LL_GPIO_AF_7}
    };
    LL_GPIO_InitTypeDef GPIO_InitStruct = {
        .OutputType = LL_GPIO_OUTPUT_PUSHPULL,
        .Pull = LL_GPIO_PULL_NO,
        .Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
    };

        if (cb->rx == NULL) {
        return USART_RX_CB_NULL_ERROR;
    }
    if (cb->txComplete == NULL) {
        return USART_TX_COMPLETE_CB_NULL_ERROR;
    }

    for (uint32_t k = 0; k < sizeof(usartGpio) / sizeof(usartGpio[0]); k++) {
        GPIO_InitStruct.Pin = usartGpio[k].pin;
        GPIO_InitStruct.Mode = usartGpio[k].mode;
        GPIO_InitStruct.Alternate = usartGpio[k].alternate;
        if (servicesEnablePerephr(usartGpio[k].port) == false) {
            return USART_PERIPHERAL_ERROR;
        }
        LL_GPIO_Init(usartGpio[k].port, &GPIO_InitStruct);
    }

    /*
     * Save CB for future use
     */
    crsf.cb = *cb;

    if (servicesEnablePerephr(CRSF_USART_DMA) == false) {
        return USART_PERIPHERAL_ERROR;
    }

    LL_DMA_InitTypeDef dmaInit = {
        .PeriphOrM2MSrcAddress = (uint32_t)&USART_CRSF->DR,
        .MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT,
        .PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT,
        .PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE,
        .MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE,
        .Priority = LL_DMA_PRIORITY_LOW,
        .FIFOMode = LL_DMA_FIFOMODE_DISABLE,
        .MemBurst = LL_DMA_MBURST_SINGLE,
        .PeriphBurst = LL_DMA_PBURST_SINGLE,
    };

    /* USART DMA RX Init */
    dmaInit.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY,
    dmaInit.Mode = LL_DMA_MODE_CIRCULAR;
    dmaInit.Channel = CRSF_USART_RX_DMA_CHANNEL;
    dmaInit.NbData = USART_CRSF_RX_BUFF_SIZE;
    dmaInit.MemoryOrM2MDstAddress = (uint32_t)usartCrsfRxBuff;
    LL_DMA_Init(CRSF_USART_DMA, CRSF_USART_RX_DMA_STREAM, &dmaInit);
    LL_DMA_EnableStream(CRSF_USART_DMA, CRSF_USART_RX_DMA_STREAM);

    /* USART DMA TX Init */
    dmaInit.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
    dmaInit.Mode = LL_DMA_MODE_NORMAL;
    dmaInit.Channel = CRSF_USART_TX_DMA_CHANNEL;
    dmaInit.NbData = 0;
    dmaInit.MemoryOrM2MDstAddress = (uint32_t)NULL;
    LL_DMA_Init(CRSF_USART_DMA, CRSF_USART_TX_DMA_STREAM, &dmaInit);
    LL_DMA_EnableIT_TC(CRSF_USART_DMA, CRSF_USART_TX_DMA_STREAM);
    LL_DMA_ClearFlag_TC7(CRSF_USART_DMA);

    /*
     * The priority mas be on the range of FreeRTOS API sys call
     */
    NVIC_SetPriority(DMA2_Stream7_IRQn, 5);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    /*
     * Keep the USART Tx Stream disable up to user transmit data
     */

    /*
     * Configure M2m DMA. This transaction is used to transfer the current state of RX data
     * from the USART IDLE interrupt to the DMA TC interrupt. Inside M2M TC inturrep we can
     * free call the FreeRTOS API (inside user CB).
     */
    if (servicesEnablePerephr(CRSF_USART_M2M_DMA) == false) {
        return USART_PERIPHERAL_ERROR;
    }
    dmaInit.MemoryOrM2MDstAddress = (uint32_t) &usartCrsfRxBuff[USART_CRSF_RX_BUFF_SIZE];
    dmaInit.PeriphOrM2MSrcAddress = (uint32_t) &usartCrsfRxBuff[0],
    dmaInit.Direction = LL_DMA_DIRECTION_MEMORY_TO_MEMORY,
    dmaInit.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT,
    dmaInit.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_INCREMENT,
    dmaInit.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_WORD,
    dmaInit.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_WORD,
    dmaInit.Mode = LL_DMA_MODE_NORMAL;
    dmaInit.Channel = CRSF_USART_M2M_DMA_CHANNEL;
    dmaInit.NbData = 0;
    dmaInit.Priority = LL_DMA_PRIORITY_HIGH;
    LL_DMA_Init(CRSF_USART_M2M_DMA, CRSF_USART_M2M_DMA_STREAM, &dmaInit);
    LL_DMA_EnableIT_TC(CRSF_USART_M2M_DMA, CRSF_USART_M2M_DMA_STREAM);

    /*
     * The priority must be on the range of FreeRTOS API system call
     */
    NVIC_SetPriority(DMA2_Stream1_IRQn, 5);
    NVIC_EnableIRQ(DMA2_Stream1_IRQn);

    /*
     * USART init
     */
    usartCrsfInit(settings->br);

    /*
     * USART1 routin only IDLE interrupt. This interrup must be processing as fast as it posibble
     * to avoide mistake on the process of calculation number of receive data,
     * becouse we never stop DMA and USART RX
     */
    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);

    return USART_OK;
}

static UsartResult usartCsrfTx(uint8_t *buff, uint32_t size)
{
    if (buff == NULL) {
        return USART_BUFF_NULL_ERROR;
    }
    if (size == 0) {
        return USART_DATA_SIZE_0_ERROR;
    }

    /*
     * To be sure that the previous transmission is complete we need to wait
     * while the USART TC flag is set
     */
    if(usartIsTxComplete(USART_CRSF) == false) {
        return USART_TX_BUSSY_ERROR;
    }

    LL_DMA_SetDataLength(CRSF_USART_DMA, CRSF_USART_TX_DMA_STREAM, size);
    LL_DMA_SetMemoryAddress(CRSF_USART_DMA, CRSF_USART_TX_DMA_STREAM,
                            (uint32_t)buff);
    /*
     * According to the instruction we also need to clear TC flag. Do it.
     */
    LL_USART_ClearFlag_TC(USART_CRSF);

    LL_DMA_ClearFlag_TE7(CRSF_USART_DMA);
    LL_DMA_ClearFlag_HT7(CRSF_USART_DMA);
    LL_DMA_ClearFlag_DME7(CRSF_USART_DMA);
    LL_DMA_ClearFlag_FE7(CRSF_USART_DMA);
    LL_DMA_ClearFlag_TC7(CRSF_USART_DMA);

    LL_DMA_EnableStream(CRSF_USART_DMA, CRSF_USART_TX_DMA_STREAM);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    return USART_OK;
}

UsartResult usartInit(UsartTarget usartTarget, UsartSettings settings, UsartCb cb)
{
    UsartResult result;

    switch (usartTarget) {
    case USART_TARGET_CRSF:
        result = usartCrsfConfig(&settings.crsf, &cb.crsf);
        break;

    default:
        result = USART_TARGET_ERROR;
    }

    return result;
}

UsartResult usartTx(UsartTarget usartTarget, uint8_t *buff, uint32_t size)
{
    UsartResult result;

    switch (usartTarget) {
    case USART_TARGET_CRSF:
        result = usartCsrfTx(buff, size);
        break;

    default:
        result = USART_TARGET_ERROR;
        break;
    }

    return result;
}

UsartResult usartSetBr(UsartTarget usartTarget, uint32_t br)
{
    UsartResult result;

    switch (usartTarget) {
    case USART_TARGET_CRSF:
        result = usartCrsfInit(br);
        break;

    default:
        result = USART_TARGET_ERROR;
        break;
    }

    return result;
}
