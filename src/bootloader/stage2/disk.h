#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	uint8_t id;
	uint8_t driveType;
	uint16_t cylinders;
	uint16_t heads;
	uint16_t sectors;
} DISK;

bool DISK_initialize(DISK* disk, uint8_t driveNumber);
bool DISK_readSectors(DISK* disk, uint32_t lba, uint8_t sectorsToRead, void* dataOut_lower_mem);
