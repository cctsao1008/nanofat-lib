#include "FATLib.h"
#include "SDDriver.h"

#pragma udata FATLIB=0x500

// Buffers used to temporarily save the obtained data
BYTE SectorBuffer[BUFFER_SIZE];
BYTE DirEntry[DIRENTRY_SIZE];

// Volume ID information
WORD BytesPerSector;
BYTE SectorsPerCluster;
WORD NumberOfReservedSectors;
BYTE NumberOfFATs;
WORD SectorsPerFAT;
WORD RootDirectoryFirstCluster;

// Information related to partitions and FAT table
WORD PartitionLBABegin;
WORD FatBeginLBA;
//WORD ClusterBeginLBA;
WORD DataBeginLBA;
BYTE FAT16Offset;

// Variables used to store the position of current entry
// when reading directory contents.
DWORD DirectoryCurrentCluster;
DWORD DirectoryCurrentLBA;
BYTE DirectoryCurrentDirEntry;
WORD DirectoryCurrentSector;
BOOL DirectoryOpened;
BOOL EODReached;

// Variables used to store file information when reading
DWORD FileCurrentCluster;
DWORD FileCurrentLBA;
WORD FileCurrentSector;
DWORD FileLengthCount;
WORD FileSectorPosition;
BOOL FileOpened;

#pragma code

DWORD Make32(BYTE lsb, BYTE mb1, BYTE mb2, BYTE msb)
{
   	// Makes a 32-bit unsigned integer from 4 bytes
	DWORD dwResult;
   
   	dwResult = (DWORD)msb;
   	dwResult <<= 8;
   	dwResult |= (DWORD)mb2;
   	dwResult <<= 8;
   	dwResult |= (DWORD)mb1;
   	dwResult <<= 8;
   	dwResult |= (DWORD)lsb;

   return dwResult;
}

WORD Make16(BYTE lsb, BYTE msb)
{
   	// Makes a 16-bit unsigned integer from 2 bytes
	WORD wResult;
   
   	wResult = (WORD)msb;
   	wResult <<= 8;
   	wResult |= (WORD)lsb;

   	return wResult;
}

BOOL BitTest(DWORD dwValue, BYTE bPosition)
{
	DWORD dwResult;

	dwResult = dwValue & (1 << bPosition);

	if (dwResult == 1)
		return TRUE;
	else
		return FALSE;
}

void ReadSector(DWORD dwLBA)
{
	SDReadBlock(dwLBA, &SectorBuffer[0]);
}

void ReadDirEntry(BYTE bNum)
{
	// Calculate Directory Entry offset according to Directory Entry number
	WORD wOffset;
	WORD i;
   
   	wOffset = (WORD)bNum * DIRENTRY_SIZE;
   
	// Copy the Directory Entry data to the DirEntry vector in order to
   	// analyze the data later
   	for (i = 0; i < DIRENTRY_SIZE; i++)
   	{
		DirEntry[i] = SectorBuffer[wOffset + i];
   	}
}

BOOL GetEntry(WORD wStartingPos, PARTITION_ENTRY *pPartitionEntry)
{
   	pPartitionEntry->IsValid = FALSE;
 
   	// Partition must exists and be ACTIVE
	if (SectorBuffer[wStartingPos] != 0x80) return FALSE;

   	pPartitionEntry->Type = SectorBuffer[wStartingPos + 4];

   	if ((pPartitionEntry->Type == 0x04) || (pPartitionEntry->Type == 0x06) || (pPartitionEntry->Type == 0x0E))
	{
		pPartitionEntry->IsFAT32 = FALSE;
		pPartitionEntry->IsValid = TRUE;
	}

   	if ((pPartitionEntry->Type == 0x0B) || (pPartitionEntry->Type == 0x0C))
	{
		pPartitionEntry->IsFAT32 = TRUE;
		pPartitionEntry->IsValid = TRUE;
	}
   	
	if (!(pPartitionEntry->IsValid)) return FALSE;

   	PartitionLBABegin = Make32(SectorBuffer[wStartingPos + 11], SectorBuffer[wStartingPos + 10], SectorBuffer[wStartingPos + 9], SectorBuffer[wStartingPos + 8]);
   	
	return TRUE;
}

