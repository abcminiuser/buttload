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
