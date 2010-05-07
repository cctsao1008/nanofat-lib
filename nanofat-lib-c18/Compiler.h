#ifndef __COMPILER_H
#define __COMPILER_H

// Microchip C18 compiler
#include <p18cxxx.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usart.h>

// Base RAM and ROM pointer types for given architecture
#define PTR_BASE		WORD
#define ROM_PTR_BASE	unsigned short long

// Definitions that apply to all 8-bit products
// (PIC18)
#define	__attribute__(a)

#define FAR                         far

// Microchip C18 specific defines
#define ROM                 	rom
#define strcpypgm2ram(a, b)		strcpypgm2ram(a,(far rom char*)b)

#endif