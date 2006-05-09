/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "SPI.h"

// ======================================================================================

void SPI_SPIInit(void)
{
	PRR &= ~(1 << PRSPI);              // Enable the SPI system by clearing the power save register SPI disable bit

	// Master, Sample falling edge (setup rising), Fosc/2 speed (8Mhz/2 = 4MHz)
	SPSR = (1 << SPI2X);
	SPCR = ((1 << SPE) | (1 << MSTR) | (1 << CPHA) | (1 << CPOL));
}

uint8_t SPI_SPITransmit(const uint8_t Data)
{
	SPDR = Data;                       // Loading a byte into the register starts the transmission
	while (!(SPSR & (1 << SPIF)));    // Wait until transmission completed
	return SPDR;
}

uint8_t SPI_SPITransmitWord(const uint16_t Data)
{
	SPI_SPITransmit((uint8_t)(Data >> 8));
	return SPI_SPITransmit((uint8_t)Data);
}