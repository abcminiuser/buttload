/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
             dean [at] fourwalledcubicle [dot] com
                  www.fourwalledcubicle.com
*/

#ifndef OSCCAL_H
#define OSCCAL_H

	// INCLUDES:
	#include <avr/io.h>

	#include "Atomic.h"
	#include "GlobalMacros.h"

	// DEFINES:
	#define OSCCAL_TARGETCOUNT         (uint16_t)(F_CPU / (32768 / 256)) // (Target Freq / Reference Freq)	
	
	// PROTOTYPES:
	void OSCCAL_Calibrate(void);
	
#endif