BOOL FindFirstFATPartition(PARTITION_ENTRY *pPartitionEntry)
{
   	WORD i;

   	// Read the first Sector
   	ReadSector(0);

	// Return if signature is not correct
   	if ((SectorBuffer[0x1FE] != 0x55) || (SectorBuffer[0x1FF] != 0xAA)) return FALSE;
   
   	pPartitionEntry->IsValid = FALSE;

   	for (i = 0; i < 4; i++)
   	{
      	if (GetEntry(FIRST_PARTITION_ENTRY + (i * PARTITION_ENTRY_SIZE), pPartitionEntry)) break;
   	}
   
	// Worst case: no partitions entry are found. Assume that MBR is at Sector 0 and its a FAT32 image.
   	if (!pPartitionEntry->IsValid)
   	{
      	// Look for the offset
      	PartitionLBABegin = 0;
      	pPartitionEntry->IsFAT32 = TRUE;
      	pPartitionEntry->IsValid = TRUE;
   	}

   	return TRUE;
}

BOOL ReadVolumeID(PARTITION_ENTRY *pPartitionEntry)
{        
   	// Read the first sector of the partition
   	ReadSector(PartitionLBABegin);

   	// Initial values
   	FAT16Offset = 0;

   	// If the signature is 0xAA55 we are in presence of a FAT volume
	// Return if signature is not correct signature
	printf("%02X %02X\r\n", SectorBuffer[0x1FE], SectorBuffer[0x1FF]);
   	if ((SectorBuffer[0x1FE] != 0x55) || (SectorBuffer[0x1FF] != 0xAA)) return FALSE;
       
   	// Bytes per Sector
   	BytesPerSector = Make16(SectorBuffer[0x0B + 1], SectorBuffer[0x0B]);

   	// Sectors per Cluster
   	SectorsPerCluster = SectorBuffer[0x0D];

   	// Number of Reserved Sectors
   	NumberOfReservedSectors = Make16(SectorBuffer[0x0E + 1], SectorBuffer[0x0E]);

   	// Number of FATs
   	NumberOfFATs = SectorBuffer[0x10];

   	// Sectors per FAT (this value is different in FAT16/32 file systems)
   	if (pPartitionEntry->IsFAT32)
	{
		SectorsPerFAT = Make32(SectorBuffer[0x24+3], SectorBuffer[0x24 + 2], SectorBuffer[0x24 + 1], SectorBuffer[0x24]);
   	}else{
      	SectorsPerFAT = Make16(SectorBuffer[0x16 + 1], SectorBuffer[0x16]);
      	// FAT16 has an offset needed to calculate real ClusterBeginLBA
		FAT16Offset = (DWORD)Make16(SectorBuffer[0x11 + 1], SectorBuffer[0x11]) * 32 / (DWORD)BytesPerSector;
   	}

   	// Important zones CALCULUS
   	FatBeginLBA = PartitionLBABegin + (DWORD)NumberOfReservedSectors;

   	DataBeginLBA = FatBeginLBA + ((DWORD)NumberOfFATs * SectorsPerFAT);

   	// Root Directory First Cluster ?????????
   	if (pPartitionEntry->IsFAT32){
		RootDirectoryFirstCluster = Make32(SectorBuffer[0x2C + 3], SectorBuffer[0x2C + 2], SectorBuffer[0x2C + 1], SectorBuffer[0x2C]);
   	} else {
      	//DataBeginLBA += 32;
      	RootDirectoryFirstCluster = 2;
   	}
   
   	return TRUE;
}

DWORD ComputeLBA(DWORD dwClusterNumber)
{
   	// Compute the LBA number using the Cluster Number
   	return DataBeginLBA + ((dwClusterNumber - 2) * ((DWORD)SectorsPerCluster));
}

