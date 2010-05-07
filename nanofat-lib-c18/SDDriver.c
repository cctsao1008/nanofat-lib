#include "SDDriver.h"

#pragma udata

BYTE SDResponse(BYTE expected_response)
{
   UINT16 count = 0x0FFF;
   
   while ((SpiRW(0xFF) != expected_response) && (--count > 0));
   
   if (count == 0)
   {
      return FALSE;
   } else {
      return TRUE;
   }
}

void SDConfigure(BYTE divisor)
{
   // Set SD chip select pin direction
   SD_CS_TRIS = 0;

   // Disable the SPI module if enabled
   SpiDisable();
   
   // Initialize the SPI port if not initialized
   SpiInit();
   
   // Slow down the SPI speed to minimum
   SpiSetDivisor(divisor);
   
   // Set SPI protocol parameters
   SpiSetTXTransition(SPI_TX_LOW_TO_HIGH);   // CKE = 0
   SpiSetClockPolarity(SPI_IDLE_HIGH);       // CKP = 1
   SpiSetSampleMode(SPI_SAMPLE_MIDDLE);      // SMP = 0
   
   // Enable the SPI port
   SpiEnable();
}

BOOL SDInit(void)
{
   BYTE res;
   UINT16 timeout;
   
   // Configure SD chip select
   SDDeassert();
   SDConfigure(SPI_DIV_64);
   
   // Just for debugging purposes
   //printf("SSPCON1 = 0x%X\r\n", SSPCON1);
   //printf("SSPSTAT = 0x%X\r\n", SSPSTAT);
   
   // Send at least 74 clocks
   SpiRW(0xFF);
   SpiRW(0xFF);
   SpiRW(0xFF);
   SpiRW(0xFF);
   SpiRW(0xFF);
   SpiRW(0xFF);
   SpiRW(0xFF);
   SpiRW(0xFF);
   SpiRW(0xFF);
   SpiRW(0xFF);
   
   // Assert the SD line
   SDAssert();
   
   // Send CMD0 to the SD card
   SpiRW(0x40);
   SpiRW(0x00);
   SpiRW(0x00);
   SpiRW(0x00);
   SpiRW(0x00);
   SpiRW(0x95);
   
   // Wait for the memory to be in idle state
   if (SDResponse(0x01) == FALSE)
   {
      //printf("No response from SD card!\r\n");
      return FALSE;
   }
   
   // De-assert the SD from the SPI bus
   SDDeassert();
   
   // Some extra clocks
   SpiRW(0xFF);
   
   // Send CMD1 to initialize the card
   timeout = 0;
   while (timeout < 0x0FFF)
   {
      timeout++;
      
      // Assert the SD card
      SDAssert();
      
      // Send CMD1
      SpiRW(0x41);
      SpiRW(0x00);
      SpiRW(0x00);
      SpiRW(0x00);
      SpiRW(0x00);
      SpiRW(0xFF);
      
      // Check for CMD1 response
      if (SDResponse(0x00) == TRUE)
      {
         // Got expected response, break out of loop
         SDDeassert();
         SpiRW(0xFF);
         break;
      } else {
         // must have timed out waiting for response, so loop again      
         SDDeassert();
         SpiRW(0xFF);
      }
   }
   
   // Check if we have a time out
   if (timeout >= 0xFE) return FALSE;
   
   // De-assert the SD card from the SPI bus
   SDDeassert();
   
   // Extra clocks
   SpiRW(0xFF);
   
   // Assert the SD card
   SDAssert();
   
   // Set the block size (512 bytes)
   SpiRW(0x50);
   SpiRW(0x00);
   SpiRW(0x00);
   SpiRW(0x02);
   SpiRW(0x00);
   SpiRW(0xFF);
   
   if (SDResponse(0x00) == FALSE)
   {
      return FALSE;
   }
   
   // De-assert the SD card from the SPI bus
   SDDeassert();
   
   // The card should has been initialized successfuly
   return TRUE;
}

BOOL SDWriteBlock(UINT32 block_number, BYTE *pBuffer)
{
   UINT16 i;
   UINT16 timeout;
   
   SDDeassert();
   SDConfigure(SPI_DIV_4);
   
   SDAssert();
   
   block_number *= 512;
   
   // Send write command
   SpiRW(0x58);
   SpiRW((BYTE)(block_number >> 24));
   SpiRW((BYTE)(block_number >> 16));
   SpiRW((BYTE)(block_number >> 8));
   SpiRW((BYTE)(block_number));
   SpiRW(0xFF);
   
   // Wait for response... exit if timeout
   if (SDResponse(0x00) == FALSE) return FALSE;
   
   // Send data token
   SpiRW(0xFE);
   
   // Write data
   for (i = 0; i < 512; i++)
   {
      SpiRW(*pBuffer);
	  pBuffer++;
   }
   
   // Dummy CRC
   SpiRW(0xFF);
   SpiRW(0xFF);
   
   timeout = 0x0FFF;
   while (((SpiRW(0xFF) & 0x1F) != 0x05) && timeout)
   {
      timeout--;
   }
   
   if (timeout == 0) return FALSE;
   
   SDDeassert();
   
   SpiDisable();
   
   return TRUE;
}

BOOL SDReadBlock(UINT32 block_number, BYTE *pBuffer)
{
   UINT16 i;
   
   SDDeassert();
   SDConfigure(SPI_DIV_4);
   
   SDAssert();
   
   block_number *= 512;
   
   // Send read command
   SpiRW(0x51);
   SpiRW((BYTE)(block_number >> 24));
   SpiRW((BYTE)(block_number >> 16));
   SpiRW((BYTE)(block_number >> 8));
   SpiRW((BYTE)(block_number));
   SpiRW(0xFF);
   
   // Wait for response... exit if timeout
   if (SDResponse(0x00) == FALSE) return FALSE;
   
   // Wait for data token
   if (SDResponse(0xFE) == FALSE) return FALSE;
   
   // Read data
   for (i = 0; i < 512; i++)
   {
      *pBuffer = SpiRW(0xFF);
	  pBuffer++;
   }
   
   // Dummy CRC
   SpiRW(0xFF);
   SpiRW(0xFF);
   
   SDDeassert();
   
   SpiDisable();
   
   return TRUE;
}