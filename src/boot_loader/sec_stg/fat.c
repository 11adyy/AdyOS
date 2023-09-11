#include "fat.h"
#include "stdio.h"
#include "memdefs.h"
#include "string.h"
#include "memory.h"
#include "ctype.h"
#include "math.h"

#include <stddef.h>
#include <stdbool.h>

#define SECTOR_SIZE             512
#define MAX_PATH_SIZE           256
#define MAX_FILE_HANDLES        10
#define ROOT_DIRECTORY_HANDLE   -1

#pragma pack(push, 1)

typedef struct {
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    // extended boot record
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;          // serial number, value doesn't matter
    uint8_t VolumeLabel[11];    // 11 bytes, padded with spaces
    uint8_t SystemId[8];

    // ... we don't care about code ...

} __attribute__((packed))FAT_BootSector;

#pragma pack(pop)


typedef struct {
    uint8_t Buffer[SECTOR_SIZE];
    FAT_file Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;

} FAT_FileData;

typedef struct {
    union {
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BootSector;

    FAT_FileData RootDirectory;

    FAT_FileData OpenedFiles[MAX_FILE_HANDLES];

} FAT_Data;

static FAT_Data* _data;
static uint8_t* _fat = NULL;
static uint32_t _dataSectionLba;
static uint8_t _fatType;

bool FAT_readBootSector(Partition* disk) {
    return Partition_readSectors(disk, 0, 1, _data->BootSector.BootSectorBytes);
}

bool FAT_readFat(Partition* disk) {
    return Partition_readSectors(disk, _data->BootSector.BootSector.ReservedSectors, _data->BootSector.BootSector.SectorsPerFat, _fat);
}

////////////////////////////////////////////////////////////////////////////// (..) //////////////
//  *BOOT SECTOR*   //      //         //           //                      // (..) //
// FST_STG  SEC_STG //      // *FAT 1* //   *FAT 2* //  *ROOT DIRECTORY*    // (..) //  *DATA*
//  *BOOT SECTOR*   //      //         //           //                      // (..) //
////////////////////////////////////////////////////////////////////////////// (..) //////////////

void FAT_detect() {
    
}

bool FAT_initialize(Partition* disk) {
    _data = (FAT_Data*)MEMORY_FAT_ADDR;

    // read boot sector
    if (!FAT_readBootSector(disk)) {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }

    // read FAT
    _fat = (uint8_t*)_data + sizeof(FAT_Data);
    uint32_t fatSize = _data->BootSector.BootSector.BytesPerSector * _data->BootSector.BootSector.SectorsPerFat;
    if (sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE) {
        printf("FAT: not enough memory to read FAT! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize, MEMORY_FAT_SIZE);
        return false;
    }

    if (!FAT_readFat(disk)) {
        printf("FAT: read FAT failed\r\n");
        return false;
    }

    // open root directory file
    uint32_t rootDirLba = _data->BootSector.BootSector.ReservedSectors + _data->BootSector.BootSector.SectorsPerFat * _data->BootSector.BootSector.FatCount;
    uint32_t rootDirSize = sizeof(FAT_directoryEntry) * _data->BootSector.BootSector.DirEntryCount;

    _data->RootDirectory.Public.Handle          = ROOT_DIRECTORY_HANDLE;
    _data->RootDirectory.Public.IsDirectory     = true;
    _data->RootDirectory.Public.Position        = 0;
    _data->RootDirectory.Public.Size            = sizeof(FAT_directoryEntry) * _data->BootSector.BootSector.DirEntryCount;
    _data->RootDirectory.Opened                 = true;
    _data->RootDirectory.FirstCluster           = rootDirLba;
    _data->RootDirectory.CurrentCluster         = rootDirLba;
    _data->RootDirectory.CurrentSectorInCluster = 0;

    if (!Partition_readSectors(disk, rootDirLba, 1, _data->RootDirectory.Buffer)) {
        printf("FAT: read root directory failed\r\n");
        return false;
    }

    // calculate data section
    uint32_t rootDirSectors = (rootDirSize + _data->BootSector.BootSector.BytesPerSector - 1) / _data->BootSector.BootSector.BytesPerSector;
    _dataSectionLba = rootDirLba + rootDirSectors;

    // reset opened files
    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        _data->OpenedFiles[i].Opened = false;

    return true;
}

uint32_t FAT_clusterToLba(uint32_t cluster) {
    return _dataSectionLba + (cluster - 2) * _data->BootSector.BootSector.SectorsPerCluster;
}

FAT_file* FAT_openEntry(Partition* disk, FAT_directoryEntry* entry) {
    // find empty handle
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++) 
        if (!_data->OpenedFiles[i].Opened)
            handle = i;
    
    // out of handles
    if (handle < 0) {
        printf("FAT: out of file handles\r\n");
        return false;
    }

    // setup vars
    FAT_FileData* fileData           = &_data->OpenedFiles[handle];

    fileData->Public.Handle          = handle;
    fileData->Public.IsDirectory     = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fileData->Public.Position        = 0;
    fileData->Public.Size            = entry->Size;
    fileData->FirstCluster           = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fileData->CurrentCluster         = fileData->FirstCluster;
    fileData->CurrentSectorInCluster = 0;

    if (!Partition_readSectors(disk, FAT_clusterToLba(fileData->CurrentCluster), 1, fileData->Buffer)) {
        printf("FAT: read error\r\n");
        return false;
    }

    fileData->Opened = true;
    return &fileData->Public;
}

uint32_t FAT_nextCluster(uint32_t currentCluster) {    
    uint32_t fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16_t*)(_fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t*)(_fat + fatIndex)) >> 4;
}

