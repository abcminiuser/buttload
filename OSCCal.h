/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef OSCCAL_H
#define OSCCAL_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <util/delay.h>

	#include "ISRMacro.h"

	// DEFINES:
	#define OSCCAL_TARGETCOUNT         (uint16_t)(F_CPU / (32768 / 256)) // (Target Freq / Reference Freq)	
	
	// PROTOTYPES:
	void OSCCAL_Calibrate(void);
	
#endif
