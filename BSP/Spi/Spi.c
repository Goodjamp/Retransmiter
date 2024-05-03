#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_dma.h"
#include "services.h"

#include "BSP.h"

#include "DebugServices.h"

#define ETH_SPI_BSY_WAITE      400000

static SpiEthCb w5500Cb;
static uint8_t spiW5500FakeTxByte = 5;

static void spiW5500ClearDmaStatus(void)
{
    LL_DMA_ClearFlag_TC3(ETH_SPI_DMA);
    LL_DMA_ClearFlag_HT3(ETH_SPI_DMA);
    LL_DMA_ClearFlag_TE3(ETH_SPI_DMA);
    LL_DMA_ClearFlag_DME3(ETH_SPI_DMA);
    LL_DMA_ClearFlag_FE3(ETH_SPI_DMA);

    LL_DMA_ClearFlag_TC0(ETH_SPI_DMA);
    LL_DMA_ClearFlag_HT0(ETH_SPI_DMA);
    LL_DMA_ClearFlag_TE0(ETH_SPI_DMA);
    LL_DMA_ClearFlag_DME0(ETH_SPI_DMA);
    LL_DMA_ClearFlag_FE0(ETH_SPI_DMA);
}

static void spiW550DisableDmaStreams(void)
{
    LL_DMA_DisableStream(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM);
    LL_DMA_DisableStream(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM);
}

static void spiW550EnableDmaStreams(void)
{
    LL_DMA_EnableStream(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM);
    LL_DMA_EnableStream(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM);
}

static void spiW550EnableSpiDmaReq(void)
{
    LL_SPI_EnableDMAReq_RX(ETH_SPI_SPI);
    LL_SPI_EnableDMAReq_TX(ETH_SPI_SPI);
}

static void spiW550DisableSpiDmaReq(void)
{
    LL_SPI_DisableDMAReq_RX(ETH_SPI_SPI);
    LL_SPI_DisableDMAReq_TX(ETH_SPI_SPI);
}

/*
 * ETH SPI TX complete
 */
void DMA2_Stream3_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TE3(ETH_SPI_DMA) == 1
        || LL_DMA_IsActiveFlag_DME3(ETH_SPI_DMA) == 1
        || LL_DMA_IsActiveFlag_FE3(ETH_SPI_DMA) == 1) {
        w5500Cb.txComplete(SPI_RES_HW_ERROR);
    }
    if (LL_DMA_IsActiveFlag_TC3(ETH_SPI_DMA)) {
        if (w5500Cb.txComplete != NULL) {
            w5500Cb.txComplete(SPI_RES_OK);
        }
    }
    spiW550DisableSpiDmaReq();
    spiW5500ClearDmaStatus();
}

/*
 * ETH SPI Rx complete
 */
void DMA2_Stream0_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TE0(ETH_SPI_DMA) == 1
        || LL_DMA_IsActiveFlag_DME0(ETH_SPI_DMA) == 1
        || LL_DMA_IsActiveFlag_FE0(ETH_SPI_DMA) == 1) {
        w5500Cb.rxComplete(SPI_RES_HW_ERROR);
    }
    if (LL_DMA_IsActiveFlag_TC0(ETH_SPI_DMA)) {
        if (w5500Cb.rxComplete != NULL) {
            w5500Cb.rxComplete(SPI_RES_OK);
        }
    }
    spiW550DisableSpiDmaReq();
    spiW5500ClearDmaStatus();
}

