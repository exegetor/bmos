#include "stdio.h"
#include "stdint.h"
#include "memdefs.h"
#include "utility.h"
#include "memory.h"
#include "ctype.h"
#include "string.h"
#include "fat.h"

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 10
#define ROOT_DIRECTORY_HANDLE -1
#define FINAL_CLUSTER_MARK 0xFF8

#pragma pack(push,1)
typedef struct
{
    uint8_t BootJumpInstruction[3];
    uint8_t OEMIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FATCount;
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
} FAT_BootSector;
#pragma pack(pop)

typedef struct
{
    FAT_File Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;
    uint8_t Buffer[SECTOR_SIZE];
} FAT_FileData;

typedef struct
{
    union {
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;
    FAT_FileData RootDirectory;
    FAT_FileData OpenedFiles[MAX_FILE_HANDLES];
} FAT_Data;

/* this won't work when we have huge FATs, so
** we will need to implement caching later */
static FAT_Data far* g_Data;
static uint8_t far* g_Fat = NULL;
static uint32_t g_DataSectionLBA;


/*----------------------------------------------------------------------------*/
bool FAT_ReadBootSector(DISK* disk)
{
    return DISK_ReadSectors(disk,
                            0,
                            1,
                            g_Data->BS.BootSectorBytes);
}


/*----------------------------------------------------------------------------*/
bool FAT_ReadFAT(DISK* disk)
{
    return DISK_ReadSectors(disk,
                            g_Data->BS.BootSector.ReservedSectors,
                            g_Data->BS.BootSector.SectorsPerFAT,
                            g_Fat);
}


/*----------------------------------------------------------------------------*/
/*
bool FAT_ReadRootDirectory(DISK* disk)
{
    uint32_t lba = g_Data->BS.BootSector.ReservedSectors + g_Data->BS.BootSector.SectorsPerFAT * g_Data->BS.BootSector.FATCount;
    uint32_t size = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
    uint32_t sectors = (size + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;

    g_RootDirectoryEnd = lba + sectors;
    return DISK_ReadSectors(disk, lba, sectors, g_RootDirectory);

}
*/


/*----------------------------------------------------------------------------*/
bool FAT_Initialize(DISK* disk)
{
    g_Data = (FAT_Data far*)MEMORY_FAT_ADDR;

    /* read boot sector */
    if (!FAT_ReadBootSector(disk)) {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }

    /* read FAT */
    g_Fat = (uint8_t far*)g_Data + sizeof(FAT_Data);
    uint32_t fatSize = g_Data->BS.BootSector.BytesPerSector * g_Data->BS.BootSector.SectorsPerFAT;
    if (sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE) {
        printf("FAT: not enough memory to read FAT! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize, MEMORY_FAT_SIZE);
        return false;
    }

    if (!FAT_ReadFAT(disk)) {
        printf("FAT: read FAT failed\r\n");
        return false;
    }

    /* read root directory */
    uint32_t rootDirLBA = g_Data->BS.BootSector.ReservedSectors + g_Data->BS.BootSector.SectorsPerFAT * g_Data->BS.BootSector.FATCount;
    uint32_t rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
    rootDirSize = align(rootDirSize, g_Data->BS.BootSector.BytesPerSector);
    
/*
    if (sizeof(FAT_Data) + fatSize + rootDirSize >= MEMORY_FAT_SIZE) {
        printf("FAT: not enough memory to read root directory! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize + rootDirSize, MEMORY_FAT_SIZE);
        return false;
    }
    if (!FAT_ReadRootDirectory(disk)) {
        printf("FAT: read root directory failed\r\n");
        return false;
    }
*/
    /* open root directory file */
    g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory.Public.IsDirectory = true;
    g_Data->RootDirectory.Public.Position = 0;
    g_Data->RootDirectory.Public.Size = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
    g_Data->RootDirectory.Opened = true;
    g_Data->RootDirectory.FirstCluster = rootDirLBA;
    g_Data->RootDirectory.CurrentCluster = rootDirLBA;
    g_Data->RootDirectory.CurrentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, rootDirLBA, 1, g_Data->RootDirectory.Buffer)) {
        printf("FAT: read root directory failed\r\n");
        return false;
    }

    /* calculate data section */
    uint32_t rootDirSectors = (rootDirSize + g_Data->BS.BootSector.BytesPerSector -1) / g_Data->BS.BootSector.BytesPerSector;
    g_DataSectionLBA = rootDirLBA + rootDirSectors;
    
    /* reset opened files */
    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        g_Data->OpenedFiles[i].Opened = false;
    return true;
}/* FAT_Initialize() */


/*----------------------------------------------------------------------------*/
/*
bool FAT_ReadSectors(DISK* disk, uint32_t lba, uint32_t count, void* bufferOut)
{
    bool ok = true;

    ok = ok && (fseek(disk, lba* g_Data->BS.BootSector.BytesPerSector, SEEK_SET) == 0);
    if (!ok)
        printf("fseek failed (lba=%d)(count=%d)\n", lba, count);

    ok = ok && (fread(bufferOut, g_Data->BS.BootSector.BytesPerSector, count, disk) == count);
    if (!ok)
        printf("fread failed\n");
    return ok;
}
*/

