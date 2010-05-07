#ifndef __DELAY_H
#define __DELAY_H

#include "Compiler.h"
#include "HardwareProfile.h"
#include <delays.h>

#if !defined(GetInstructionClock)
	#error GetInstructionClock() must be defined.
#endif

#define Delay10us(us)		Delay10TCYx(((GetInstructionClock()/1000000)*(us)))
#define DelayMs(ms)												\
	do																\
	{																\
		unsigned int _iTemp = (ms); 								\
		while(_iTemp--)												\
			Delay1KTCYx((GetInstructionClock()+999999)/1000000);	\
	} while(0)

#endif
