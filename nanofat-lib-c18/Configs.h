#ifndef _CONFIGS_
#define _CONFIGS_

//#pragma config PLLDIV   = 5         // (20 MHz crystal)
#pragma config CPUDIV   = OSC1_PLL2
//#pragma config USBDIV   = 2         // Clock source from 96MHz PLL/2
#pragma config FOSC     = HS
#pragma config FCMEN    = OFF
#pragma config IESO     = OFF
#pragma config PWRT     = OFF
#pragma config BOR      = OFF
//#pragma config VREGEN   = ON      //USB Voltage Regulator
#pragma config WDT      = OFF
#pragma config MCLRE    = OFF
#pragma config LVP      = OFF
#pragma config XINST    = OFF       // Extended Instruction Set

#endif