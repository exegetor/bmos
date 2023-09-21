#pragma once

#include <stdint.h>
#include "disk.h"

typedef struct
{
	int      handle;
	bool     isDirectory;
	uint32_t position;  // in bytes
	uint32_t size;      // in bytes
} FAT12_file_descriptor;

typedef struct
{
	uint8_t  name[11];
	uint8_t  attributes;
	uint8_t  _reserved;
	uint8_t  createdTimeTenths;
	uint16_t createdTime;
	uint16_t createdDate;
	uint16_t accessedDate;
	uint16_t firstClusterHigh;
	uint16_t modifiedTime;
	uint16_t modifiedDate;
	uint16_t firstClusterLow;
	uint32_t size;
} __attribute__((packed)) FAT12_file_datablock;

enum FAT12_attributes
{
	FAT12_ATTRIBUTE_READ_ONLY = 0x01,
	FAT12_ATTRIBUTE_HIDDEN    = 0x02,
	FAT12_ATTRIBUTE_SYSTEM    = 0x04,
	FAT12_ATTRIBUTE_VOLUME_ID = 0x08,
	FAT12_ATTRIBUTE_DIRECTORY = 0x10,
	FAT12_ATTRIBUTE_ARCHIVE   = 0x20,
	FAT12_ATTRIBUTE_LFN       = FAT12_ATTRIBUTE_READ_ONLY | FAT12_ATTRIBUTE_HIDDEN | FAT12_ATTRIBUTE_SYSTEM | FAT12_ATTRIBUTE_VOLUME_ID
};

bool FAT12_initialize(DISK* disk);
FAT12_file_descriptor* FAT12_fopen(DISK* disk, const char* path);
uint32_t FAT12_readFile(DISK* disk, FAT12_file_descriptor* file, uint32_t byteCount, void* dataOut);
bool FAT12_readEntry(DISK* disk, FAT12_file_descriptor* file, FAT12_file_datablock* dirEntry);
void FAT12_fclose(FAT12_file_descriptor* file);
