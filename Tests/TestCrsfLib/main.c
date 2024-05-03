#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "Crsf.h"
#include "CrsfInternal.h"

/*
void printBits(void *inBuff, uint32_t buffSize)
{
    uint8_t *byteBuff = (uint8_t *)inBuff;
    uint8_t bitCnt = 1;

    printf("\n");
    for (uint32_t k = 0; k < buffSize; k++) {
        uint8_t bit = 1;
        for (uint32_t i = 0; i < 8; i++) {
            printf("%3u", bitCnt);
            if (++bitCnt >= 12) {
                printf("  ");
                bitCnt = 1;
            }
            bit <<= 1;
        }
    }
    bitCnt = 1;
    printf("\n");
    for (uint32_t k = 0; k < buffSize; k++) {
        uint8_t bit = 1;
        for (uint32_t i = 0; i < 8; i++) {
            printf("%3u", byteBuff[k] & bit ? 1 : 0);
            if (++bitCnt >= 12) {
                printf("  ");
                bitCnt = 1;
            }
            bit <<= 1;
        }
    }
}

void printBits_(void *inBuff, uint32_t buffSize)
{
    uint8_t *byteBuff = (uint8_t *)inBuff;

    printf("\n");
    for (uint32_t k = 0; k < buffSize; k++) {
        uint8_t bit = 1;
        for (uint32_t i = 0; i < 8; i++) {
            printf("%2u", byteBuff[k] & bit ? 1 : 0);
            bit <<= 1;
        }
    }
}

void testRcPack(void)
{
    CrsfRcChannelsPacketRaw_ temp3;// = {0b11, 0b11, 0b11, 0b11, 0b11};
    uint8_t temp4[] = {0b00000011, 0b00011000, 0b11000000, 0b00000000, 0b00000110, 0b00110000, 0b10000000, 0b00000001, 0b00000000};
    CrsfRcChannelsPacketRaw_ *temp5 = (CrsfRcChannelsPacketRaw_ *)temp4;
    uint16_t temp6;

    temp6 = temp5->ch0;
    printBits_(&temp6, sizeof(temp6));

    temp6 = temp5->ch1;
    printBits_(&temp6, sizeof(temp6));

    temp6 = temp5->ch2;
    printBits_(&temp6, sizeof(temp6));

    temp6 = temp5->ch3;
    printBits_(&temp6, sizeof(temp6));

    temp6 = temp5->ch4;
    printBits_(&temp6, sizeof(temp6));

    temp6 = temp5->ch5;
    printBits_(&temp6, sizeof(temp6));

    temp6 = temp5->ch6;
    printBits_(&temp6, sizeof(temp6));



    memset(&temp3, 0, sizeof(temp3));
    temp3.ch0 = 0b11;
    temp3.ch1 = 0b11;
    temp3.ch2 = 0b11;
    temp3.ch3 = 0b11;
    temp3.ch4 = 0b11;

    //printBits(&temp3, sizeof(temp3));
}
*/

uint32_t rcPacketCnt = 0;
uint32_t linkStatCnt = 0;

#define TEST_DUMP_RC_CHANNEL                        0
#define TEST_DUMP_LINK_STATISTICS                   1
#define TEST_DUMP_LINK_STATISTICS_AND_RC_CHANNEL    2

struct {
    uint32_t size;
    uint8_t buff[56];
} testBuff[] = {
    [TEST_DUMP_RC_CHANNEL] = {
        .size = 26,
        .buff = {0xEE, 0x18, 0x16, 0xDC, 0x93, 0x5E, 0x2B, 0xAC, 0xD7, 0x8A, 0x89, 0x83, 0xAF, 0x15, 0xE0, 0x03, 0x1F, 0xF8, 0xC0, 0x07, 0x3E, 0xF0, 0x81, 0x0F, 0x7C, 0x8A}
    },
    [TEST_DUMP_LINK_STATISTICS] = {
        .size = 14,
        .buff = {0xEA, 0x0C, 0x14, 0xB8, 0x00, 0x64, 0x03, 0x00, 0x06, 0x03, 0xB0, 0x64, 0x03, 0x54},
    },
    [TEST_DUMP_LINK_STATISTICS_AND_RC_CHANNEL] = {
        .size = 40,
        .buff = {0xEE, 0x18, 0x16, 0xDC, 0xA3, 0xDE, 0x2D, 0xA8, 0xD7, 0x8A, 0x89, 0x83, 0xAF, 0x15, 0xE0, 0x03, 0x1F, 0xF8, 0xC0, 0x07, 0x3E, 0xF0, 0x81, 0x0F, 0x7C, 0x4C,
                 0xEA, 0x0C, 0x14, 0xB8, 0x00, 0x64, 0x03, 0x00, 0x06, 0x03, 0xB0, 0x64, 0x03, 0x54},
    }
};

