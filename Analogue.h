/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef BATTVOLTAGE_H
#define BATTVOLTAGE_H
	
	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>

	#include "Main.h"
	#include "GlobalMacros.h"
	#include "Atomic.h"
	
	// PROTOTYPES:
	uint16_t AN_GetADCValue(const uint8_t Channel) ATTR_WARN_UNUSED_RESULT;
	
	// DEFINES:
	#define AN_CHANNEL_SLEEP        (1 << MUX0)
	
	#define AN_SLEEP_TRIGGER_VALUE  0x50
	
#endif
