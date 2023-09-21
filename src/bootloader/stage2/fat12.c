#include <stddef.h>
#include "stdio.h"
#include "stdint.h"
#include "memdefs.h"
//#include "utility.h"
#include "string.h"
#include "memory.h"
#include "ctype.h"
#include "minmax.h"
#include "fat12.h"

#define SECTOR_SIZE    512
#define MAX_PATH_SIZE  256
#define MAX_FILE_HANDLES 10
#define ROOT_DIRECTORY_HANDLE  -1
#define FINAL_CLUSTER_MARK 0xFF8

typedef struct
{
	uint8_t  bootJumpInstruction[3];
	uint8_t  OEMIdentifier[8];
	uint16_t bytesPerSector;
	uint8_t  sectorsPerCluster;
	uint16_t reservedSectors;
	uint8_t  FATCount;
	uint16_t dirEntryCount;
	uint16_t totalSectors;
	uint8_t  mediaDescriptorType;
	uint16_t sectorsPerFAT;
	uint16_t sectorsPerTrack;
	uint16_t heads;
	uint32_t hiddenSectors;
	uint32_t largeSectorCount;

	// extended boot record
	uint8_t  driveNumber;
	uint8_t  _reserved;
	uint8_t  signature;
	uint32_t volumeID;         // serial number, value doesn't matter
	uint8_t  volumeLabel[11];  // 11 bytes, padded with spaces
	uint8_t  systemID[8];
} __attribute__((packed)) FAT12_BS_datablock;

typedef struct
{
	FAT12_file_descriptor fd;
	bool     opened;
	uint32_t firstCluster;
	uint32_t currentCluster;
	uint32_t currentSectorInCluster;
	uint8_t  buffer[SECTOR_SIZE];
} FAT12_FileData;

typedef struct
{
	union {
		FAT12_BS_datablock BS_datablock;
		uint8_t bootSectorBytes[SECTOR_SIZE];
	} BS;
	FAT12_FileData rootDirectory;
	FAT12_FileData openedFiles[MAX_FILE_HANDLES];
} FAT12_Data;

/* this won't work when we have huge FATs, so
** we will need to implement caching later */
static FAT12_Data* g_BS_Data;         // points to MEMORY_FAT_ADDR
static uint8_t*    g_FAT = NULL;      // points just after struct above ^ this
static uint32_t    g_DataSectionLBA;

/*------------------------------------------------------------------------------
** PRIVATE FUNCTIONS / FORWARD DECLARATIONS
*/
bool FAT12_readBootSector(DISK* disk);
bool FAT12_readAllocationTable(DISK* disk);
bool FAT12_findFile(DISK* disk, FAT12_file_descriptor* file, const char* name, FAT12_file_datablock* entryOut);
FAT12_file_descriptor* FAT12_openFATEntry(DISK* disk, FAT12_file_datablock* entry);
uint32_t FAT12_nextCluster(uint32_t currentCluster);
uint32_t FAT12_clusterToLBA(uint32_t cluster);


