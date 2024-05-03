#ifndef __CRSF_H__
#define __CRSF_H__

#include <stdint.h>
#include "crsf_protocol.h"

#define CRSF_RC_CHANNELS_NUMBER       16
#define CRSF_RC_CHANNELS_DIGITS       11
#define CRSF_RC_CHANNELS_BUFF_SIZE    (CRSF_RC_CHANNELS_NUMBER * CRSF_RC_CHANNELS_DIGITS / 8)

typedef enum {
    CRSF_OK = 0,
    CRSF_HANDLER_NULL_ERROR = 1,
    CRSF_CRC_ERROR = 2,
    CRSF_BUFF_NULL_ERROR = 3,
    CRSF_BUFF_MIN_SIZE_ERROR = 4,
    CRSF_ADDRESS_ERROR = 5,
    CRSF_PAYLOAD_NULL_ERROR = 6,
} CrsfResult;

typedef enum {
    CRSF_RC_CHANNELS_PACKED,
    CRSF_LINK_STATISTICS,
} CrsfFrameType;

typedef enum {
    CrsfAddressCrsfTransmiter,
    CrsfAddressRadioTransmiter,
    CrsfAddressCrcsReceiver,
    CrsfAddressFlyControler,
} CrsfAddress;

typedef struct {
    uint16_t chValue[CRSF_RC_CHANNELS_NUMBER];
} CrsfRcChannelsPacked;

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
} CrsfLinkStatistic;

typedef struct {
    CrsfFrameType type;
    union {
        CrsfRcChannelsPacked rcChannelsPacked;
        CrsfLinkStatistic linkStatistic;
    } payload;
} CrsfFrame;

typedef struct {
    uint8_t (*crc)(const uint8_t *data, uint32_t size); // polynom: 0xD5; Initial value 0x00; Final XOR value: 0x00
    void (*rxFrame)(CrsfAddress address, CrsfFrame *frame);
} CrsfCb;

typedef struct {
    CrsfCb cb;
} CrsfH;


/**
 * @brief Initilisate the example of CRSF protocol.
 *
 * @param crsfHandler the item of CRSF project
 * @param cb the list of CB function
 * @return CrsfResult
 */
CrsfResult crsfInit(CrsfH *crsfHandler, CrsfCb cb);

/**
 * @brief Deserialiase input buffer to the CRSF frames. Deserialisation results
 *        return over the relation CB function, see CrsfCb
 */
CrsfResult crsfDeserialiase(CrsfH *crsfHandler, uint8_t *buff, uint32_t size);

CrsfResult crsfSerialiaseRcChannelsPacked(CrsfH *crsfHandler, uint8_t *buff, uint32_t *size,
                                          CrsfAddress address,
                                          CrsfRcChannelsPacked *payload);

CrsfResult crsfSerialiaseLinkStatistics(CrsfH *crsfHandler, uint8_t *buff, uint32_t *size,
                                        CrsfAddress address,
                                        CrsfLinkStatistic *payload);

#endif
