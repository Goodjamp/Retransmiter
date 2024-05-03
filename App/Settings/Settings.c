#include "Settings.h"
#include <stddef.h>

bool settingsReadOwnIp(uint8_t *ip)
{
    if (ip == NULL) {
        return false;
    }

    ip[0] = 192;
    ip[1] = 168;
    ip[2] = 0;
    ip[3] = 120;
/*
    ip[0] = 192;
    ip[1] = 168;
    ip[2] = 0;
    ip[3] = 20;
*/
    return true;
}

bool settingsReadTransmitterModuleIp(uint8_t *ip)
{
    if (ip == NULL) {
        return false;
    }

    ip[0] = 192;
    ip[1] = 168;
    ip[2] = 0;
    ip[3] = 121;


/*
    ip[0] = 192;
    ip[1] = 168;
    ip[2] = 0;
    ip[3] = 170;
*/
/*
    ip[0] = 94;
    ip[1] = 158;
    ip[2] = 83;
    ip[3] = 30;
*/
    return true;
}