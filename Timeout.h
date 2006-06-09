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
	
	#include "Main.h"
	#include "ISRMacro.h"
	
	// DEFINES AND MACROS:
	#define TIMEOUT_PACKET_TIMEOUTTICKS   150   // Approx 1 timeout every 5 secs (which is close to the computer timout period)
	#define TIMEOUT_PACKET_TIMER_OFF()    MACROS{ TCCR2A = 0; TIMSK2 = 0; }MACROE
	#define TIMEOUT_PACKET_TIMER_ON()     MACROS{ PacketTimeOut = FALSE;  \
												  PacketTimeOutTicks = 0; \
												  TCNT2 = 0;              \
												  OCR2A = 240;            \
												  TIMSK2 = (1 << OCIE2A); \
												  TCCR2A = ((1 << WGM21) | (1 << CS22) | (1 << CS21) | (1 << CS20)); }MACROE
	
	#define TIMEOUT_SLEEP_TIMER_OFF()     MACROS{ TCCR1B = 0; }MACROE
	#define TIMEOUT_SLEEP_TIMER_ON()      MACROS{ TCCR1B = (1 << CS10); }MACROE
	#define TIMEOUT_SLEEP_TIMEOUT_RESET() MACROS{ SleepTimeOutTicks = 0; TCNT1 = 0; }MACROE
	
	// PROTOTYPES:
	void TOUT_SetupSleepTimer(void);
	
	// EXTERNAL VARIABLES:
	extern volatile uint8_t   PacketTimeOut;
	extern volatile uint8_t   PacketTimeOutTicks;
	extern volatile uint16_t  SleepTimeOutTicks;
	extern const    uint8_t   AutoSleepTOValues[5];
#endif
