#include <stdint.h>
#include "stdio.h"
#include "x86.h"
#include "disk.h"
#include "fat12.h"

/*----------------------------------------------------------------------------*/
//void* g_Data = (void*)0x60000;



/*----------------------------------------------------------------------------*/
void __attribute__((cdecl)) start(uint16_t bootDrive)
{
//	uint8_t driveType;
//	uint16_t cylinders, heads, sectors;
	DISK disk;

	clrscr();
	printf("Entering stage2\r\n");

//	if (!x86_disk_get_drive_params(bootDrive,   &driveType,   &cylinders,   &heads,   &sectors)) {
//		goto end;
//	}
//	printf("Drive ID: %u, type %u, cyl %u, heaads %u, secs %u\r\n", bootDrive, driveType, cylinders, heads, sectors);
//	for (;;);
//

	if (!DISK_initialize(&disk, bootDrive)) {
		printf("DISK_initialize() failed\r\n");
		goto end;
	}

/*
	if (!DISK_readSectors(&disk, 0, 1, g_Data)) {
		printf("DISK_readSectors failed\r\n");
		goto end;
	}
	printf("Boot sector:\r\n");
	print_buffer("", g_Data, 512);
*/
	if (!FAT12_initialize(&disk)) {
		printf("FAT12_initialize() failed\r\n");
		goto end;
	}

	// browse files in root
	printf("--------------------------\r\n");
	FAT12_file_descriptor* fd = FAT12_fopen(&disk, "/");
	FAT12_file_datablock entry;
	int count = 0;
	while (FAT12_readEntry(&disk, fd, &entry) && count++ < 5) {
//		printf("                       ");
		for (int i = 0; i < 11;  i++) {
			putc_color(entry.name[i], COLOR_RED);
		}
		printf("   size: %u,  1st cluster (Low): %u; (High): %u\r\n", entry.size, entry.firstClusterLow, entry.firstClusterHigh);
		printf("\r\n");
	}
	FAT12_fclose(fd);
	printf("--------------------------\r\n");

	// read test.txt
	char buffer[100];
	uint32_t read;
	fd = FAT12_fopen(&disk, "test.txt");
	if (!fd) {
		printf("FAT12_fopen(/test.txt) failed\r\n");
	}
	else {
		printf("Opened text.txt/  position= %u, size= %u\r\n", fd->position, fd->size);
	}

//	read = FAT12_readFile(&disk, fd, sizeof(buffer), buffer);
//	printf("bytes read = %u\r\n", read);
//	for (uint32_t i = 0; i < read; i++) {
//		if (buffer[i] == '\n')
//			putc('\r');
//		putc_color(buffer[i], COLOR_RED);
//	}
	while ((read = FAT12_readFile(&disk, fd, sizeof(buffer), buffer))) {
		printf("bytes read = %u\r\n", read);
		for (uint32_t i = 0; i < read; i++) {
			if (buffer[i] == '\n')
				putc('\r');
			putc_color(buffer[i], COLOR_RED);
		}
	}
	printf("bytes read = %u\r\n", read); 
	FAT12_fclose(fd);

end:
	for (;;)
		;
}
