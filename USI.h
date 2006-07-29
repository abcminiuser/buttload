/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef USI_H
	#define USI_H
	
	#define USIDelay              GPIOR0 // Psudo-variable, GPIO register for speed
	#define USI_CONTROL_REG_FLAGS ((1 << USIWM0) | (1 << USICS0) | (1 << USITC))

	#ifndef __ASSEMBLER__
	
		// INCLUDES:
		#include <avr/io.h>
		#include <avr/interrupt.h>
		#include <avr/pgmspace.h>
		#include <util/delay.h>
			
		#include "GlobalMacros.h"
		#include "AVRISPCommandBytes.h"
		#include "ISRMacro.h"
		#include "Main.h"
			
		// USI PORT DEFINES:
		#define USI_CLOCK_PIN	   4
		#define USI_DATAIN_PIN	   5
		#define USI_DATAOUT_PIN	   6

		// OTHER DEFINES:
		#define USI_PRESET_SPEEDS  4
			
		// PROTOTYPES:
		void           USI_SPIInitMaster(void);
		void           USI_SPIOff(void);
		void           USI_SPIToggleClock(void);
		uint8_t        USI_SPITransmitWord(const uint16_t val);
		extern uint8_t USI_SPITransmit(const uint8_t val);
	
	#endif
#endif