/*----------------------------------------------------------------------------*/
uint32_t FAT_ClusterToLBA(uint32_t cluster)
{
    return g_DataSectionLBA + (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

/*----------------------------------------------------------------------------*/
FAT_File far* FAT_OpenEntry(DISK* disk, FAT_DirectoryEntry* entry)
{
    /* find enpty handle */
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++) {
        if (!g_Data->OpenedFiles[i].Opened)
            handle = i;
    }
    if (handle < 0) {
        printf("FAT: out of file handles\r\n");
        return false;
    }

    /* set up vars */
    FAT_FileData far* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = 0;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, FAT_ClusterToLBA(fd->CurrentCluster), 1, fd->Buffer)) {
        printf("FAT: read error\r\n");
        return false;
    }
    fd->Opened = true;
    return &fd->Public;
}/* FAT_OpenEntry() */


/*----------------------------------------------------------------------------*/
uint32_t FAT_NextCluster(uint32_t currentCluster)
{
    uint32_t fatIndex = currentCluster * 3 / 2;
    if (currentCluster % 2 == 0)
        return (*(uint16_t far*)(g_Fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t far*)(g_Fat + fatIndex)) >> 4;
}


/*----------------------------------------------------------------------------*/
uint32_t FAT_Read(DISK* disk, FAT_File far* file, uint32_t byteCount, void* dataOut)
{
    /* get file data */
    FAT_FileData far* fd = (file->Handle == ROOT_DIRECTORY_HANDLE)
        ? &g_Data->RootDirectory
        : &g_Data->OpenedFiles[file->Handle];

    uint8_t* u8dataOut = (uint8_t*)dataOut;

    /* don't read past the end of the file */
    if (!fd->Public.IsDirectory)
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);
    while (byteCount > 0) {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8dataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8dataOut += take;
        fd->Public.Position += take;
        byteCount -= take;
        /* see if we need to read more data */
        if (leftInBuffer == take) {
            /* special handling for ROOT DIRECTORY*/
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE) {
                ++fd->CurrentCluster;
                /* read next sector */
                if (!DISK_ReadSectors(disk,
                                      fd->CurrentCluster,
                                      1,
                                      fd->Buffer)) {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
            else {
                /* calculate next cluster & sector to read */
                if (++fd->CurrentSectorInCluster >= g_Data->BS.BootSector.SectorsPerCluster) {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(fd->CurrentCluster);
                }
                if (fd->CurrentCluster >= FINAL_CLUSTER_MARK) {
                //  printf("FAT: read error! invalid next cluster\r\n");
                    // mark end of file
                    fd->Public.Size = fd->Public.Position;
                    break;
                }
                /* read next sector */
                if (!DISK_ReadSectors(disk,
                                    FAT_ClusterToLBA(fd->CurrentCluster) + fd->CurrentSectorInCluster,
                                    1,
                                    fd->Buffer)) {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
        }
    }
    return u8dataOut - (uint8_t*)dataOut;
}/* FAT_Read() */


/*----------------------------------------------------------------------------*/
bool FAT_ReadEntry(DISK* disk, FAT_File far* file, FAT_DirectoryEntry* dirEntry)
{
    return FAT_Read(disk,
                    file,
                    sizeof(FAT_DirectoryEntry),
                    dirEntry)
           == sizeof(FAT_DirectoryEntry);
}


/*----------------------------------------------------------------------------*/
void FAT_Close(FAT_File far* file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE) {
        file->Position = 0;
        g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
    }
    else {
        g_Data->OpenedFiles[file->Handle].Opened = false;
    }
}


/*----------------------------------------------------------------------------*/
bool FAT_FindFile(DISK* disk, FAT_File far* file, const char* name, FAT_DirectoryEntry* entryOut)
{
    char fatName[12];
    FAT_DirectoryEntry entry;

    /*convert name to fatName */
    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';
    /* TODO: fix this.  won't handle more than one dot in filename */
    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = toupper(name[i]);

    if (ext != NULL) {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);
    }

    while (FAT_ReadEntry(disk, file, &entry))
    {
        if (memcmp(fatName, entry.Name, 11) == 0) {
            *entryOut = entry;
            return true;
        }
    }
    return false;
}/* FAT_FindFile() */


/*----------------------------------------------------------------------------*/
FAT_File far* FAT_Open(DISK* disk, const char* path)
{
    char name[MAX_PATH_SIZE];

    /* ignore leading slash */
    if (path[0] == '/')
        path++;

    FAT_File far* current = &g_Data->RootDirectory.Public;

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
        FAT_DirectoryEntry entry;
        if (FAT_FindFile(disk, current, name, &entry)) {
            FAT_Close(current);
            /* check is directory */
            if (!isLast && entry.Attributes & FAT_ATTRIBUTE_DIRECTORY == 0) {
                printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }
            /* open new directory entry */
            current = FAT_OpenEntry(disk, &entry);
        }
        else {
            FAT_Close(current);
            printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }
    return current;
}/* FAT_Open() */
