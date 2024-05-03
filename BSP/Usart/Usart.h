#ifndef __USART_CRSF_H__
#define __USART_CRSF_H__

#include <stdint.h>

//#define USART_ENABLE_LOG

#define USART_CRSF_MAX_DATA_SIZE    CSFR_MAX_PACKET_SIZE

typedef enum {
    USART_OK,
    USART_TARGET_ERROR,
    USART_RX_CB_NULL_ERROR,
    USART_TX_COMPLETE_CB_NULL_ERROR,
    USART_BR_ERROR,
    USART_INTERNAL_ERROR,
    USART_PERIPHERAL_ERROR,
    USART_BUFF_NULL_ERROR,
    USART_DATA_SIZE_0_ERROR,
    USART_TX_BUSSY_ERROR,
} UsartResult;

typedef enum {
    USART_TARGET_CRSF,
} UsartTarget;

typedef struct {
    void (*rx)(uint8_t *buff, uint32_t size);
    void (*txComplete)(void);
} UsartCsrfCb;

typedef struct {
    uint32_t br;
} UsartCrsfSettings;

typedef union {
    UsartCsrfCb crsf;
} UsartCb;

typedef union {
    UsartCrsfSettings crsf;
} UsartSettings;

UsartResult usartInit(UsartTarget usartTarget, UsartSettings settings, UsartCb cb);
UsartResult usartTx(UsartTarget usartTarget, uint8_t *buff, uint32_t size);
UsartResult usartRx(UsartTarget usartTarget, uint8_t *buff, uint32_t size);
UsartResult usartSetBr(UsartTarget usartTarget, uint32_t br);

#endif
