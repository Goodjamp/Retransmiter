#include "Storage.h"
#include "stm32f4xx_flash.h"
#include <stddef.h>

#define SECTOR       FLASH_Sector_1
#define SECTOR_ADDR  0x08004000U
#define SECTOR_SIZE  (16 * 1024)

#define DATA_MAX_SIZE 512

typedef struct {
	uint32_t magicNumber;
	uint32_t crc;
	uint32_t len;
	uint8_t data[];
} Storage;

static const uint32_t MAGIC_NUMBER = 0xDEADBEEF;

static void flashRead(uint32_t addr, uint8_t *buf, uint32_t size)
{
	for (uint32_t i = 0; i < size; i++) {
        buf[i] = *((uint8_t *)(addr + i));
    }
}

static void flashWrite(uint32_t addr, uint8_t *buf, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        FLASH_ProgramByte(addr + i, buf[i]);
    }
}

static uint32_t crc(uint8_t *buf, uint32_t len)
{
	return 42;
}

bool storageRead(uint8_t *data, uint32_t dataSize)
{
	if (data == NULL || dataSize > DATA_MAX_SIZE) {
		return false;
	}
	
	Storage header = {0};
	flashRead(SECTOR_ADDR, (uint8_t *)&header, sizeof(header));
	
	if ((header.magicNumber != MAGIC_NUMBER) || (header.len != dataSize)) {
		// settings invalid
		return false;
	}
	
	flashRead(SECTOR_ADDR + sizeof(header), data, dataSize);
	return header.crc == crc(data, dataSize);
}

bool storageWrite(uint8_t *data, uint32_t dataSize)
{
	if (data == NULL || dataSize > DATA_MAX_SIZE) {
		return false;
	}
	
	Storage header;
	header.magicNumber = MAGIC_NUMBER;
	header.crc = crc(data, dataSize);
	header.len = dataSize;

    FLASH_Unlock();
    FLASH_EraseSector(SECTOR, VoltageRange_3);
	flashWrite(SECTOR_ADDR, (uint8_t *)&header, sizeof(header));
	flashWrite(SECTOR_ADDR + sizeof(header), data, dataSize);
    FLASH_Lock();
	return true;
}