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
#define TIMEOUT_TICKSBEFORETIMEOUT 150   // Approx 1 timeout every 5 secs (which is close to the computer timout period)

#define TIMEOUT_TIMER_ON()         MACROS{ TimeOut = FALSE;        \
                                           Ticks = 0;              \
                                           TCNT2 = 0;              \
                                           OCR2A = 240;            \
                                           TIMSK2 = (1 << OCIE2A); \
                                           TCCR2A = ((1 << WGM21) | (1 << CS22) | (1 << CS21) | (1 << CS20)); }MACROE
#define TIMEOUT_TIMER_OFF()        MACROS{ TCCR2A = 0; TIMSK2 = 0; }MACROE

// EXTERNAL VARIABLES:
extern volatile uint8_t   TimeOut;
extern volatile uint8_t   Ticks;

#endif
