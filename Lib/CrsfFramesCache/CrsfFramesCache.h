#ifndef __CRSF_FRAMES_CACHE_H__
#define __CRSF_FRAMES_CACHE_H__

#include <stdbool.h>

#include "Crsf.h"

typedef enum {
    CRSF_FRAME_CACHE_OK = 0,
    CRSF_FRAME_CACHE_ATOMIC_ERROR = 1,
    CRSF_FRAME_CACHE_UNKNOWN_FRAME_ERROR = 2,
    CRSF_FRAME_CACHE_HANDLER_NULL_ERROR = 2,
} CrsfFrameCacheResult;

typedef bool (*CrsfFrameCacheAtomicCb)(bool isTake);

typedef struct {
    CrsfFrameCacheAtomicCb cb;
    struct {
        CrsfAddress address;
        uint32_t timeStamp;
        CrsfRcChannelsPacked payload;
    } rcChannelsPacked;
    struct {
        CrsfAddress address;
        uint32_t timeStamp;
        CrsfLinkStatistic payload;;
    } linkStatistic;
} CrsfFrameCacheH;

CrsfFrameCacheResult crsfFrameCacheInit(CrsfFrameCacheH *handler, CrsfFrameCacheAtomicCb cb);
CrsfFrameCacheResult crsfFrameCacheReset(CrsfFrameCacheH *handler);
CrsfFrameCacheResult crsfFrameCachePush(CrsfFrameCacheH *handler, CrsfAddress address,
                                        CrsfFrameType frameType, void *payload);
CrsfFrameCacheResult crsfFrameCachePop(CrsfFrameCacheH *handler, CrsfAddress *address,
                                       uint32_t *timeStamp, CrsfFrameType frameType, void *frame);

#endif