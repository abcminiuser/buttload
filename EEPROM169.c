/* 
	Workaround since the avr-libc has no support
	for the ATmega169 eeprom at the time of 
	writing this (avr-libc V1.0.4) 

	This is based on the ATMEL Databook for the m169, 
	with some changes in the interfaces and interrupt
	control
	
	done by Martin Thomas, KL, .de
*/

#include <inttypes.h>
#include <avr/interrupt.h>
#include "eeprom169.h"

uint8_t eeprom_read_byte_169(const uint16_t *addr)
{
	cli();
	/* Set up address register */
	EEAR = *addr;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	sei();
	/* Return data from Data Register */
	return EEDR;
}

void eeprom_write_byte_169(const uint16_t *addr, uint8_t val) 
{
	if(eeprom_read_byte_169(addr) == val) // Compare stored value with new value for match
		return; // Don't re-write the same value and waste EEPROM life

	cli();
	/* Set up address and Data Registers */
	EEAR = *addr;
	EEDR = val;
	/* Write logical one to EEMWE */
	EECR |= (1<<EEMWE);
	/* Start eeprom write by setting EEWE */
	EECR |= (1<<EEWE);
	sei();
	/* Wait for completion */
	while(EECR & (1<<EEWE));
}
