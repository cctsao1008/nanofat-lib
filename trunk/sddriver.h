#byte LATA = 0xF89
#byte TRISA = 0xF92
#byte SSPSTAT = 0xFC7
#byte SSPCON1 = 0xFC6
#byte TRISB = 0xF93
#byte LATB = 0xF8A
#byte TRISC = 0xF94
#byte LATC = 0xF8B
#byte PORTB = 0xF81

#bit SDI_TRIS = TRISB.0
#bit SCK_TRIS = TRISB.1
#bit SDO_TRIS = TRISC.7
#bit SDI = PORTB.0
#bit SCK = LATB.1
#bit SDO = LATC.7

#bit SD_CS = LATA.5
#bit SD_CS_TRIS = TRISA.5
#bit CKE = SSPSTAT.6
#bit SMP = SSPSTAT.7
#bit CKP = SSPCON1.4
#bit SSPEN = SSPCON1.5

#define SPI_DIV_4    0x00
#define SPI_DIV_16   0x01
#define SPI_DIV_64   0x02

#define SPI_IDLE_HIGH   1
#define SPI_IDLE_LOW    0

#define SPI_TX_LOW_TO_HIGH 0
#define SPI_TX_HIGH_TO_LOW 1

#define SPI_SAMPLE_END     1
#define SPI_SAMPLE_MIDDLE  0

#define SpiEnable()    { SSPEN = 1; }
#define SpiDisable()   { SSPEN = 0; }

#define SDAssert()     { SD_CS = 0; }
#define SDDeassert()   { SD_CS = 1; }

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

void SpiSetDivisor(BYTE divisor)
{
   SSPCON1 = (SSPCON1 & 0xF0) | divisor;
}

void SpiSetClockPolarity(unsigned int1 polarity)
{
   CKP = polarity;
}

void SpiSetTXTransition(unsigned int1 direction)
{
   CKE = direction;
}

void SpiSetSampleMode(unsigned int1 mode)
{
   SMP = mode;
}

void SpiInit(void)
{
   SDI_TRIS = 1;
   SDO_TRIS = 0;
   SCK_TRIS = 0;
}

BYTE SDResponse(BYTE expected_response)
{
   unsigned int16 count = 0x0FFF;
   
   while ((SPI_READ(0xFF) != expected_response) && (--count > 0));
   
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

BYTE SDInit(void)
{
   BYTE res;
   unsigned int16 timeout;
   
   // Configure SD chip select
   SDDeassert();
   SDConfigure(SPI_DIV_64);
   
   // Send at least 74 clocks
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   
   // Assert the SD line
   SDAssert();
   
   // Send CMD0 to the SD card
   Spi_Write(0x40);
   Spi_Write(0x00);
   Spi_Write(0x00);
   Spi_Write(0x00);
   Spi_Write(0x00);
   Spi_Write(0x95);
   
   // Wait for the memory to be in idle state
   if (SDResponse(0x01) == FALSE)
   {
      //printf("No response from SD card!\r\n");
      return FALSE;
   }
   
   // De-assert the SD from the SPI bus
   SDDeassert();
   
   // Some extra clocks
   Spi_Write(0xFF);
   
   // Send CMD1 to initialize the card
   timeout = 0;
   while (timeout < 0x0FFF)
   {
      timeout++;
      
      // Assert the SD card
      SDAssert();
      
      // Send CMD1
      Spi_Write(0x41);
      Spi_Write(0x00);
      Spi_Write(0x00);
      Spi_Write(0x00);
      Spi_Write(0x00);
      Spi_Write(0xFF);
      
      // Check for CMD1 response
      if (SDResponse(0x00) == TRUE)
      {
         // Got expected response, break out of loop
         SDDeassert();
         Spi_Write(0xFF);
         break;
      } else {
         // must have timed out waiting for response, so loop again      
         SDDeassert();
         Spi_Write(0xFF);
      }
   }
   
   // Check if we have a time out
   if (timeout >= 0xFE) return FALSE;
   
   // De-assert the SD card from the SPI bus
   SDDeassert();
   
   // Extra clocks
   Spi_Write(0xFF);
   
   // Assert the SD card
   SDAssert();
   
   // Set the block size (512 bytes)
   Spi_Write(0x50);
   Spi_Write(0x00);
   Spi_Write(0x00);
   Spi_Write(0x02);
   Spi_Write(0x00);
   Spi_Write(0xFF);
   
   if (SDResponse(0x00) == FALSE)
   {
      return FALSE;
   }
   
   // De-assert the SD card from the SPI bus
   SDDeassert();
   
   // The card should has been initialized successfuly
   return TRUE;
}

BYTE SDOpenBlockForReading(unsigned int32 block_number)
{
   unsigned int16 i;
   
   SDDeassert();
   SDConfigure(SPI_DIV_4);
   
   SDAssert();
   
   block_number *= 512;
   
   // Send read command
   Spi_Write(0x51);
   Spi_Write((BYTE)(block_number >> 24));
   Spi_Write((BYTE)(block_number >> 16));
   Spi_Write((BYTE)(block_number >> 8));
   Spi_Write((BYTE)(block_number));
   Spi_Write(0xFF);
   
   // Wait for response... exit if timeout
   if (SDResponse(0x00) == FALSE) return FALSE;
   
   // Wait for data token
   if (SDResponse(0xFE) == FALSE) return FALSE;
   
   return TRUE;
}

BYTE SDOpenBlockForWriting(unsigned int32 block_number)
{
   unsigned int16 i;
   
   SDDeassert();
   SDConfigure(SPI_DIV_4);
   
   SDAssert();
   
   block_number *= 512;
   
   // Send read command
   Spi_Write(0x51);
   Spi_Write((BYTE)(block_number >> 24));
   Spi_Write((BYTE)(block_number >> 16));
   Spi_Write((BYTE)(block_number >> 8));
   Spi_Write((BYTE)(block_number));
   Spi_Write(0xFF);
   
   // Wait for response... exit if timeout
   if (SDResponse(0x00) == FALSE) return FALSE;
   
   // Send data token
   Spi_Write(0xFE);
   
   return TRUE;
}

void SDCloseReadBlock()
{
   // Dummy CRC
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   
   SDDeassert();
   
   SpiDisable();
}

BYTE SDCloseWrittenBlock()
{
   unsigned int16 timeout;
   BYTE response;
   
   timeout = 0x0FFF;
   response = TRUE;
   
   // Dummy CRC
   Spi_Write(0xFF);
   Spi_Write(0xFF);
   
   // Wait for SD to be ready
   while (((Spi_Read(0xFF) & 0x1F) != 0x05) && timeout)
   {
      timeout--;
   }
   
   if (timeout == 0)
   {
      response = FALSE;
   }
   
   SDDeassert();
   
   SpiDisable();
   
   return response;
}
