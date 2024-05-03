#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

#include "Crsf.h"
#include "CrsfFramesCache.h"

/*
 * To guarantee obsolete data at the start-up or after reset, we set a
 * minimum of 10 minutes age of data.
 */
#define CRSF_FRAMES_CACHE_RESET_TIMEOUTE    (0xFFFFFFFF - pdMS_TO_TICKS(10 * 60 * 1000))

CrsfFrameCacheResult crsfFrameCacheInit(CrsfFrameCacheH *handler, CrsfFrameCacheAtomicCb cb)
{
    if (handler == NULL) {
        return CRSF_FRAME_CACHE_HANDLER_NULL_ERROR;
    }

    handler->cb = cb;
    crsfFrameCacheReset(handler);

    return CRSF_FRAME_CACHE_OK;
}

CrsfFrameCacheResult crsfFrameCacheReset(CrsfFrameCacheH *handler)
{
    int result = CRSF_FRAME_CACHE_OK;

    if (handler->cb(true) == true) {
        /*
        * To guarantee obsolete data at the start-up or after reset, we set a
        * minimum of 10 minutes age of data.
        */
        handler->rcChannelsPacked.timeStamp = CRSF_FRAMES_CACHE_RESET_TIMEOUTE;
        handler->linkStatistic.timeStamp = CRSF_FRAMES_CACHE_RESET_TIMEOUTE;

        handler->cb(false);
    } else {
        result = CRSF_FRAME_CACHE_ATOMIC_ERROR;
    }

    return result;
}

CrsfFrameCacheResult crsfFrameCachePush(CrsfFrameCacheH *handler, CrsfAddress address,
                                        CrsfFrameType frameType, void *payload)
{
    int result = CRSF_FRAME_CACHE_OK;

    if (handler->cb(true) == true) {
        switch (frameType) {
        case CRSF_RC_CHANNELS_PACKED:
            handler->rcChannelsPacked.address = address;
            handler->rcChannelsPacked.timeStamp = xTaskGetTickCount();
            memcpy(&handler->rcChannelsPacked.payload, payload,
                   sizeof(CrsfRcChannelsPacked));
            break;

        case CRSF_LINK_STATISTICS:
            handler->linkStatistic.address = address;
            handler->linkStatistic.timeStamp = xTaskGetTickCount();
            memcpy(&handler->linkStatistic.payload, payload,
                   sizeof(CrsfLinkStatistic));
            break;

        default:
            result = CRSF_FRAME_CACHE_UNKNOWN_FRAME_ERROR;
        }
        handler->cb(false);
    } else {
        result = CRSF_FRAME_CACHE_ATOMIC_ERROR;
    }

    return result;
}

CrsfFrameCacheResult crsfFrameCachePop(CrsfFrameCacheH *handler, CrsfAddress *address,
                                     uint32_t *timeStamp, CrsfFrameType frameType, void *frame)
{
    int result = CRSF_FRAME_CACHE_OK;

    if (handler->cb(true) == true) {

        switch (frameType) {
        case CRSF_RC_CHANNELS_PACKED:
            *address = handler->rcChannelsPacked.address;
            *timeStamp = handler->rcChannelsPacked.timeStamp;
            memcpy(frame, &handler->rcChannelsPacked.payload,
                   sizeof(CrsfRcChannelsPacked));
            break;

        case CRSF_LINK_STATISTICS:
            *address = handler->linkStatistic.address;
            *timeStamp = handler->linkStatistic.timeStamp;
            memcpy(frame, &handler->linkStatistic.payload,
                   sizeof(CrsfLinkStatistic));
            break;
        default:
            result = CRSF_FRAME_CACHE_UNKNOWN_FRAME_ERROR;
        }

        handler->cb(false);
    } else {
        result = CRSF_FRAME_CACHE_ATOMIC_ERROR;
    }

    return result;
}