/*----------------------------------------------------------------------------*/
bool
FAT12_initialize
(DISK* disk)
{
#ifdef DEBUG
	printf("%s()\r\n", __FUNCTION__);
#endif
	g_BS_Data = (FAT12_Data*)MEMORY_FAT12_ADDR;

	/* read boot sector */
	if (!FAT12_readBootSector(disk)) {
		printf("FAT: read boot sector failed\r\n");
		return false;
	}

	/* read FAT */
	g_FAT = (uint8_t*)g_BS_Data + sizeof(FAT12_Data);
	uint32_t FATSize = g_BS_Data->BS.BS_datablock.bytesPerSector 
	                 * g_BS_Data->BS.BS_datablock.sectorsPerFAT;
	if (sizeof(FAT12_Data) + FATSize >= MEMORY_FAT12_SIZE) {
		printf("FAT: not enough memory to read FAT! Required %lu, only have %u\r\n", sizeof(FAT12_Data) + FATSize, MEMORY_FAT12_SIZE);
		return false;
	}
	if (!FAT12_readAllocationTable(disk)) {
		printf("FAT: read FAT failed\r\n");
		return false;
	}

	/* read root directory */
	uint32_t rootDirLBA =  g_BS_Data->BS.BS_datablock.reservedSectors
	                    + (g_BS_Data->BS.BS_datablock.FATCount * g_BS_Data->BS.BS_datablock.sectorsPerFAT);
	uint32_t rootDirSize = g_BS_Data->BS.BS_datablock.dirEntryCount 
	                     * sizeof(FAT12_file_datablock);
//	rootDirSize = align(rootDirSize, g_BS_Data->BS.BS_datablock.BytesPerSector);
    
/*
	if (sizeof(FAT_Data) + FATSize + rootDirSize >= MEMORY_FAT_SIZE) {
		printf("FAT: not enough memory to read root directory! Required %lu, only have %u\r\n", sizeof(FAT_Data) + FATSize + rootDirSize, MEMORY_FAT_SIZE);
		return false;
	}
	if (!FAT_ReadRootDirectory(disk)) {
		printf("FAT: read root directory failed\r\n");
		return false;
	}
*/
	/* open root directory file */
	g_BS_Data->rootDirectory.fd.handle = ROOT_DIRECTORY_HANDLE;
	g_BS_Data->rootDirectory.fd.isDirectory = true;
	g_BS_Data->rootDirectory.fd.position = 0;
	g_BS_Data->rootDirectory.fd.size = g_BS_Data->BS.BS_datablock.dirEntryCount
	                                 * sizeof(FAT12_file_datablock);
	g_BS_Data->rootDirectory.opened = true;
	g_BS_Data->rootDirectory.firstCluster = rootDirLBA;
	g_BS_Data->rootDirectory.currentCluster = rootDirLBA;
	g_BS_Data->rootDirectory.currentSectorInCluster = 0;

	if (!DISK_readSectors(disk, rootDirLBA, 1, g_BS_Data->rootDirectory.buffer)) {
		printf("FAT: read root directory failed\r\n");
		return false;
	}

	/* calculate data section */
	uint32_t rootDirSectors = (rootDirSize + g_BS_Data->BS.BS_datablock.bytesPerSector - 1)
	                        / g_BS_Data->BS.BS_datablock.bytesPerSector;
	g_DataSectionLBA = rootDirLBA + rootDirSectors;

	/* reset opened files */
	for (int i = 0; i < MAX_FILE_HANDLES; i++)
		g_BS_Data->openedFiles[i].opened = false;
	return true;
}/* FAT12_initialize() */

/*----------------------------------------------------------------------------*/
bool
FAT12_readBootSector
(DISK* disk)
{
#ifdef DEBUG
	printf("%s()\r\n", __FUNCTION__);
#endif
	return DISK_readSectors(disk,
	                        0,
	                        1,
	                        g_BS_Data->BS.bootSectorBytes);
}

/*----------------------------------------------------------------------------*/
bool
FAT12_readAllocationTable
(DISK* disk)
{
#ifdef DEBUG
	printf("%s()\r\n", __FUNCTION__);
#endif
	return DISK_readSectors(disk,
	                        g_BS_Data->BS.BS_datablock.reservedSectors,
	                        g_BS_Data->BS.BS_datablock.sectorsPerFAT,
	                        g_FAT);
}

/*----------------------------------------------------------------------------*/
FAT12_file_descriptor*
FAT12_fopen
(DISK* disk, const char* path)
{
#ifdef DEBUG
	printf("%s()\r\n", __FUNCTION__);
#endif

	char name[MAX_PATH_SIZE];
	memset(name, ' ', MAX_PATH_SIZE);

	/* ignore leading slash */
	if (path[0] == '/')
		path++;

	FAT12_file_descriptor* current = &g_BS_Data->rootDirectory.fd;

	while (*path) {
		/* extract next file name from path */
		bool isLast = false;
		const char* delim = strchr(path, '/');
		if (delim != NULL) {
			memcpy(name, path, delim - path);
			name[delim - path + 1] = '\0';
			path = delim + 1;
		}
		else {
			unsigned len = strlen(path);
			memcpy(name, path, len);
			name[len + 1] = '\0';
			path += len;
			isLast = true;
		}

		/* find directory entry in current directory */
		FAT12_file_datablock entry;
		if (FAT12_findFile(disk, current, name, &entry)) {
			FAT12_fclose(current);
			/* check is directory */
			if (!isLast && ((entry.attributes & FAT12_ATTRIBUTE_DIRECTORY) == 0)) {
				printf("FAT: %s not a directory\r\n", name);
				return NULL;
			}
			/* open new directory entry */
			current = FAT12_openFATEntry(disk, &entry);
		}
		else {
			printf("FAT: %s not found\r\n", name);
			FAT12_fclose(current);
			return NULL;
		}
	}/* while(path) */
	return current;
}/* FAT12_fopen() */

