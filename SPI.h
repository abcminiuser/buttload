/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
             dean [at] fourwalledcubicle [dot] com
                  www.fourwalledcubicle.com
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
	void SPI_SPIInit(void);

	// INLINE FUNCTIONS:
	static inline uint8_t SPI_SPITransmit(const uint8_t Data) ATTR_ALWAYS_INLINE;
	static inline uint8_t SPI_SPITransmit(const uint8_t Data)
	{
		SPDR = Data;                       // Loading a byte into the data register, data is shifted out automatically
		while (!(SPSR & (1 << SPIF)));     // Wait until transmission completed

		return SPDR;
	}
	
#endif
