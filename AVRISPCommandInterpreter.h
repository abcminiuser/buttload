/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef AICI_H
#define AICI_H

// INCLUDES:
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "Main.h"
#include "lcd_driver.h"
#include "SPI.h"
#include "ISPChipComm.h"
#include "V2Protocol.h"
#include "AVRISPCommandBytes.h"

// EXTERNAL VARIABLES:
extern const uint8_t AVRISPModeMessage[] PROGMEM;

// PROTOTYPES:
void AICI_InterpretPacket(void);

#endif