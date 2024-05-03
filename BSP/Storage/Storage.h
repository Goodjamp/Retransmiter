#ifndef __STRORAGE_H__
#define __STRORAGE_H__

#include <stdbool.h>
#include <stdint.h>

bool storageRead(uint8_t *data, uint32_t dataSize);
bool storageWrite(uint8_t *data, uint32_t dataSize);

#endif