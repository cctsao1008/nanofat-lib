#ifndef _FAT_LIBRARY_
#define _FAT_LIBRARY_

#include "Compiler.h"
#include "GenericTypeDefs.h"

// Constants used when determining Directory Entry types
#define EOD 0
#define UNUSED 1
#define LFNAME 2
#define NFNAME 3
#define NDNAME 4     //Normal directory

// Global constants used by the FAT32 file system
#define BUFFER_SIZE 512
#define DIRENTRY_SIZE 32
#define FILENAME_SIZE 12
#define FILEDATA_SIZE 32
#define PARTITION_ENTRY_SIZE  16

#define FIRST_PARTITION_ENTRY 0x1BE

//#define ReadSectorRelativeToPartition(x) ReadSector(x)

// File descriptor structure
typedef struct
{
   BYTE Name[FILENAME_SIZE];
   BYTE Attributes;
   DWORD FirstLBA;
   DWORD Size;
   DWORD FirstCluster;
} FILE_DESCRIPTOR;

// Partition entry structure
typedef struct
{   
   BOOL IsValid;
   BOOL IsFAT32;
   BYTE Type;
} PARTITION_ENTRY;

// File data structure
typedef struct
{
   BYTE Buffer[FILEDATA_SIZE];
   WORD BytesRead;
   BOOL IsEOF;
} FILE_DATA;

// Buffers used to temporarily save the obtained data
extern BYTE SectorBuffer[BUFFER_SIZE];
extern BYTE DirEntry[DIRENTRY_SIZE];
extern PARTITION_ENTRY PartitionEntry;

// Volume ID information
extern WORD BytesPerSector;
extern BYTE SectorsPerCluster;
extern WORD NumberOfReservedSectors;
extern BYTE NumberOfFATs;
extern WORD SectorsPerFAT;
extern WORD RootDirectoryFirstCluster;

// Information related to partitions and FAT table
extern WORD PartitionLBABegin;
extern WORD FatBeginLBA;
//extern WORD ClusterBeginLBA;
extern WORD DataBeginLBA;
extern BYTE FAT16Offset;

// Variables used to store the position of current entry
// when reading directory contents.
extern DWORD DirectoryCurrentCluster;
extern DWORD DirectoryCurrentLBA;
extern BYTE DirectoryCurrentDirEntry;
extern WORD DirectoryCurrentSector;
extern BOOL DirectoryOpened;
extern BOOL EODReached;

// Variables used to store file information when reading
extern DWORD FileCurrentCluster;
extern DWORD FileCurrentLBA;
extern WORD FileCurrentSector;
extern DWORD FileLengthCount;
extern WORD FileSectorPosition;
extern BOOL FileOpened;

DWORD Make32(BYTE lsb, BYTE mb1, BYTE mb2, BYTE msb);
WORD Make16(BYTE lsb, BYTE msb);
BOOL BitTest(DWORD dwValue, BYTE bPosition);
void ReadSector(DWORD dwLBA);
void ReadDirEntry(BYTE bNum);
BOOL GetEntry(WORD wStartingPos, PARTITION_ENTRY *pPartitionEntry);
BOOL FindFirstFATPartition(PARTITION_ENTRY *pPartitionEntry);
BOOL ReadVolumeID(PARTITION_ENTRY *pPartitionEntry);
DWORD ComputeLBA(DWORD dwClusterNumber);
BOOL CheckIfEOD();
BYTE GetAttib();
BOOL CheckIfNormalFile();
BYTE DetermineDirEntryType();
DWORD GetFileSize();
void GetFileName(BYTE *bFileName);
void GetDirName(BYTE *bFileName);
DWORD GetFileFirstCluster();
DWORD ReadFAT(DWORD dwClusterNumber);
void OpenDirectory(DWORD dwDirectoryCluster);
FILE_DESCRIPTOR FindNextFile();
void CloseDirectory();
void OpenFile(FILE_DESCRIPTOR fdFile);
FILE_DATA ReadFile(FILE_DESCRIPTOR fdFile, DWORD dwCount);
void CloseFile();

#endif