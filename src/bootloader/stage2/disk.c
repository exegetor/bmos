#include <stdint.h>
#include "x86.h"
#include "stdio.h"
#include "logging.h"
#include "disk.h"


/*----------------------------------------------------------------------------*/
bool
DISK_initialize
(DISK* disk, uint8_t driveNumber)
{
	uint8_t  driveType;
	uint16_t cylinders;
	uint16_t heads;
	uint16_t sectors;

	TRACE("%s()\r\n", __FUNCTION__);

	disk->id = driveNumber;
	if (!x86_disk_get_drive_params(disk->id, &driveType, &cylinders, &heads, &sectors)) {
		WARN("x86_disk_get_drive_params() failed!\r\n");
		return false;
	}
	disk->driveType = driveType;
	disk->cylinders = cylinders;
	disk->heads = heads;
	disk->sectors = sectors;
	return true;
}/* DISK_initialize() */


/*----------------------------------------------------------------------------*/
void
DISK_LBA2CHS
(DISK* disk, uint32_t lba, uint16_t* cylinderOut, uint16_t* headOut, uint16_t* sectorOut)
{
	TRACE("%s()\r\n", __FUNCTION__);

	// sector = (LBA % sectors per track + 1)
	*sectorOut = lba % disk->sectors + 1;
	// cylinder = (LBA / sectors per track) / heads
	*cylinderOut = (lba / disk->sectors) / disk->heads;
	// head = (LBA / sectors per track) % heads
	*headOut = (lba / disk->sectors) % disk->heads;

	DEBUG("lba %u, cyl %u, head %u, sector %u\r\n", lba, *cylinderOut, *headOut, *sectorOut);
}/* DISK_LBA2CHS() */


/*----------------------------------------------------------------------------*/
bool
DISK_readSectors
(DISK* disk, uint32_t lba, uint8_t sectorsToRead, void* dataOut_lower_mem)
{
	uint16_t cylinder;
	uint16_t head;
	uint16_t sector;

	TRACE("%s()\r\n", __FUNCTION__);

	DISK_LBA2CHS(disk, lba, &cylinder, &head, &sector);

	DEBUG("reading LBA %u (C %u, H %u, S %u)\r\n", lba, cylinder, head, sector);
	DEBUG("buffer at %u\r\n", dataOut_lower_mem);
//	if (dataOut_lower_mem > 1000000)
//		for (;;);

	int i;
	for (i = 0; i < 3; i++) {
		if (x86_disk_read(disk->id, cylinder, head, sector, sectorsToRead, dataOut_lower_mem)) {
			return true;
		}
		INFO("resetting disk...\r\n");
		x86_disk_reset(disk->id);
	}
	WARN("x86_disk_read() faied %u times!\r\n", i);
	return false;
}/* DISK_readSectors() */
