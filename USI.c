/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "USI.h"

void USI_SPIInitMaster()
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

void USI_SPIOff(void)
{
	DDRE  &= ~((1 << USI_DATAOUT_PIN) | (1 << USI_CLOCK_PIN));
	PORTE &= ~((1 << USI_DATAIN_PIN)  | (1 << USI_DATAOUT_PIN) | (1 << USI_CLOCK_PIN));

	MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE);
}

void USI_SPIToggleClock(void)
{
	_delay_us(100);
	USICR = (USICONTROLREGS);
	_delay_us(100);
	USICR = (USICONTROLREGS | (1 << USICLK));
	_delay_us(100);
}

uint8_t USI_SPITransmitWord(const uint16_t val)
{
	USI_SPITransmit((uint8_t)(val >> 8));
	return USI_SPITransmit((uint8_t)val);
}
