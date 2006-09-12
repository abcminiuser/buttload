/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef TIMEOUT_H
#define TIMEOUT_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/pgmspace.h>
	
	#include "Main.h"
	#include "GlobalMacros.h"
	#include "ISRMacro.h"
	#include "Analogue.h"
	
	// DEFINES AND MACROS:
	#define TIMEOUT_PACKET_TIMEOUTTICKS   150   // Approx 1 timeout every 5 secs (which is close to the computer timout period)
	#define TIMEOUT_PACKET_TIMER_OFF()    MACROS{ TCCR0A = 0; TIMSK0 = 0; }MACROE
	#define TIMEOUT_PACKET_TIMER_ON()     MACROS{ PacketTimeOut = FALSE;  \
												  PacketTimeOutTicks = 0; \
												  TCNT0  = 0;             \
												  OCR0A  = 240;           \
												  TIMSK0 = (1 << OCIE0A); \
												  TCCR0A = ((1 << WGM01) | (1 << CS02) | (1 << CS01) | (1 << CS00)); }MACROE
	
	#define TIMEOUT_SLEEP_TIMER_OFF()     MACROS{ TCCR2A = 0; ASSR = 0; }MACROE
	#define TIMEOUT_SLEEP_TIMER_ON()      MACROS{ ASSR = (1 << AS2); TCCR2A = ((1 << WGM21) | (1 << CS22) | (1 << CS21)); }MACROE
	#define TIMEOUT_SLEEP_TIMEOUT_RESET() MACROS{ SleepTimeOutSecs = 0; TCNT2 = 0; }MACROE
	
	// EXTERNAL VARIABLES:
	extern volatile uint8_t  PacketTimeOut;
	extern volatile uint8_t  PacketTimeOutTicks;
	extern volatile uint8_t  SleepTimeOutSecs;
	extern volatile uint8_t  SecsBeforeAutoSleep;

	extern const    uint8_t  AutoSleepTOValues[5] PROGMEM;	

	// PROTOTYPES:
	void TOUT_SetupSleepTimer(void);
		
#endif
