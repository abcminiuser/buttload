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
#define TIMEOUT_TICKSBEFORETIMEOUT 6   // Approx 1 timeout every 5 secs (which is computer timout period)

#define TIMEOUT_TIMER_ON()         TCCR1B = ((1 << CS10) | (1 << CS11));
#define TIMEOUT_TIMER_OFF()        TCCR1B = 0; Ticks = 0; TCNT1 = 0

// EXTERNAL VARIABLES:
extern volatile uint8_t   TimeOut;
extern volatile uint8_t   Ticks;

// PROTOTYPES:
void TIMEOUT_SetupTimeoutTimer(void);

#endif
