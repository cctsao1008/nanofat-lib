#include "Configs.h"
#include "HardwareProfile.h"
#include "SDDriver.h"
#include "FATLib.h"
#include "Delay.h"

#define SPBRG 129 // 9600 bps

void UserInit(void);
void BlockToSerial(UINT32 block_number);

void main(void)
{
	WORD i, j;

	// Create a file descriptor
   	FILE_DESCRIPTOR fdFileDescriptor;
   
   	// Create a FileData structure
   	FILE_DATA fdFileData;

	// Create a Partition Entry structure
	PARTITION_ENTRY pePartitionEntry;

	// Initializes board
	UserInit();

	// Initializes SD card driver
	if (SDInit())
	{
		printf("SD card initialized successfully.\r\n\r\n");
	} else {
		printf("Can not initialize SD card!\r\n");
		while (TRUE);
	}

	// Find the first FAT partition
   	FindFirstFATPartition(&pePartitionEntry);
   
	if (pePartitionEntry.IsValid)
	{
    	if (pePartitionEntry.IsFAT32)
			printf("System is: FAT32\r\n");
		else
			printf("System is: FAT16\r\n");
   	} else {
      	printf("Memory card does not contain a valid FAT16 or FAT32 partition!\r\n");
   	}

	// Read the Volume ID to get some information important for further calculations
   	// If the function ReadVolumeID() returns True, it is a FAT32 volume.
   	if (ReadVolumeID(&pePartitionEntry))
   	{
       	printf("Bytes per sector: %d\r\n", BytesPerSector);
       	printf("Sectors per cluster: %d\r\n", SectorsPerCluster);
       	printf("Number of reserved sectors: %d\r\n", NumberOfReservedSectors);
       	printf("Number of FATs: %d\r\n", NumberOfFATs);
       	printf("Sectors per FAT: %d\r\n", SectorsPerFAT);
       	printf("Root directory first cluster: %d\r\n", RootDirectoryFirstCluster);
       	printf("FAT begin LBA: %d\r\n", FatBeginLBA);
       	printf("Data begin LBA: %d\r\n", (DataBeginLBA + FAT16Offset));
       	printf("Root Directory LBA: %d\r\n", ComputeLBA(RootDirectoryFirstCluster));
   
       	printf("\r\n");
	}

	// Loop for ever
	while (TRUE);
}

void UserInit(void)
{
	// Configures USART module
	OpenUART(USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, SPBRG);

	printf("FATLib v2.1 Test Application\r\nby Andres Olivares (andyolivares@gmail.com)\r\n\r\n");
}

void BlockToSerial(UINT32 block_number)
{
	WORD i, j;
	BYTE *p;

	SDReadBlock(block_number, &SectorBuffer[0]);
	
	p = &SectorBuffer[0];

	for (i = 0; i < 32; i++)
	{
		for (j = 0; j < 16; j++)
		{
			printf("%02X ", *p++);
		}
		printf("\r\n");
	}	
}