/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
                  www.fourwalledcubicle.com
*/

#include "USI.h"

// ======================================================================================

/*
 NAME:      | USI_SPIInitMaster
 PURPOSE:   | Initializes the USI subsystem, ready for SPI mode data transmission/reception
 ARGUMENTS: | None
 RETURNS:   | None
*/
void USI_SPIInitMaster(void)
{
 	DDRE  |=  (1 << USI_DATAOUT_PIN) | (1 << USI_CLOCK_PIN);
	DDRE  &= ~(1 << USI_DATAIN_PIN);
	PORTE |=  (1 << USI_DATAIN_PIN);
	PORTE &= ~(1 << USI_DATAOUT_PIN) | (1 << USI_CLOCK_PIN);
	
	// Configure USI to 3-wire master mode:
	USICR = (1 << USIWM0);

	// Get the clock delay value:
	USIDelay = eeprom_read_byte(&EEPROMVars.SCKDuration);
	
	if (USIDelay == 0xFF)
	  USIDelay = pgm_read_byte(&USISpeedIndex[0]);
}

/*
 NAME:      | USI_SPIOff
 PURPOSE:   | Disables the USI subsystem, including tristating the outputs
 ARGUMENTS: | None
 RETURNS:   | None
*/
void USI_SPIOff(void)
{
	DDRE  &= ~((1 << USI_DATAOUT_PIN) | (1 << USI_CLOCK_PIN));
	PORTE &= ~((1 << USI_DATAIN_PIN)  | (1 << USI_DATAOUT_PIN) | (1 << USI_CLOCK_PIN));

	MAIN_SetTargetResetLine(MAIN_RESET_INACTIVE);
}

/*
 NAME:      | USI_SPIToggleClock
 PURPOSE:   | Toggle the clock output of the USI once, used to sync communications
 ARGUMENTS: | None
 RETURNS:   | None
*/
void USI_SPIToggleClock(void)
{
	_delay_us(100);
	USICR = (USICONTROLREGS);
	_delay_us(100);
	USICR = (USICONTROLREGS | (1 << USICLK));
	_delay_us(100);
}

/*
 NAME:      | USI_SPITransmitWord
 PURPOSE:   | Transmits two bytes and recieves a single byte via the USI subsystem (in SPI mode)
 ARGUMENTS: | None
 RETURNS:   | None
*/
uint8_t USI_SPITransmitWord(const uint16_t val)
{
	USI_SPITransmit((uint8_t)(val >> 8));
	return USI_SPITransmit((uint8_t)val);
}