BOOL CheckIfEOD()
{
   	// Check if the current directory entry (contained in the DirEntry vector)
   	// correspond to the flag End Of Directory.
   	if (DirEntry[0] == 0x00)
   	{
       	return TRUE;
   	}
   	else
   	{
       	return FALSE;
   	}
}

BYTE GetAttib()
{
   	// Get the Attributes byte from current Directory Entry
   	return DirEntry[0x0B];
}

BOOL CheckIfNormalFile()
{
   	// Check if the Directory Entry corresponds to a Normal File Name
   	BYTE bAttr;
   
   	bAttr = GetAttib();

   	bAttr <<= 4;
   	if (bAttr == 0xF0)
   	{
       	// If not, it is a Long File Name entry
       	return FALSE;
   	}
   	else
   	{
       	return TRUE;
   	}
}

BYTE DetermineDirEntryType()
{
   	// Determine the Directory Entry type
   	if (CheckIfEOD())
   	{
       	// The Directory Entry is an End Of Directory flag
       	EODReached = TRUE;
       	return EOD;
   	}
   	else
   	{
       	// If not, we check the first byte to know if the entry is unused
       	if (DirEntry[0] == 0xE5)
       	{
           	return UNUSED;
       	}
       	else
       	{
           	// If it is used, we check if the Directory Entry corresponds to a Normal File Name
           	if (CheckIfNormalFile())
           	{
               	if (BitTest((DWORD)GetAttib(), 4)) return NDNAME; // it is a sub directory.
               	return NFNAME;
           	}
           	else
           	{
               	return LFNAME;
           	}
       	}
   	}
}

DWORD GetFileSize()
{
   	// Get the file size of the current Directory Entry
   	return Make32(DirEntry[0x1C + 3], DirEntry[0x1C + 2], DirEntry[0x1C + 1], DirEntry[0x1C + 0]);
}

void GetFileName(BYTE *bFileName)
{
   	// Get the 8+3 FAT32 short file name.
   	// This function may be changed in other platforms and/or architectures
   	// using the correct string functions.
	BYTE i;
   
   	for (i = 0; i < 8; i++)
   	{
       	*bFileName = DirEntry[i];
		bFileName++;
   	}

   	*bFileName = '.';
	bFileName++;

   	for (i = 8; i < 11; i++)
   	{
       	*bFileName = DirEntry[i];
		bFileName++;
   	}
}

void GetDirName(BYTE * bFileName)
{
   	// Get the 8 bytes FAT32 short file name.
   	// This function may be changed in other platforms and/or architectures
   	// using the correct string functions.
   	BYTE i;
   
   	for (i = 0; i < 11; i++)
   	{
       	*bFileName = DirEntry[i];
		bFileName++;
   	}

   	*bFileName = '\0';
	bFileName++;
}

DWORD GetFileFirstCluster()
{
   	// Get the file first cluster using the current Directory Entry data
   	return Make32(DirEntry[0x14 + 1], DirEntry[0x14], DirEntry[0x1A + 1], DirEntry[0x1A]);
}

DWORD ReadFAT(DWORD dwClusterNumber)
{
   	// This might be one of the most complicated functions in the entire FAT32
   	// implementation. It computes the next cluster of a file/directory reading the
   	// FAT table.

   	// Calculate the sector number (after FAT LBA Begin) in which we can find
   	// the cluster information we are looking for.
   
	DWORD dwCalc;
   	DWORD dwOffset;
   	DWORD dwRest;
   	DWORD dwSOff;
   
   	dwCalc = dwClusterNumber / ((DWORD)BytesPerSector / 4);

   	// Calculate the LBA of the sector containing the information
   	dwOffset = FatBeginLBA + dwCalc;

   	// Calculate the offset within the sector that has been read
   	dwRest = dwClusterNumber - (dwCalc * ((DWORD)BytesPerSector / 4));
   	dwSOff = dwRest * 4;

   	// Read the sector
   	//ReadSectorRelativeToPartition(offset);
	ReadSector(dwOffset);
   
	// Read the next cluster number
   	return Make32(SectorBuffer[dwSOff + 3], SectorBuffer[dwSOff + 2], SectorBuffer[dwSOff + 1], SectorBuffer[dwSOff]);
}

