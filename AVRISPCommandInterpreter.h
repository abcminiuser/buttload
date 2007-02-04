/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef AICI_H
#define AICI_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/pgmspace.h>
	#include <Delay.h>
	
	#include "LCD_Driver.h"
	#include "ISPChipComm.h"
	#include "V2Protocol.h"
	#include "AVRISPCommandBytes.h"
	
	// EXTERNAL VARIABLES:
	extern const char AVRISPModeMessage[] PROGMEM;
	
	// PROTOTYPES:
	void AICI_InterpretPacket(void);
	
#endif
