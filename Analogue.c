/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
                  www.fourwalledcubicle.com
*/

#include "Analogue.h"

// ======================================================================================

/*
 NAME:      | AN_GetADCValue
 PURPOSE:   | Enables, reads and disables a requested ADC channel and returns its 10-bit value
 ARGUMENTS: | Channel: Channel number to read
 RETURNS:   | 16 bit ADC value
*/
uint16_t AN_GetADCValue(const uint8_t Channel)
{
	uint16_t ADCResult = 0;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		PORTF  |= (1 << 3);                    // Enable VCP (analogue circuitry power)
		PRR    &= ~(1 << PRADC);               // Enable ADC circuitry
	
		ADCSRA  = ((1 << ADEN)  | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)); // Enable ADC at Fcpu/128
		ADMUX   = ((1 << REFS0) | Channel);    // Set reference to AVCC, set channel

		ADCSRA |= (1 << ADSC);                 // Start conversion
		while (!(ADCSRA & (1 << ADIF)));       // Wait until conversion complete
		ADCSRA |= ((1 << ADIF) | (1 << ADSC)); // Clear the conversion complete flag (requires logic 1 to clear) and start another conversion
		while (!(ADCSRA & (1 << ADIF)));       // Wait until conversion complete		
		ADCResult = ADC;                       // Save result of the second conversion
	
		ADMUX   = 0;                           // Turn off ADC reference
		ADCSRA  = 0;                           // Turn off ADC
	
		PRR    |= (1 << PRADC);                // Disable ADC circuitry
		PORTF  &= ~(1 << 3);                   // Disable VCP (analogue circuitry power)

	}

	return ADCResult;                          // Return result
}
