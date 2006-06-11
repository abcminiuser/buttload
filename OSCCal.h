/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef OSCCAL_H
#define OSCCAL_H
	// DEFINES:
	#define OSCCAL_TARGETCOUNT         (uint16_t)(7372800UL / 128) // (Target Freq / Reference Freq)
	
	#define OSCCAL_SETSYSCLOCKSPEED(x) 	MACROS{ CLKPR = (1 << CLKPCE); CLKPR = x; }MACROE
	#define OSCCAL_BASECLOCKSPEED_8MHZ 0
	#define OSCCAL_BASECLOCKSPEED_1MHZ ((1 << CLKPS0) | (1 << CLKPS1))
	
	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	
	#include "Main.h"
	#include "ISRMacro.h"
	
	// PROTOTYPES:
	void OSCCAL_Calibrate(void);
#endif
