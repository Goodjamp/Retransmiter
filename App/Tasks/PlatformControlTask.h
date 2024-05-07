#ifndef __PLATFORM_CONTROL_TASK__
#define __PLATFORM_CONTROL_TASK__

typedef enum {
    PLATFORM_STATE_CALIBRATE_0,
    PLATFORM_STATE_CALIBRATE_90,
    PLATFORM_STATE_WORK,
} PlatformState;

bool platformControlTaskInit(void);
void platformControlUpdateState(void);

#endif