uint32_t FAT_read(Partition* disk, FAT_file* file, uint32_t byteCount, void* dataOut) {
    // get file data
    FAT_FileData* fileData = (file->Handle == ROOT_DIRECTORY_HANDLE) 
        ? &_data->RootDirectory 
        : &_data->OpenedFiles[file->Handle];

    uint8_t* u8DataOut = (uint8_t*)dataOut;

    // don't read past the end of the file
    if (!fileData->Public.IsDirectory || (fileData->Public.IsDirectory && fileData->Public.Size != 0)) 
        byteCount = min(byteCount, fileData->Public.Size - fileData->Public.Position);

    while (byteCount > 0) {
        uint32_t leftInBuffer = SECTOR_SIZE - (fileData->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fileData->Buffer + fileData->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fileData->Public.Position += take;
        byteCount -= take;

        // printf("leftInBuffer=%lu take=%lu\r\n", leftInBuffer, take);
        // See if we need to read more data
        if (leftInBuffer == take) {
            // Special handling for root directory
            if (fileData->Public.Handle == ROOT_DIRECTORY_HANDLE) {
                ++fileData->CurrentCluster;

                // read next sector
                if (!Partition_readSectors(disk, fileData->CurrentCluster, 1, fileData->Buffer)) {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
            else {
                // calculate next cluster & sector to read
                if (++fileData->CurrentSectorInCluster >= _data->BootSector.BootSector.SectorsPerCluster) {
                    fileData->CurrentSectorInCluster = 0;
                    fileData->CurrentCluster = FAT_nextCluster(fileData->CurrentCluster);
                }

                if (fileData->CurrentCluster >= 0xFF8) {
                    // Mark end of file
                    fileData->Public.Size = fileData->Public.Position;
                    break;
                }

                // read next sector
                if (!Partition_readSectors(disk, FAT_clusterToLba(fileData->CurrentCluster) + fileData->CurrentSectorInCluster, 1, fileData->Buffer)) {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
        }
    }

    return u8DataOut - (uint8_t*)dataOut;
}

bool FAT_readEntry(Partition* disk, FAT_file* file, FAT_directoryEntry* dirEntry) {
    return FAT_read(disk, file, sizeof(FAT_directoryEntry), dirEntry) == sizeof(FAT_directoryEntry);
}

void FAT_close(FAT_file* file) {
    printf("FAT is closing...\r\n");

    if (file->Handle == ROOT_DIRECTORY_HANDLE) {
        file->Position = 0;
        _data->RootDirectory.CurrentCluster = _data->RootDirectory.FirstCluster;
    }
    else {
        _data->OpenedFiles[file->Handle].Opened = false;
    }

    printf("FAT closed\r\n");
}

bool FAT_findFile(Partition* disk, FAT_file* file, const char* name, FAT_directoryEntry* entryOut) {
    char fatName[12];
    FAT_directoryEntry entry;

    // convert from name to fat name
    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = toupper(name[i]);

    if (ext != name + 11) 
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);

    while (FAT_readEntry(disk, file, &entry)) {
        if (memcmp(fatName, entry.Name, 11) == 0) {
            *entryOut = entry;
            return true;
        }
    }
    
    return false;
}

FAT_file* FAT_open(Partition* disk, const char* path) {
    printf("Starting opening FAT: ");
    printf(path);
    printf("\r\n");

    char name[MAX_PATH_SIZE];

    // ignore leading slash
    if (path[0] == '/')
        path++;

    FAT_file* current = &_data->RootDirectory.Public;

    while (*path) {
        // extract next file name from path
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
        
        // find directory entry in current directory
        FAT_directoryEntry entry;
        if (FAT_findFile(disk, current, name, &entry)) {
            FAT_close(current);

            // check if directory
            if (!isLast && entry.Attributes & FAT_ATTRIBUTE_DIRECTORY == 0) {
                printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }

            // open new directory entry
            current = FAT_openEntry(disk, &entry);
        }
        else {
            FAT_close(current);

            printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }

    printf("\r\n");
    printf("Opening ended succes!\r\n");
    return current;
}
