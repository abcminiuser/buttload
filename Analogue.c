/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "Analogue.h"

// ======================================================================================

uint16_t AN_GetADCValue(const uint8_t Channel)
{
	uint16_t ADCResult = 0;

	cli();
	
	PORTF  |= (1 << 3);                   // Enable VCP (analogue circuitry power)
	PRR    &= ~(1 << PRADC);              // Enable ADC circuitry

	ADCSRA  = ((1 << ADEN)  | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)); // Enable ADC at Fcpu/128
	ADMUX   = ((1 << REFS0) | Channel);   // Set reference to AVCC, set channel
		
	for (uint8_t ConvNum = 0; ConvNum < 5; ConvNum++)
	{
		ADCSRA   |= (1 << ADSC);          // Start conversion
		while (!(ADCSRA & (1 << ADIF)));  // Wait until conversion complete
		ADCSRA   |= (1 << ADIF);          // Clear the conversion complete flag (requires logic 1 to clear)
		ADCResult = ADC;                  // Save result
	}

	ADMUX   = 0;                          // Turn off ADC reference
	ADCSRA  = 0;                          // Turn off ADC

	PRR    |= (1 << PRADC);               // Disable ADC circuitry
	PORTF  &= ~(1 << 3);                  // Disable VCP (analogue circuitry power)

	sei();

	return ADCResult;                     // Return result
}
