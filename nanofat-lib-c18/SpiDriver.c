#include "HardwareProfile.h"
#include "SpiDriver.h"

void SpiSetDivisor(BYTE divisor)
{
   SSPCON1 = (SSPCON1 & 0xF0) | divisor;
}

void SpiSetClockPolarity(BOOL polarity)
{
   CKP = polarity;
}

void SpiSetTXTransition(BOOL direction)
{
   CKE = direction;
}

void SpiSetSampleMode(BOOL mode)
{
   SMP = mode;
}

void SpiInit(void)
{
   SDI_TRIS = INPUT_PIN;
   SDO_TRIS = OUTPUT_PIN;
   SCK_TRIS = OUTPUT_PIN;
}

BYTE SpiRW(BYTE value)
{
   SSPBUF = value;
   while (!BF);
   return SSPBUF;
}