void OpenDirectory(DWORD dwDirectoryCluster)
{
   	// Initialize the variables needed to follow the directory pointed by
   	// the passed argument
   	DirectoryCurrentCluster = dwDirectoryCluster;
   	DirectoryCurrentLBA = ComputeLBA(DirectoryCurrentCluster);
   	DirectoryCurrentDirEntry = 0;
   	DirectoryCurrentSector = 0;

   	EODReached = FALSE;
   	DirectoryOpened = TRUE;
}

FILE_DESCRIPTOR FindNextFile()
{
   	// Define the variables that will be used
   	FILE_DESCRIPTOR fdFile;
   	BOOL bFileFound;

   	BYTE i;

   	// Set initial values
   	bFileFound = FALSE;
   
   	fdFile.Attributes = 0;
   	fdFile.FirstLBA = 0;
   	fdFile.FirstCluster = 0;
   	fdFile.Size = 0;

   	// Read the next sector of the cluster
   	//ReadSectorRelativeToPartition(DirectoryCurrentLBA);
	ReadSector(DirectoryCurrentLBA);

   	// If the directory has been opened with OpenDirectory
   	if (DirectoryOpened)
   	{
       	// Follow the Directory Entries looking for files
       	do
       	{
           	// Read the next directory entry
           	ReadDirEntry(DirectoryCurrentDirEntry);

            switch (DetermineDirEntryType())
			{
               	// If the directory entry is a Normal Directory Name
               	case NDNAME:
                  	// We are in presence of a sub directory, and we get its information
                  	GetDirName(&fdFile.Name[0]);
                  	fdFile.Attributes = GetAttib();
                  	fdFile.FirstCluster = GetFileFirstCluster();
                  	fdFile.FirstLBA = ComputeLBA(fdFile.FirstCluster) + FAT16Offset;

                  	// Read the last computed sector, to re-fill the SectorBuffer
                  	// that could have been overwritten when obtaining file information.
                  	//ReadSectorRelativeToPartition(DirectoryCurrentLBA);  ????
   
                  	// Set the File Found flag
                  	bFileFound = TRUE;
               		break;

               	// If the directory entry is a Normal File Name
               	case NFNAME:
                  	// And the Attribute is 0x20
                  	if (GetAttib() == 0x20)
                  	{
                      	// We are in presence of a file, and we get its information
                      	GetFileName(&fdFile.Name[0]);
                      	fdFile.Size = GetFileSize();
                      	fdFile.Attributes = GetAttib();
                      	fdFile.FirstCluster = GetFileFirstCluster();
                      	fdFile.FirstLBA = ComputeLBA(fdFile.FirstCluster) + FAT16Offset;
  
                      	// Read the last computed sector, to re-fill the SectorBuffer
                      	// that could have been overwritten when obtaining file information.
                      	//ReadSectorRelativeToPartition(DirectoryCurrentLBA);  ?????
   
                      	// Set the File Found flag
                      	bFileFound = TRUE;
                  	}               
               		break;
			}
         
           	// Increment the current Directory Entry index
           	DirectoryCurrentDirEntry++;
           	
			// If the Directory Entry index is greater than 15 (only 16
           	// directory entries could be found into a sector)
           	if (DirectoryCurrentDirEntry > 15)
           	{
               	// Reset the Directory Entry index
               	DirectoryCurrentDirEntry = 0;

               	// Increment the current sector index
               	DirectoryCurrentSector++;
               	
				// If the sector index is minor than the SectorsPerCluster
               	if (DirectoryCurrentSector < SectorsPerCluster)
               	{
                   	// We move to the next sector
                   	DirectoryCurrentLBA++;
               	}
               	// If not, we need to move to another cluster
               	else
               	{
                   	// And we compute the next cluster first sector
                   	DirectoryCurrentSector = 0;
                   	DirectoryCurrentCluster = ReadFAT(DirectoryCurrentCluster);
                   	DirectoryCurrentLBA = ComputeLBA(DirectoryCurrentCluster);
               	}

               	// Read the next sector computed before
               	//ReadSectorRelativeToPartition(DirectoryCurrentLBA);
				ReadSector(DirectoryCurrentLBA);
           	}
       	}
       	// Do this until an EOD is found or the File Found flag is set
       	while ((!CheckIfEOD()) && (!bFileFound));
   	}

   	// Return the File Descriptor
   	return fdFile;
}