/*----------------------------------------------------------------------------*/
bool
FAT12_findFile
(DISK* disk, FAT12_file_descriptor* file, const char* name, FAT12_file_datablock* entryOut)
{
#ifdef DEBUG
	printf("%s(%s)\r\n", __FUNCTION__, name);
#endif

	char FATName[12];
	FAT12_file_datablock entry;

	/*convert name to FATName */
	memset(FATName, ' ', sizeof(FATName));
	FATName[11] = '\0';
	/* TODO: fix this.  won't handle more than one dot in filename */
	const char* ext = strchr(name, '.');
	if (ext == NULL)
		ext = name + 11;
	for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
		FATName[i] = toupper(name[i]);
	if (ext != name + 11) {
		for (int i = 0; i < 3 && ext[i + 1]; i++)
			FATName[i + 8] = toupper(ext[i + 1]);
	}

	while (FAT12_readEntry(disk, file, &entry)) {
//		printf("FATName %s, name %s, entry.name %s\r\n",FATName, name, entry.name);
		if (memcmp(FATName, entry.name, 11) == 0) {
			*entryOut = entry;
		#ifdef DEBUG
			printf("FAT12_findFile(): found file  %s\r\n", name);
		#endif
			return true;
		}
	}
#ifdef DEBUG
	printf("FAT12_findFile(): File not found: %s\r\n", name);
#endif
	return false;
}/* FAT12_findFile() */

/*----------------------------------------------------------------------------*/
bool
FAT12_readEntry
(DISK* disk, FAT12_file_descriptor* file, FAT12_file_datablock* dirEntry)
{
#ifdef DEBUG
//	printf("%s(%s)\t", __FUNCTION__, dirEntry->name);
#endif
	return FAT12_readFile(disk,
	                      file,
	                      sizeof(FAT12_file_datablock),
	                      dirEntry)
	       == sizeof(FAT12_file_datablock);
}

/*------------------------------------------------------------------------------
** FAT12_readFile()
**	IN:
**			
**	OUT: 
**			number of bytes read (by a difference of pointer values)
*/
uint32_t
FAT12_readFile
(DISK* disk, FAT12_file_descriptor* file, uint32_t byteCount, void* dataOut)
{
//#ifdef DEBUG
//	printf("%s()\r\n", __FUNCTION__);
//#endif

	uint8_t* u8dataOut = (uint8_t*)dataOut;

	/* get file data */
	FAT12_FileData* fileData = (file->handle == ROOT_DIRECTORY_HANDLE)
	                         ? &g_BS_Data->rootDirectory
	                         : &g_BS_Data->openedFiles[file->handle];

    /* don't read past the end of the file */
	if (!fileData->fd.isDirectory || (fileData->fd.isDirectory && fileData->fd.size != 0))
		byteCount = min(byteCount, fileData->fd.size - fileData->fd.position);

	while (byteCount > 0) {
		uint32_t leftInBuffer = SECTOR_SIZE - (fileData->fd.position % SECTOR_SIZE);
		uint32_t take = min(byteCount, leftInBuffer);

		memcpy(u8dataOut, fileData->buffer + fileData->fd.position % SECTOR_SIZE, take);
		u8dataOut += take;
		fileData->fd.position += take;
		byteCount -= take;
		/* see if we need to read more data */
		if (leftInBuffer == take) {
			/* special handling for root directory*/
			if (fileData->fd.handle == ROOT_DIRECTORY_HANDLE) {
				++fileData->currentCluster;
				/* read next sector */
				if (!DISK_readSectors(disk,
				                      fileData->currentCluster,
				                      1,
				                      fileData->buffer)) {
					printf("FAT: read error!\r\n");
					break;
//					goto fail;
				}
			}/* if(Handle==-1) */
			else { /* not the root directory */
				/* calculate next cluster & sector to read */
				if (++fileData->currentSectorInCluster >= g_BS_Data->BS.BS_datablock.sectorsPerCluster) {
					fileData->currentSectorInCluster = 0;
					fileData->currentCluster = FAT12_nextCluster(fileData->currentCluster);
				}
				if (fileData->currentCluster >= FINAL_CLUSTER_MARK) {
					/* mark end of file */
					fileData->fd.size = fileData->fd.position;
					break;
				}
				/* read next sector */
				if (!DISK_readSectors(disk,
				                      FAT12_clusterToLBA(fileData->currentCluster) + fileData->currentSectorInCluster,
				                      1,
				                      fileData->buffer)) {
					printf("FAT: read error!\r\n");
					break;
				}
			}/* if/else */
		}/* if() */
	}/* while() */
	return u8dataOut - (uint8_t*)dataOut;
fail:
	return 0;
}/* FAT12_readFile() */

