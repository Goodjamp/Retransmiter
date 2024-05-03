#ifndef __SPI_H__
#define __SPI_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief This file provide board depend SPI driver
 */

typedef enum {
    SPI_RES_OK,
    SPI_RES_SPI_TARGET_ERROR,
    SPI_RES_HW_ERROR,
    SPI_RES_BUFF_NULL_ERROR,
    SPI_RES_SIZE_0_ERROR,
    SPI_RES_INTERNAL_ERROR,
} SpiResult;

typedef enum {
    SPI_ETH,
    SPI_CNT,
} SpiTarget;

typedef struct {
    void (*rxComplete)(SpiResult result);
	void (*txComplete)(SpiResult result);
} SpiEthCb;

typedef struct {
    uint32_t speed; // currently ignor. default SCK frq = 3 MHz
} SpiEthSettings;

typedef union {
    SpiEthCb eth;
} SpiCb;

typedef union {
    SpiEthSettings eth;
} SpiSettings;

SpiResult spiInit(SpiTarget target, SpiSettings settings, SpiCb cb);
SpiResult spiTx(SpiTarget target, uint8_t buff[], uint32_t size);
SpiResult spiRx(SpiTarget target, uint8_t buff[], uint32_t size);
SpiResult spiCsControl(SpiTarget target, bool set);

#endif
