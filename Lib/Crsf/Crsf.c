#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "CrsfInternal.h"
#include "Crsf.h"
#include "crsf_protocol.h"

#define RC_CHANNEL_MASK(X)    (X & 0x7FF)

CrsfResult crsfInit(CrsfH *crsfHandler, CrsfCb cb)
{
    if (crsfHandler == NULL) {
        return CRSF_HANDLER_NULL_ERROR;
    }
    if (cb.crc == NULL) {
        cb.crc = calcCrc;
    }
    crsfHandler->cb = cb;

    return CRSF_OK;
}

static CrsfAddress crsfAddressToUserAddress(uint8_t address)
{
    CrsfAddress userAddress = 0;

    switch (address) {
    case CRSF_ADDRESS_CRSF_RECEIVER:
        userAddress = CrsfAddressCrcsReceiver;
        break;

    case CRSF_ADDRESS_CRSF_TRANSMITTER:
        userAddress = CrsfAddressCrsfTransmiter;
        break;

    case CRSF_ADDRESS_RADIO_TRANSMITTER:
        userAddress = CrsfAddressRadioTransmiter;
        break;

    case CRSF_ADDRESS_CRSF_FLY_CONTROLER:
        userAddress = CrsfAddressFlyControler;
        break;
    }

    return userAddress;
}

static uint8_t userAddressToCrsfAddress(CrsfAddress address)
{
    uint8_t crsfAddress = 0;

    switch (address) {
    case CrsfAddressCrcsReceiver:
        crsfAddress = CRSF_ADDRESS_CRSF_RECEIVER;
        break;

    case CrsfAddressCrsfTransmiter:
        crsfAddress = CRSF_ADDRESS_CRSF_TRANSMITTER;
        break;

    case CrsfAddressRadioTransmiter:
        crsfAddress = CRSF_ADDRESS_RADIO_TRANSMITTER;
        break;

    case CrsfAddressFlyControler:
        crsfAddress = CRSF_ADDRESS_CRSF_FLY_CONTROLER;
        break;
    }

    return crsfAddress;
}

