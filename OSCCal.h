/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef OSCCAL_H
#define OSCCAL_H

// DEFINES:
#define TARGETCOUNT (uint16_t)((7372800 / 128) - 5) // Compile time constant, ((Target Freq / Reference Freq) - 5)

// INCLUDES:
#include <avr/io.h>
#include <avr/interrupt.h>

#include "Main.h"
#include "ISRMacro.h"

// PROTOTYPES:
void OSCCAL_Calibrate(void);

#endif
