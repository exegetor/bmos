#include <stdint.h>
#include "stdio.h"
#include "x86.h"
#include "disk.h"

/*----------------------------------------------------------------------------*/
void* g_Data = (void*)0x60000;

/*----------------------------------------------------------------------------*/
void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    uint8_t driveType;
    uint16_t cylinders, heads, sectors;
    DISK disk;

    clrscr();
    printf("Entering stage2\r\n");
    if (!x86_disk_get_drive_params(bootDrive,   &driveType,   &cylinders,   &heads,   &sectors)) {
        goto end;
    };
    printf(    "drive params: bootDrive=%u, driveType=%u, cylinders=%u, heads=%u, sectors=%u\r\n",
                              bootDrive,    driveType,    cylinders,    heads,    sectors);

    if (!DISK_Initialize(&disk, bootDrive)) {
        printf("Disk init error\r\n");
        goto end;
    }
    DISK_ReadSectors(&disk, 0, 1, g_Data);
    printf("Boot sector:\r\n");
    print_buffer("", g_Data, 512);

end:
    for (;;)
        ;
}
