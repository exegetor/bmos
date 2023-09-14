#pragma once

/*******************************************************************************
** 0x0000:0000 - 0x0000:03ff - interrupt vector table
** 0x0000:0400 - 0x0000:04ff - BIOS data area
*/
#define MEMORY_MIN                   0x00000500
#define MEMORY_MAX                   0x00080000

/* 0000:0500 - 0001:0500 - FAT driver */
#define MEMORY_FAT_ADDR  (void*)0x20000
#define MEMORY_FAT_SIZE         0x10000

/* 0002:0000 - 0003:0000 - stage2 */

/* 0003:0000 - 0008:0000 - free */

/*******************************************************************************
** 0x0008:0000 - 0x0009:ffff - extended BIOS data areaa
** 0x000A:0000 - 0x000C:7fff - Video
** 0x000C:8000 - 0x000f:ffff - BIOS
*/