static SpiResult spiInitW5500(SpiEthSettings *settings, SpiEthCb *cb)
{
    struct {
        GPIO_TypeDef *port;
        uint32_t pin;
        uint32_t mode;
        uint32_t alternate;
    } spiGpio[] = {
        {ETH_SPI_GPIO_CS_PORT, ETH_SPI_GPIO_CS_PIN, LL_GPIO_MODE_OUTPUT, LL_GPIO_AF_5},
        {ETH_SPI_GPIO_SCK_PORT, ETH_SPI_GPIO_SCK_PIN, LL_GPIO_MODE_ALTERNATE, LL_GPIO_AF_5},
        {ETH_SPI_GPIO_MOSI_PORT, ETH_SPI_GPIO_MOSI_PIN, LL_GPIO_MODE_ALTERNATE, LL_GPIO_AF_5},
        {ETH_SPI_GPIO_MISO_PORT, ETH_SPI_GPIO_MISO_PIN, LL_GPIO_MODE_ALTERNATE, LL_GPIO_AF_5},
    };
    LL_GPIO_InitTypeDef GPIO_InitStruct = {
        .OutputType = LL_GPIO_OUTPUT_PUSHPULL,
        .Pull = LL_GPIO_PULL_NO,
        .Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH,
    };

    for (uint32_t k = 0; k < sizeof(spiGpio) / sizeof(spiGpio[0]); k++) {
        GPIO_InitStruct.Pin = spiGpio[k].pin;
        GPIO_InitStruct.Mode = spiGpio[k].mode;
        GPIO_InitStruct.Alternate = spiGpio[k].alternate;
        servicesEnablePerephr(spiGpio[k].port);
        LL_GPIO_Init(spiGpio[k].port, &GPIO_InitStruct);
    }

    w5500Cb = *cb;

    /*
     * The W5500 deselect by the default
     */
    LL_GPIO_SetOutputPin(ETH_SPI_GPIO_CS_PORT, ETH_SPI_GPIO_CS_PIN);

    /* SPI DMA RX init */

    servicesEnablePerephr(ETH_SPI_DMA);

    LL_DMA_InitTypeDef dmaInit = {
        .NbData = 0,
        .MemoryOrM2MDstAddress = 0,
        .PeriphOrM2MSrcAddress = (uint32_t)&ETH_SPI_SPI->DR,
        .Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY,
        .Mode = LL_DMA_MODE_NORMAL,
        .MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT,
        .PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT,
        .PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE,
        .MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE,
        .Channel = ETH_SPI_DMA_RX_CH,
        .Priority = LL_DMA_PRIORITY_LOW,
        .FIFOMode = LL_DMA_FIFOMODE_DISABLE,
        .MemBurst = LL_DMA_MBURST_SINGLE,
        .PeriphBurst = LL_DMA_PBURST_SINGLE,
    };

    LL_DMA_Init(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM, &dmaInit);
    LL_DMA_DisableFifoMode(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM);
    LL_DMA_EnableIT_TC(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM);
    NVIC_SetPriority(DMA2_Stream0_IRQn, 6);
    NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    /* SPI DMA TX init */

    dmaInit.Channel = ETH_SPI_DMA_TX_CH;
    dmaInit.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    LL_DMA_Init(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM, &dmaInit);
    LL_DMA_DisableFifoMode(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM);
    LL_DMA_EnableIT_TC(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM);
    NVIC_SetPriority(DMA2_Stream3_IRQn, 6);
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);

    /* SPI init*/

    servicesEnablePerephr(ETH_SPI_SPI);

    LL_SPI_InitTypeDef spiInit = {
        .TransferDirection = LL_SPI_FULL_DUPLEX,
        .Mode = LL_SPI_MODE_MASTER,
        .DataWidth = LL_SPI_DATAWIDTH_8BIT,
        .ClockPolarity = LL_SPI_POLARITY_LOW,
        .ClockPhase = LL_SPI_PHASE_1EDGE,
        .BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV32,
        .BitOrder = LL_SPI_MSB_FIRST,
        .CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE,
        .CRCPoly = 10,
    };
    spiInit.NSS = LL_SPI_NSS_SOFT;
    LL_SPI_Init(ETH_SPI_SPI, &spiInit);
    LL_SPI_SetStandard(ETH_SPI_SPI, LL_SPI_PROTOCOL_MOTOROLA);
    LL_SPI_DisableDMAReq_RX(ETH_SPI_SPI);
    LL_SPI_DisableDMAReq_TX(ETH_SPI_SPI);

    LL_SPI_Enable(ETH_SPI_SPI);

    return SPI_RES_OK;
}

