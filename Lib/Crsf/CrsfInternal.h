#ifndef __CRC_INTERNAL_H__
#define __CRC_INTERNAL_H__

#include <stdint.h>

#include "crsf.h"

#define STR_FIELD_POS(S, F)                  ((uint32_t)&(((S*)0)->F))
#define STR_FIELD_SIZE(S, F)                 sizeof(((S*)0)->F)
#define FIELD_FRAME_SIZE(FULL_FRAME_SIZE)    (FULL_FRAME_SIZE - 2)
#define CRC_SIZE(FULL_FRAME_SIZE)            (FULL_FRAME_SIZE - 3)

#define CRSF_MAX_BUFF_SIZE       64
#define CRSF_MIN_PAYLOAD_SIZE    (sizeof(CrsfHeader) + 1)
#define CRSF_MAX_PAYLOAD_SIZE    (CRSF_MAX_BUFF_SIZE - 2)
#define CRSF_PAYLOAD_POS         2
#define CRSF_CRC_SIZE            sizeof(uint8_t)

typedef enum {
    CRSF_ADDRESS_CRSF_RECEIVER = 0xEC,
    CRSF_ADDRESS_RADIO_TRANSMITTER = 0xEA,
    CRSF_ADDRESS_CRSF_TRANSMITTER = 0xEE,
    CRSF_ADDRESS_CRSF_FLY_CONTROLER = 0xC8,
} CrsfAddressDesr;

typedef enum {
    CRSF_FRAME_LINK_STATISTICS = 0x14,
    CRSF_FRAME_RC_CHANNELS_PACKED = 0x16,
} CrsfFrameType_;

#pragma pack(push, 1)

typedef struct {
    uint8_t address; // The source address
    uint8_t frameSize;  // the data size after this byte
    uint8_t frameType;
    uint8_t payload[0];
} CrsfHeader;

/*
 * The headr extantion for the frames type on the range [0x28; 0x96]
 */
typedef struct {
    uint8_t  destAddr;
    uint8_t  origAddr;
} CrsfHeaderExtantion;

typedef struct {
    unsigned ch0  : 11;
    unsigned ch1  : 11;
    unsigned ch2  : 11;
    unsigned ch3  : 11;
    unsigned ch4  : 11;
    unsigned ch5  : 11;
    unsigned ch6  : 11;
    unsigned ch7  : 11;
    unsigned ch8  : 11;
    unsigned ch9  : 11;
    unsigned ch10 : 11;
    unsigned ch11 : 11;
    unsigned ch12 : 11;
    unsigned ch13 : 11;
    unsigned ch14 : 11;
    unsigned ch15 : 11;
} CrsfRcChannelsPacketRaw;

typedef struct {
    uint8_t uplinkRssi1;
    uint8_t uplinkRssi2;
    uint8_t uplinkLinkQuality;
    int8_t uplinkSnr;
    uint8_t activeAntenna;
    uint8_t rfMode;
    uint8_t uplinkTxPower;
    uint8_t downlinkRssi;
    uint8_t downlinkLinkQuality;
    int8_t downlinkSnr;
} CrsfLinkStatisticsRaw;

#pragma pack(pop)

uint8_t calcCrc(const uint8_t *data, uint32_t size);

#endif