CrsfResult crsfDeserialiase(CrsfH *crsfHandler, uint8_t *buff, uint32_t size)
{
    CrsfHeader *header;
    uint8_t crc;
    bool isFrameFound = false;
    uint32_t k = 0;
    CrsfFrame crsfFrame;
    bool isPacketRx = false;

    if (crsfHandler == NULL) {
        return CRSF_HANDLER_NULL_ERROR;
    }
    if (buff == NULL) {
        return CRSF_BUFF_NULL_ERROR;
    }
    if (size < CRSF_MIN_PAYLOAD_SIZE) {
        return CRSF_BUFF_MIN_SIZE_ERROR;
    }

    while (k < size) {
        header = (CrsfHeader *)&buff[k];
        isFrameFound = false;
        /*
         * Test the input buffer and found the first byte of the frame
         */
        if (header->address == CRSF_ADDRESS_CRSF_RECEIVER
            || header->address == CRSF_ADDRESS_CRSF_TRANSMITTER
            || header->address == CRSF_ADDRESS_RADIO_TRANSMITTER) {
            if (header->frameSize <= CRSF_MAX_PAYLOAD_SIZE
                && header->frameSize >= CRSF_MIN_PAYLOAD_SIZE
                && (k + 2 + header->frameSize) <= size) {
                crc = crsfHandler->cb.crc(&buff[k + CRSF_PAYLOAD_POS], header->frameSize - 1);
                if (crc == buff[k + 1 + header->frameSize]) {
                    switch (header->frameType) {
                    case CRSF_FRAME_RC_CHANNELS_PACKED: {
                        CrsfRcChannelsPacketRaw *channelsPacketRaw = (CrsfRcChannelsPacketRaw *)header->payload;

                        /*
                         * Take a RC channel value
                         */
                        crsfFrame.type = CRSF_RC_CHANNELS_PACKED;
                        crsfFrame.payload.rcChannelsPacked.chValue[0] = channelsPacketRaw->ch0;
                        crsfFrame.payload.rcChannelsPacked.chValue[1] = channelsPacketRaw->ch1;
                        crsfFrame.payload.rcChannelsPacked.chValue[2] = channelsPacketRaw->ch2;
                        crsfFrame.payload.rcChannelsPacked.chValue[3] = channelsPacketRaw->ch3;
                        crsfFrame.payload.rcChannelsPacked.chValue[4] = channelsPacketRaw->ch4;
                        crsfFrame.payload.rcChannelsPacked.chValue[5] = channelsPacketRaw->ch5;
                        crsfFrame.payload.rcChannelsPacked.chValue[6] = channelsPacketRaw->ch6;
                        crsfFrame.payload.rcChannelsPacked.chValue[7] = channelsPacketRaw->ch7;
                        crsfFrame.payload.rcChannelsPacked.chValue[8] = channelsPacketRaw->ch8;
                        crsfFrame.payload.rcChannelsPacked.chValue[9] = channelsPacketRaw->ch9;
                        crsfFrame.payload.rcChannelsPacked.chValue[10] = channelsPacketRaw->ch10;
                        crsfFrame.payload.rcChannelsPacked.chValue[11] = channelsPacketRaw->ch11;
                        crsfFrame.payload.rcChannelsPacked.chValue[12] = channelsPacketRaw->ch12;
                        crsfFrame.payload.rcChannelsPacked.chValue[13] = channelsPacketRaw->ch13;
                        crsfFrame.payload.rcChannelsPacked.chValue[14] = channelsPacketRaw->ch14;
                        crsfFrame.payload.rcChannelsPacked.chValue[15] = channelsPacketRaw->ch15;

                        isPacketRx = true;

                        break;
                    }

                    case CRSF_FRAME_LINK_STATISTICS: {
                        CrsfLinkStatisticsRaw *linkStatistics = (CrsfLinkStatisticsRaw *) header->payload;

                        crsfFrame.type = CRSF_LINK_STATISTICS;
                        crsfFrame.payload.linkStatistic.uplinkRssi1 = linkStatistics->uplinkRssi1;
                        crsfFrame.payload.linkStatistic.uplinkRssi2 = linkStatistics->uplinkRssi2;
                        crsfFrame.payload.linkStatistic.uplinkLinkQuality = linkStatistics->uplinkLinkQuality;
                        crsfFrame.payload.linkStatistic.uplinkSnr = linkStatistics->uplinkSnr;
                        crsfFrame.payload.linkStatistic.activeAntenna = linkStatistics->activeAntenna;
                        crsfFrame.payload.linkStatistic.rfMode = linkStatistics->rfMode;
                        crsfFrame.payload.linkStatistic.uplinkTxPower = linkStatistics->uplinkTxPower;
                        crsfFrame.payload.linkStatistic.downlinkRssi = linkStatistics->downlinkRssi;
                        crsfFrame.payload.linkStatistic.downlinkLinkQuality = linkStatistics->downlinkLinkQuality;
                        crsfFrame.payload.linkStatistic.downlinkSnr = linkStatistics->downlinkSnr;

                        isPacketRx = true;

                        break;
                    }

                    default: // unknown command
                        break;
                    }

                    /*
                     * Notify external code about receive new CRSF packet
                     */
                    if (crsfHandler->cb.rxFrame != NULL && isPacketRx == true) {
                        crsfHandler->cb.rxFrame(crsfAddressToUserAddress(header->address),
                                                &crsfFrame);
                    }
                    isPacketRx = false;
                    /*
                     * Frame was found. Set this flag to jump to the
                     * input data index after current frame
                     */
                    isFrameFound = true;
                }
            }
        }
        k += isFrameFound ? (2 + header->frameSize) : 1;
    }

    return CRSF_OK;
}

