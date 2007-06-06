/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
             dean [at] fourwalledcubicle [dot] com
                  www.fourwalledcubicle.com
*/

#include "SPI.h"

// ======================================================================================

/*
 NAME:      | SPI_SPIInit
 PURPOSE:   | Initializes the SPI subsystem ready for data transfers
 ARGUMENTS: | None
 RETURNS:   | None
*/
void SPI_SPIInit(void)
{
	PRR &= ~(1 << PRSPI);              // Enable the SPI system by clearing the power save register SPI disable bit

	SPCR = ((1 << SPE) | (1 << MSTR) | (1 << CPHA) | (1 << CPOL) | (1 << SPR0)); // Master, Sample falling edge (setup rising), Fcpu/16 speed (7.3MHz/16 = 467KHz)
}

/*
 NAME:      | SPI_SPITransmit
 PURPOSE:   | Transmits and recieves a single byte via the SPI subsystem
 ARGUMENTS: | None
 RETURNS:   | Byte recieved at time of transmission
*/
uint8_t SPI_SPITransmit(const uint8_t Data)
{
	SPDR = Data;                       // Loading a byte into the data register, data is shifted out automatically
	while (!(SPSR & (1 << SPIF)));     // Wait until transmission completed

	return SPDR;
}

/*
 NAME:      | SPI_SPITransmitWord
 PURPOSE:   | Transmits two bytes and recieves a single byte via the SPI subsystem
 ARGUMENTS: | None
 RETURNS:   | Byte recieved at time of second byte transmission
*/
uint8_t SPI_SPITransmitWord(const uint16_t Data)
{
	// Routine repeats code in SPI_SPITransmit rather than calling it to increase speed

	SPDR = (Data >> 8);                // Load high byte into the data register, data is shifted out automatically
	while (!(SPSR & (1 << SPIF)));     // Wait until transmission completed

	SPDR = (Data & 0xFF);              // Load low byte into the data register, data is shifted out automatically
	while (!(SPSR & (1 << SPIF)));     // Wait until transmission completed

	return SPDR;
}
