#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdint.h>
#include <stdbool.h>

bool settingsReadOwnIp(uint8_t *ip);
bool settingsReadTransmitterModuleIp(uint8_t *ip);

#endif