CrsfResult crsfSerialiaseRcChannelsPacked(CrsfH *crsfHandler, uint8_t *buff, uint32_t *size,
                                          CrsfAddress address,
                                          CrsfRcChannelsPacked *payload)
{
    CrsfRcChannelsPacketRaw channelsPacketRaw;
/*
 * The channel size inclue: size of Header, size of payload, size of CRC
 */
#define RC_CHANNELS_PACKED_FRAME_SIZE    (sizeof(CrsfHeader) + sizeof(CrsfRcChannelsPacketRaw) + CRSF_CRC_SIZE)
#define RC_CHANNELS_PACKED_CRC_POS       (RC_CHANNELS_PACKED_FRAME_SIZE - 1)

    if (crsfHandler == NULL) {
        return CRSF_HANDLER_NULL_ERROR;
    }
    if (buff == NULL) {
        return CRSF_BUFF_NULL_ERROR;
    }
    if (*size < RC_CHANNELS_PACKED_FRAME_SIZE) {
        return CRSF_BUFF_MIN_SIZE_ERROR;
    }
    if (payload == NULL) {
        return CRSF_PAYLOAD_NULL_ERROR;
    }

    *size = RC_CHANNELS_PACKED_FRAME_SIZE;

    buff[STR_FIELD_POS(CrsfHeader, address)] = userAddressToCrsfAddress(address);
    if (buff[STR_FIELD_POS(CrsfHeader, address)] == 0) {
        return CRSF_ADDRESS_ERROR;
    }

    buff[STR_FIELD_POS(CrsfHeader, frameSize)] = FIELD_FRAME_SIZE(RC_CHANNELS_PACKED_FRAME_SIZE);
    buff[STR_FIELD_POS(CrsfHeader, frameType)] = CRSF_FRAME_RC_CHANNELS_PACKED;
    channelsPacketRaw.ch0 = RC_CHANNEL_MASK(payload->chValue[0]);
    channelsPacketRaw.ch1 = RC_CHANNEL_MASK(payload->chValue[1]);
    channelsPacketRaw.ch2 = RC_CHANNEL_MASK(payload->chValue[2]);
    channelsPacketRaw.ch3 = RC_CHANNEL_MASK(payload->chValue[3]);
    channelsPacketRaw.ch4 = RC_CHANNEL_MASK(payload->chValue[4]);
    channelsPacketRaw.ch5 = RC_CHANNEL_MASK(payload->chValue[5]);
    channelsPacketRaw.ch6 = RC_CHANNEL_MASK(payload->chValue[6]);
    channelsPacketRaw.ch7 = RC_CHANNEL_MASK(payload->chValue[7]);
    channelsPacketRaw.ch8 = RC_CHANNEL_MASK(payload->chValue[8]);
    channelsPacketRaw.ch9 = RC_CHANNEL_MASK(payload->chValue[9]);
    channelsPacketRaw.ch10 = RC_CHANNEL_MASK(payload->chValue[10]);
    channelsPacketRaw.ch11 = RC_CHANNEL_MASK(payload->chValue[11]);
    channelsPacketRaw.ch12 = RC_CHANNEL_MASK(payload->chValue[12]);
    channelsPacketRaw.ch13 = RC_CHANNEL_MASK(payload->chValue[13]);
    channelsPacketRaw.ch14 = RC_CHANNEL_MASK(payload->chValue[14]);
    channelsPacketRaw.ch15 = RC_CHANNEL_MASK(payload->chValue[15]);
    memcpy(&buff[STR_FIELD_POS(CrsfHeader, payload)], &channelsPacketRaw, sizeof(channelsPacketRaw));
    buff[RC_CHANNELS_PACKED_CRC_POS] =
        crsfHandler->cb.crc(&buff[STR_FIELD_POS(CrsfHeader, frameType)],
                            CRC_SIZE(RC_CHANNELS_PACKED_FRAME_SIZE));

    return CRSF_OK;
}

CrsfResult crsfSerialiaseLinkStatistics(CrsfH *crsfHandler, uint8_t *buff, uint32_t *size,
                                        CrsfAddress address,
                                        CrsfLinkStatistic *payload)
{
/*
 * The channel size inclue: size of Header, size of payload, size of CRC
 */
#define LINK_STATISTICS_FRAME_SIZE    (sizeof(CrsfHeader) + sizeof(CrsfLinkStatisticsRaw) + CRSF_CRC_SIZE)
#define LINK_STATISTICS_CRC_POS       (LINK_STATISTICS_FRAME_SIZE - 1)
    if (crsfHandler == NULL) {
        return CRSF_HANDLER_NULL_ERROR;
    }
    if (buff == NULL) {
        return CRSF_BUFF_NULL_ERROR;
    }
    if (*size < LINK_STATISTICS_FRAME_SIZE) {
        return CRSF_BUFF_MIN_SIZE_ERROR;
    }
    if (payload == NULL) {
        return CRSF_PAYLOAD_NULL_ERROR;
    }

    *size = LINK_STATISTICS_FRAME_SIZE;

    buff[STR_FIELD_POS(CrsfHeader, address)] = userAddressToCrsfAddress(address);
    if (buff[STR_FIELD_POS(CrsfHeader, address)] == 0) {
        return CRSF_ADDRESS_ERROR;
    }

    buff[STR_FIELD_POS(CrsfHeader, frameSize)] = FIELD_FRAME_SIZE(LINK_STATISTICS_FRAME_SIZE);
    buff[STR_FIELD_POS(CrsfHeader, frameType)] = CRSF_FRAME_LINK_STATISTICS;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, uplinkRssi1)] =
        payload->uplinkRssi1;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, uplinkRssi2)] =
        payload->uplinkRssi2;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, uplinkLinkQuality)] =
        payload->uplinkLinkQuality;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, uplinkSnr)] =
        payload->uplinkSnr;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, activeAntenna)] =
        payload->activeAntenna;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, rfMode)] =
        payload->rfMode;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, uplinkTxPower)] =
        payload->uplinkTxPower;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, downlinkRssi)] =
        payload->downlinkRssi;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, downlinkLinkQuality)] =
        payload->downlinkLinkQuality;
    buff[STR_FIELD_POS(CrsfHeader, payload) + STR_FIELD_POS(CrsfLinkStatisticsRaw, downlinkSnr)] =
        payload->downlinkSnr;

    buff[LINK_STATISTICS_CRC_POS] =
        crsfHandler->cb.crc(&buff[STR_FIELD_POS(CrsfHeader, frameType)],
                            CRC_SIZE(LINK_STATISTICS_FRAME_SIZE));

    return CRSF_OK;
}