void CloseDirectory()
{
   	// Close the directory
   	DirectoryOpened = FALSE;
}

void OpenFile(FILE_DESCRIPTOR fdFile)
{
   	// Initialize the variables needed to follow the file pointed by
   	// the passed File Descriptor.
   	FileCurrentCluster = fdFile.FirstCluster;
   	FileCurrentLBA = ComputeLBA(FileCurrentCluster) + FAT16Offset;
   	FileCurrentSector = 0;
   	FileLengthCount = 0;
   	FileSectorPosition = 0;

   	FileOpened = TRUE;
}

FILE_DATA ReadFile(FILE_DESCRIPTOR fdFile, DWORD dwCount)
{
   	// Define the variables that will be used
   	FILE_DATA fdData;
	WORD wBytesRead = 0;
	WORD i;

   	// Set initial values and initialize the Buffer vector
   	fdData.BytesRead = 0;
   	fdData.IsEOF = TRUE;

   	// Read the next file sector
   	//ReadSectorRelativeToPartition(FileCurrentLBA);
	ReadSector(FileCurrentLBA);

   	// If the file has been opened with OpenFile()
   	if (FileOpened)
   	{
       	// Read 'count' bytes from the current file sector
       	for (i = 0; i < dwCount; i++)
       	{
           	// Increment the length count
           	FileLengthCount++;
           	// Compare the length count to the total file size
           	if (FileLengthCount <= fdFile.Size)
           	{
               	// We only read if we have not reached the total file size
               	fdData.Buffer[i] = SectorBuffer[FileSectorPosition + i];
               	// Count the bytes that have been read
               	wBytesRead++;
           	}
       	}
       
       	// Set the bytes read into the resulting FileData structure
       	fdData.BytesRead = wBytesRead;

       	// Set the current sector position in order to continue from there the next time
       	FileSectorPosition += (WORD)wBytesRead;

       	// If the sector position is greater than the maximum sector size
       	if (FileSectorPosition > (BUFFER_SIZE - 1))
       	{
           	// Reset the file sector position
           	FileSectorPosition = 0;

           	// Increment the current sector
           	FileCurrentSector++;
           	// If the current sector is minor than the Sectors Per Cluster
           	if (FileCurrentSector < SectorsPerCluster)
           	{
               	// Move to the next sector of the cluster
               	FileCurrentLBA++;
           	}
           	// If not, read the FAT looking for the next cluster
           	else
           	{
               	// And then, compute the first cluster sector
               	FileCurrentSector = 0;
               	FileCurrentCluster = ReadFAT(FileCurrentCluster);
               	FileCurrentLBA = ComputeLBA(FileCurrentCluster) + FAT16Offset;
           	}
           	// Read the sector computer before
           	//ReadSectorRelativeToPartition(FileCurrentLBA);
			ReadSector(FileCurrentLBA);
       	}
       	// If the sector position is greater than the sector buffer size
       	else
       	{
           	// Check if the total file size has been reached
           	if (FileLengthCount >= fdFile.Size)
           	{
               	// Force an EOF (End of File)
               	FileCurrentCluster = 0xFFFFFFFF;
           	}
       	}

       	// If the current cluster is greater than 0xFFFFFFF8, we have an EOF
       	if (FileCurrentCluster < 0xFFFFFFF8)
       	{
           	fdData.IsEOF = FALSE;
       	}
       	else
       	{
           	fdData.IsEOF = TRUE;
       	}
   	}

   	// Return the FileData structure
   	return fdData;
}

void CloseFile()
{
   // Close the currently opened file
   FileOpened = FALSE;
}