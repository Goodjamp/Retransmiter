#ifndef __CRSF_TASK_TASK_H__
#define  __CRSF_TASK_TASK_H__

#include <stdint.h>
#include <stdbool.h>

#include "Crsf.h"

#define CRSF_TASK_TASK_LOG_ENABLE

#define CRSF_TASK_CRS_DATA_OBSOLET_WARNING       1
#define CRSF_TASK_OK                             0
#define CRSF_TASK_USART_INIT_ERROR               -1
#define CRSF_TASK_CREATE_TX_COMPL_SEM_ERROR      -2
#define CRSF_TASK_CREATE_EVENT_QUEUE_ERROR       -4
#define CRSF_TASK_CREATE_TASK_ERROR              -5
#define CRSF_TASK_INIT_CRSF_ERROR                -6
#define CRSF_TASK_CREATE_DATAMIRROR_MUT_ERROR    -7
#define CRSF_TASK_GET_DATAMIRROR_MUT_ERROR       -8

typedef struct {
    struct {
        uint8_t address;
        CrsfRcChannelsPacked payload;
    } rcChannelsPacked;
} CrsfTaskControlState;

typedef struct {
    struct {
        uint8_t address;
        CrsfLinkStatistic payload;
    } linkStatistic;
} CrsfTaskInformationState;

int crsfTaskSetControlState(CrsfTaskInformationState *controlState);
int crsfTaskGetControlState( CrsfTaskControlState *controlState);
int crsfTaskInit(void);

#endif
