#ifndef _SPI_DRIVER_
#define _SPI_DRIVER_

#include "GenericTypeDefs.h"
#include "Compiler.h"

#define SDI_TRIS (TRISBbits.TRISB0)
#define SDI		 (PORTBbits.PORTB0)
#define SCK_TRIS (TRISBbits.TRISB1)
#define SCK		 (LATBbits.LATB1)
#define SDO_TRIS (TRISCbits.TRISC7)
#define SDO		 (LATCbits.LATC7)

#define SMP		(SSPSTATbits.SMP)
#define CKE 	(SSPSTATbits.CKE)
#define BF		(SSPSTATbits.BF)
#define SSPEN	(SSPCON1bits.SSPEN)
#define CKP		(SSPCON1bits.CKP)

#define SPI_DIV_4    0x00
#define SPI_DIV_16   0x01
#define SPI_DIV_64   0x02

#define SPI_IDLE_HIGH   1
#define SPI_IDLE_LOW    0

#define SPI_TX_LOW_TO_HIGH 0
#define SPI_TX_HIGH_TO_LOW 1

#define SPI_SAMPLE_END     1
#define SPI_SAMPLE_MIDDLE  0

#define SpiSend(x)	{ SSPBUF = x; }
#define SpiClocks() { while (!SSPSTATbits.BF); }

#define SpiEnable()		{ SSPEN = 1; }
#define SpiDisable()	{ SSPEN = 0; }

void SpiSetDivisor(BYTE divisor);
void SpiSetClockPolarity(BOOL polarity);
void SpiSetTXTransition(BOOL direction);
void SpiSetSampleMode(BOOL mode);

void SpiInit(void);
BYTE SpiRW(BYTE value);

#endif