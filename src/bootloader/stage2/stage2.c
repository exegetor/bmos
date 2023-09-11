#include "stdint.h"
#include "stdio.h"
#include "disk.h"
#include "fat.h"

/*----------------------------------------------------------------------------*/
void _cdecl cstart_(uint16_t bootDrive)
{
    printf("Entering stage2\r\n");

    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive)) {
        printf("Disk init error\r\n");
        goto end;
    }
    if (!FAT_Initialize(&disk)) {
        printf("FAT init error\r\n");
        goto end;
    }
    /* browse files in root */
    FAT_File far* fd = FAT_Open(&disk, "/");
    FAT_DirectoryEntry entry;
    int i = 0;
    while (FAT_ReadEntry(&disk, fd, &entry) && i++ < 5) {
        printf("  ");
        for (int i = 0; i < 11; i++)
            putc(entry.Name[i]);
        printf("\r\n");


    }
    FAT_Close(fd);

end:
    for (;;)
        ;
}
