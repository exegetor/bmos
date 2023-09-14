#include "disk.h"
#include "x86.h"
#include "stdio.h"

/*----------------------------------------------------------------------------*/
bool DISK_Initialize(DISK* disk, uint8_t driveNumber)
{
    uint8_t driveType;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors;

//    disk->id = driveNumber;
    if (!x86_disk_get_drive_params(disk->id, &driveType, &cylinders, &heads, &sectors))
        return false;
    disk->id = driveNumber;
    disk->cylinders = cylinders + 1;
    disk->heads = heads + 1;
    disk->sectors = sectors;
    return true;
}

/*----------------------------------------------------------------------------*/
void DISK_LBA2CHS(DISK* disk, uint32_t lba, uint16_t* cylinderOut, uint16_t* headOut, uint16_t* sectorOut)
{
    // sector = (LBA % sectors per track + 1)
    *sectorOut = lba % disk->sectors + 1;
    // cylinder = (LBA / sectors per track) / heads
    *cylinderOut = (lba / disk->sectors) / disk->heads;
    // head = (LBA / sectors per track) % heads
    *headOut = (lba / disk->sectors) % disk->heads;
}

/*----------------------------------------------------------------------------*/
bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectorsToRead, void* dataOut_lower_mem)
{
    uint16_t cylinder;
    uint16_t head;
    uint16_t sector;

    DISK_LBA2CHS(disk, lba, &cylinder, &head, &sector);
    printf("reading LBA %u (C %u, H %u, S %u)\r\n", lba, cylinder, head, sector);
    for (int i = 0; i < 3; i++) {
        if (x86_disk_read(disk->id, cylinder, head, sector, sectorsToRead, dataOut_lower_mem))
            return true;
        x86_disk_reset(disk->id);
    }
    return false;
}
