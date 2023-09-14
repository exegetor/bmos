#include <stdint.h>
#include "stdio.h"
#include "x86.h"

/*----------------------------------------------------------------------------*/
void __attribute__((cdecl)) start(uint16_t bootDrive)
{
    uint8_t driveType;
    uint16_t cylinders, heads, sectors;

//    clrscr();
    printf("Entering stage2\n");
    x86_disk_get_drive_params(bootDrive,   &driveType,   &cylinders,   &heads,   &sectors);
    printf(    "drive params: bootDrive=%u, driveType=%u, cylinders=%u, heads=%u, sectors=%u",
                              bootDrive,    driveType,    cylinders,    heads,    sectors);
    for (;;)
        ;
}
