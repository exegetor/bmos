#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef uint8_t bool;
#define true 1
#define false 0

typedef struct
{
    uint8_t BootJumpInstruction[3];
    uint8_t OEMIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFAT;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;
    // extended boot record
    uint8_t DriveNumber;
    uint8_t _reserved;
    uint8_t Signature;
    uint32_t VolumeID;
    uint8_t VolumeLabel[8];
    uint8_t SystemID[8];
} __attribute__((packed)) BootSector;
BootSector g_BootSector;

uint8_t* g_Fat = NULL;

typedef struct
{
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed)) DirectoryEntry;
DirectoryEntry* g_RootDirectory = NULL;

uint32_t g_RootDirectoryEnd;


//------------------------------------------------------------------------------
bool readBootSector(FILE* disk)
{
    return fread(&g_BootSector, sizeof(g_BootSector), 1, disk) > 0;
}


//------------------------------------------------------------------------------
void print_boot_data()
{
//    printf("bytes per sector:%d\n", g_bootsector.BytesPerSector);
//    printf("reserved sectors:%d\n", g_bootsector.ReservedSectors);
//    printf("total sectors:%d\n", g_bootsector.TotalSectors);
//    printf("FAT count:%d\n", g_bootsector.FatCount);
//    printf("sectors per FAT:%d\n", g_bootsector.SectorsPerFAT);
//    printf("DirEntryCount:%d\n", g_bootsector.DirEntryCount);

//    uint8_t BootJumpInstruction[3];
//    uint8_t OEMIdentifier[8];
//    uint8_t SectorsPerCluster;
//    uint8_t MediaDescriptorType;
//    uint16_t SectorsPerTrack;
//    uint16_t Heads;
//    uint32_t HiddenSectors;
//    uint32_t LargeSectorCount;
//    // extended boot record
//    uint8_t DriveNumber;
//    uint8_t _reserved;
//    uint8_t Signature;
//    uint32_t VolumeID;
//    uint8_t VolumeLabel[8];
//    uint8_t SystemID[8];
}

//------------------------------------------------------------------------------
bool readSectors(FILE* disk, uint32_t lba, uint32_t count,  void* bufferOut)
{
    bool ok = true;

    ok = ok && (fseek(disk, lba* g_BootSector.BytesPerSector, SEEK_SET) == 0);
    if (!ok)
        fprintf(stderr, "fseek failed (lba=%d)(count=%d)\n", lba, count);

    ok = ok && (fread(bufferOut, g_BootSector.BytesPerSector, count, disk) == count);
    if (!ok)
        fprintf(stderr, "fread failed\n");
    return ok;
}

//------------------------------------------------------------------------------
bool readFAT(FILE* disk)
{
    g_Fat = (uint8_t*)malloc(g_BootSector.SectorsPerFAT * g_BootSector.BytesPerSector);
//    if (!g_Fat)
//        return false;
    return readSectors(disk, g_BootSector.ReservedSectors, g_BootSector.SectorsPerFAT, g_Fat);
}


//------------------------------------------------------------------------------
bool readRootDirectory(FILE* disk)
{
    uint32_t lba = g_BootSector.ReservedSectors + g_BootSector.FatCount * g_BootSector.SectorsPerFAT;
    uint32_t size = sizeof(DirectoryEntry) * g_BootSector.DirEntryCount;
    uint32_t sectors = (size / g_BootSector.BytesPerSector);
    if (size % g_BootSector.BytesPerSector > 0) {
        sectors++;
    }

    g_RootDirectoryEnd = lba + sectors;
    g_RootDirectory = (DirectoryEntry*)malloc(sectors * g_BootSector.BytesPerSector);
    if (!g_RootDirectory)
        return false;
    return readSectors(disk, lba, sectors, g_RootDirectory);
}


//------------------------------------------------------------------------------
DirectoryEntry* findFile(const char* name)
{
    for (uint32_t i = 0; i < g_BootSector.DirEntryCount; i++) {
        if (memcmp(name, g_RootDirectory[i].Name, 11) == 0)
            return &g_RootDirectory[i];
    }
    return NULL;
}


//------------------------------------------------------------------------------
bool readFile(DirectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer)
{
    bool ok = true;
    uint16_t currentCluster = fileEntry->FirstClusterLow;
    
    do {
                //                       first 2 clusters are reserved
        uint32_t lba = g_RootDirectoryEnd + (currentCluster - 2) * g_BootSector.SectorsPerCluster;
        ok = ok && readSectors(disk, lba, g_BootSector.SectorsPerCluster, outputBuffer);
        outputBuffer += g_BootSector.SectorsPerCluster * g_BootSector.BytesPerSector;

        // Find the next cluster; this is a simple lookup in the File Allocation Table...
        // ...but slightly more complex since each entry is 12 bits wide
        uint32_t fatIndex = currentCluster * 3 / 2;
        if (currentCluster % 2 == 0)
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF; // strip off the top 4 bits
        else
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) >> 4; // shift down by 4 bits
    } while (ok && currentCluster < 0x0FF8);

    return ok;
}
//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    bool ok = true;
    bool ret = 0;

    if (argc < 3) {
        printf("syntax: %s <disk image>\n", argv[0]);
        return -1;
    }
    FILE* disk = fopen(argv[1], "rb");
    if (!disk) {
        fprintf(stderr, "Cannot open disk image %s\n", argv[1]);
        return -1;        
    }
    if (!readBootSector(disk)) {
        fprintf(stderr, "Cannot read boot sector!\n");
        return -2;
    }
    print_boot_data();

    if (!readFAT(disk)) {
        fprintf(stderr, "Cannot read FAT!\n");
        free(g_Fat);
        return -3;
    }
    if (!readRootDirectory(disk)) {
        fprintf(stderr, "Cannot read root directory!\n");
        free(g_Fat);
        free(g_RootDirectory);
        return -4;
    }

    DirectoryEntry* fileEntry = findFile(argv[2]);
    if (!fileEntry) {
        fprintf(stderr, "Cannot locate file %s\n", argv[2]);
        free(g_Fat);
        free(g_RootDirectory);
        return -5;
    }

    uint8_t* buffer = (uint8_t*)malloc(fileEntry->Size + g_BootSector.BytesPerSector);
    if (!buffer)
        goto fail;
    if (!readFile(fileEntry, disk, buffer)) {
        fprintf(stderr, "Cannot read file %s\n", argv[2]);
        free(g_Fat);
        free(g_RootDirectory);
        free(buffer);
        return -6;
    }

    for (size_t i = 0; i < fileEntry->Size; i++) {
        if (isprint(buffer[i])) fputc(buffer[i], stdout);
        else printf("<%02x>", buffer[i]); 
    }
    printf("\n");
 
fail:
    free(g_Fat);
    free(g_RootDirectory);
    free(buffer);
    return 0;
}
