#ifndef _SD_DRIVER_
#define _SD_DRIVER_

#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "SpiDriver.h"

#define SD_CS		(LATAbits.LATA0)
#define SD_CS_TRIS	(TRISAbits.TRISA0)

#define SDAssert()		{ SD_CS = 0; }
#define SDDeassert()	{ SD_CS = 1; }

BYTE SDResponse(BYTE expected_response);
void SDConfigure(BYTE divisor);
BOOL SDInit(void);
BOOL SDWriteBlock(UINT32 block_number, BYTE *pBuffer);
BOOL SDReadBlock(UINT32 block_number, BYTE *pBuffer);

#endif