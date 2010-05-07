#ifndef __HARDWARE_PROFILE_H
#define __HARDWARE_PROFILE_H

#include "GenericTypeDefs.h"
#include "Compiler.h"

#define GetInstructionClock() (5000000)

#define BusyUART()				BusyUSART()
#define CloseUART()				CloseUSART()
#define ConfigIntUART(a)		ConfigIntUSART(a)
#define DataRdyUART()			DataRdyUSART()
#define OpenUART(a,b)			OpenUSART(a,b)
#define ReadUART()				ReadUSART()
#define WriteUART(a)			WriteUSART(a)
#define getsUART(a,b,c)			getsUSART(b,a)
#define putsUART(a)				putsUSART(a)
#define getcUART()				ReadUSART()
#define putcUART(a)				WriteUSART(a)
#define putrsUART(a)			putrsUSART((far rom char*)a)

#define USART_TX		(LATCbits.LATC6)
#define USART_TX_TRIS	(TRISCbits.TRISC6)

#define INPUT_PIN 1
#define OUTPUT_PIN 0

#endif