/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef PROGDATAFLASH_H
#define PROGDATAFLASH_H
	// INCLUDES:
	#include <avr/io.h>
	
	#include "Main.h"
	#include "V2Protocol.h"
	#include "Dataflash.h"
	#include "AVRISPCommandBytes.h"
	
	// EXTERNAL VARIABLES:
	extern const uint8_t DataFlashProgMode[] PROGMEM;
	
	// PROTOTYPES:
	void PD_InterpretAVRISPPacket(void);
	void PD_SetupDFAddressCounters(void);
	void PD_StoreDataflashByte(const uint8_t Data);
#endif