uint8_t serialiaseBuff[32];
uint32_t serialiaseFrameSize;

CrsfFrame crsfTestFrame;
CrsfAddress crsfTestAddress;

void rxFrame(CrsfAddress address, CrsfFrame *frame)
{
    crsfTestFrame = *frame;
    crsfTestAddress = address;

    switch (frame->type) {
    case CRSF_RC_CHANNELS_PACKED:
        printf("I: Rx CRSF_RC_CHANNELS_PACKED, address: %u\n", address);
        rcPacketCnt++;
        break;

    case CRSF_LINK_STATISTICS:
        printf("I: Rx CRSF_LINK_STATISTIC, address: %u\n", address);
        linkStatCnt++;
        break;

    default:
        break;
    }
}

bool testRcPacket(void)
{
    CrsfResult result;
    CrsfH handler;
    CrsfCb crsfCb = {
        .rxFrame = rxFrame,
    };

    result = crsfInit(&handler, crsfCb);
    if (result != CRSF_OK) {
        printf("E: Crsf init %u\n", result);
        return false;
    }

    rcPacketCnt = 0;
    linkStatCnt = 0;
    result = crsfDeserialiase(&handler, testBuff[TEST_DUMP_RC_CHANNEL].buff, testBuff[TEST_DUMP_RC_CHANNEL].size);
    if (result != CRSF_OK) {
        printf("E: Crsf deserialiase RC channel %u\n", result);
        return false;
    }

    serialiaseFrameSize = sizeof(serialiaseBuff);
    result = crsfSerialiaseRcChannelsPacked(&handler, serialiaseBuff, &serialiaseFrameSize,
                                            crsfTestAddress,
                                            &crsfTestFrame.payload.rcChannelsPacked);
    if (result != CRSF_OK) {
        printf("E: Crsf serialiase RC channel %u\n", result);
        return false;
    }
    if (serialiaseFrameSize != testBuff[TEST_DUMP_RC_CHANNEL].size) {
        printf("E: Crsf serialiase RC channel frame size\n");
        return false;
    }
    if (memcmp(serialiaseBuff, testBuff[TEST_DUMP_RC_CHANNEL].buff, serialiaseFrameSize) != 0) {
        printf("E: Crsf serialiase RC channel frame\n");
        return false;
    }
}

bool testLinkStatistics(void)
{
    CrsfResult result;
    CrsfH handler;
    CrsfCb crsfCb = {
        .rxFrame = rxFrame,
    };

    result = crsfInit(&handler, crsfCb);
    if (result != CRSF_OK) {
        printf("E: Crsf init %u\n", result);
        return false;
    }

    rcPacketCnt = 0;
    linkStatCnt = 0;
    result = crsfDeserialiase(&handler, testBuff[TEST_DUMP_LINK_STATISTICS].buff, testBuff[TEST_DUMP_LINK_STATISTICS].size);
    if (result != CRSF_OK) {
        printf("E: Crsf deserialiase Link statistiscs %u\n", result);
        return false;
    }

    serialiaseFrameSize = sizeof(serialiaseBuff);
    result = crsfSerialiaseLinkStatistics(&handler, serialiaseBuff, &serialiaseFrameSize,
                                          crsfTestAddress,
                                          &crsfTestFrame.payload.linkStatistic);
    if (result != CRSF_OK) {
        printf("E: Crsf serialiase Link statistiscs %u\n", result);
        return false;
    }
    if (serialiaseFrameSize != testBuff[TEST_DUMP_LINK_STATISTICS].size) {
        printf("Crsf serialiase Link statistiscs frame size\n");
        return false;
    }
    if (memcmp(serialiaseBuff, testBuff[TEST_DUMP_LINK_STATISTICS].buff, serialiaseFrameSize) != 0) {
        printf("E: Crsf serialiase Link statistiscs frame\n");
        return false;
    }

    return true;
}

bool testRcChannelAndLinkStatistics(void)
{
    CrsfResult result;
    CrsfH handler;
    CrsfCb crsfCb = {
        .rxFrame = rxFrame,
    };

    result = crsfInit(&handler, crsfCb);
    if (result != CRSF_OK) {
        printf("E: Crsf init %u\n", result);
        return false;
    }

    rcPacketCnt = 0;
    linkStatCnt = 0;
    result = crsfDeserialiase(&handler, testBuff[TEST_DUMP_LINK_STATISTICS_AND_RC_CHANNEL].buff,
                              testBuff[TEST_DUMP_LINK_STATISTICS_AND_RC_CHANNEL].size);
    if (result != CRSF_OK) {
        printf("E: Crsf deserialiase Link statistiscs %u\n", result);
        return false;
    }
}

int main(int argCnt, char *argVal)
{
    testRcPacket();
    testLinkStatistics();
    testRcChannelAndLinkStatistics();

    return 0;
}
