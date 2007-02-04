/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef OSCCAL_H
#define OSCCAL_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>

	#include "GlobalMacros.h"
	#include "Atomic.h"

	// DEFINES:
	#define OSCCAL_TARGETCOUNT         (uint16_t)(F_CPU / (32768 / 256)) // (Target Freq / Reference Freq)	
	
	// PROTOTYPES:
	void OSCCAL_Calibrate(void);
	
#endif