/*----------------------------------------------------------------------------*/
FAT12_file_descriptor*
FAT12_openFATEntry
(DISK* disk, FAT12_file_datablock* entry)
{
#ifdef DEBUG
	printf("%s()\r\n", __FUNCTION__);
#endif

	/* find empty handle */
	int handle = -1;
	for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++) {
		if (!g_BS_Data->openedFiles[i].opened)
			handle = i;
    }
	if (handle < 0) {
		printf("FAT: out of file handles\r\n");
		return false;
	}
	else { 
	#ifdef DEBUG
		printf ("assigned handle %u\r\n", handle);
	#endif
	}

	/* set up vars */
	FAT12_FileData* fileData = &g_BS_Data->openedFiles[handle];
	fileData->fd.handle = handle;
	fileData->fd.isDirectory = (entry->attributes & FAT12_ATTRIBUTE_DIRECTORY) != 0;
	fileData->fd.position = 0;
	fileData->fd.size = entry->size;
	fileData->firstCluster = entry->firstClusterLow + ((uint32_t)entry->firstClusterHigh << 16);
	fileData->currentCluster = fileData->firstCluster;
	fileData->currentSectorInCluster = 0;

	if (!DISK_readSectors(disk, FAT12_clusterToLBA(fileData->currentCluster), 1, fileData->buffer)) {
		printf("FAT: read error\r\n");
		return false;
	}
	fileData->opened = true;
	return &fileData->fd;
}/* FAT12_openFATEntry() */

/*----------------------------------------------------------------------------*/
void
FAT12_fclose
(FAT12_file_descriptor* file)
{
#ifdef DEBUG
	printf("%s()\r\n", __FUNCTION__);
#endif
	if (file->handle == ROOT_DIRECTORY_HANDLE) {
		file->position = 0;
		g_BS_Data->rootDirectory.currentCluster = g_BS_Data->rootDirectory.firstCluster;
	}
	else {
		g_BS_Data->openedFiles[file->handle].opened = false;
	}
}/* FAT12_fclose */

/*----------------------------------------------------------------------------*/
uint32_t
FAT12_nextCluster
(uint32_t currentCluster)
{
#ifdef DEBUG
	printf("%s()\r\n", __FUNCTION__);
#endif
	uint32_t FATIndex = currentCluster * 3 / 2;
	if (currentCluster % 2 == 0)
		return (*(uint16_t*)(g_FAT + FATIndex)) & 0x0FFF;
	else
		return (*(uint16_t*)(g_FAT + FATIndex)) >> 4;
}

/*----------------------------------------------------------------------------*/
uint32_t
FAT12_clusterToLBA
(uint32_t cluster)
{
#ifdef DEBUG
	printf("%s()\r\n", __FUNCTION__);
#endif

	uint32_t lba;

	printf("    disk data area starts at cluster %u\r\n", g_DataSectionLBA);
	printf("    calculating LBA for cluster %u\r\n", cluster);
	lba = g_DataSectionLBA
	    + (g_BS_Data->BS.BS_datablock.sectorsPerCluster * (cluster - 2));

	printf("    LBA = %u\r\n", lba);
	return lba;
}
