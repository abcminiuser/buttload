/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef SPI_H
#define SPI_H

	// INCLUDES:
	#include <avr/io.h>
	
	#include "GlobalMacros.h"
	#include "Main.h"
	
	// MACROS:
	#define SPI_SPIOFF()	     MACROS{ PRR |= (1 << PRSPI); }MACROE
	
	// PROTOTYPES:
	void    SPI_SPIInit(void);
	uint8_t SPI_SPITransmit(const uint8_t Data);
	uint8_t SPI_SPITransmitWord(const uint16_t Data);
	
#endif