SpiResult spiInit(SpiTarget target, SpiSettings settings, SpiCb cb)
{
    SpiResult result = SPI_RES_OK;

    switch (target) {
    case SPI_ETH:
        spiInitW5500(&settings.eth, &cb.eth);
        break;

    default:
        result = SPI_RES_SPI_TARGET_ERROR;
        break;
    }

   return result;
}

static bool spiIsTransactionComplete(SpiTarget target)
{
    uint32_t waitBsyCnt = ETH_SPI_BSY_WAITE;

    /*
     * Waite to complete transaction
     */
    while (LL_SPI_IsActiveFlag_BSY(ETH_SPI_SPI) == 1
           && (--waitBsyCnt != 0)) {
    }

    return waitBsyCnt != 0;
}

SpiResult spiTx(SpiTarget target, uint8_t buff[], uint32_t size)
{
    if (spiIsTransactionComplete(target) == false) {
        return SPI_RES_HW_ERROR;
    }
    if (buff == NULL){
        return SPI_RES_BUFF_NULL_ERROR;
    }
    if (size == 0){
        return SPI_RES_SIZE_0_ERROR;
    }
    if (target > SPI_CNT) {
        return SPI_RES_SPI_TARGET_ERROR;
    }

    spiW550DisableDmaStreams();
    spiW5500ClearDmaStatus();
    LL_DMA_SetMemoryIncMode(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetMemoryAddress(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM, (uint32_t)buff);
    LL_DMA_SetDataLength(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM, size);
    LL_DMA_EnableIT_TC(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM);
    LL_DMA_DisableIT_TC(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM);
    LL_DMA_EnableStream(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM);

    /*
     * To run the DMA transaction, we must guarantee that the SPI->SR.TXE
     * bit set, the DR must be read.
     */
    LL_SPI_ReceiveData8(ETH_SPI_SPI);

    spiW550EnableSpiDmaReq();

    return SPI_RES_OK;
}

SpiResult spiRx(SpiTarget target, uint8_t buff[], uint32_t size)
{
    if (spiIsTransactionComplete(target) == false) {
        return SPI_RES_HW_ERROR;
    }

    if (buff == NULL){
        return SPI_RES_BUFF_NULL_ERROR;
    }
    if (size == 0){
        return SPI_RES_SIZE_0_ERROR;
    }
    if (target > SPI_CNT) {
        return SPI_RES_SPI_TARGET_ERROR;
    }

    spiW550DisableDmaStreams();
    spiW5500ClearDmaStatus();

    LL_DMA_SetMemoryAddress(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM, (uint32_t)buff);
    LL_DMA_SetDataLength(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM, size);

    /*
     * Configure Tx to transmite fake byte only for generate SCK signal
     */
    LL_DMA_SetMemoryIncMode(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM, LL_DMA_MEMORY_NOINCREMENT);
    LL_DMA_SetMemoryAddress(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM, (uint32_t)&spiW5500FakeTxByte);
    LL_DMA_SetDataLength(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM, size);

    LL_DMA_DisableIT_TC(ETH_SPI_DMA, ETH_SPI_DMA_TX_STREAM);
    LL_DMA_EnableIT_TC(ETH_SPI_DMA, ETH_SPI_DMA_RX_STREAM);

    /*
     * To run the DMA transaction, we must guarantee that the SPI->SR.TXE
     * bit set, the DR must be read.
     */
    LL_SPI_ReceiveData8(ETH_SPI_SPI);

    spiW550EnableDmaStreams();
    spiW550EnableSpiDmaReq();

    return SPI_RES_OK;
}

SpiResult spiCsControl(SpiTarget target, bool set)
{
    if (spiIsTransactionComplete(target) == false) {
        return SPI_RES_HW_ERROR;
    }

    if (set == true) {
        LL_GPIO_SetOutputPin(ETH_SPI_GPIO_CS_PORT, ETH_SPI_GPIO_CS_PIN);
    } else {
        LL_GPIO_ResetOutputPin(ETH_SPI_GPIO_CS_PORT, ETH_SPI_GPIO_CS_PIN);
    }

    return SPI_RES_OK;
}
