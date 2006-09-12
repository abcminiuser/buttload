/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

/* This module checks the status of the Butterfly's battery, using the (fixed) voltage
   on the Green status LED (if present) as a reference. This is odd in that it works in
   reverse; the AREF is connected to the Butterfly's battery while the ADC channel is
   connected to the fixed voltage. This means that as the battery loses voltage, the ADC
   value returned should increase as the AREF value lowers, due to the scale being
   "squished" into a smaller range. Thanks to Nard who gave me the idea to do this      */

#include "BattVoltage.h"

#define BV_BATT_LOW_VOLTAGE  2.6
#define BV_REF_VOLTAGE       1.5
#define BV_LOWBATT_ADC_VAL   (uint8_t)(((1024 / BV_BATT_LOW_VOLTAGE) * BV_BATT_REF_VOLTAGE) >> 2)

// ======================================================================================

uint8_t BV_GetBatteryADCValue(void)
{
	uint8_t  GreenStatLedValue = (PORTF & MAIN_STATLED_GREEN); // Get current green status LED value
	uint16_t ADCResult = 0;
	
	PORTF &= ~MAIN_STATLED_GREEN;            // Set green status LED low (off)
	
	if (!(PINF & (1 << 7)))                  // Check if PF7 is low - it is linked to the green status LED, if so voltage check can commence
	{
		PORTF  &= ~(1 << 7);                 // Disable pullup on the PF7 port pin
		PRR    &= ~(1 << PRADC);             // Enable ADC circuitry
		ADCSRA  = ((1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0)); // Enable ADC at Fcpu/128
		ADMUX   = ((1 << REFS0) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));   // Set reference to AVCC, choose channel 7 (PF7)
		
		for (uint8_t ConvNum = 0; ConvNum < 10; ConvNum++)
		{
			ADCSRA  |= (1 << ADSC);          // Start conversion
			while (!(ADCSRA & (1 << ADIF))); // Wait until conversion complete
			ADCSRA  |= (1 << ADIF);          // Clear the conversion complete flag (requires logic 1 to clear)
			ADCResult = ADC;                 // Save result
		}

		ADCSRA  = 0;                         // Turn off ADC
		PRR    |= (1 << PRADC);              // Disable ADC circuitry
		PORTF  |= (1 << 7);                  // Enable pullup on the PF7 port pin
	}
	
	PORTF |= GreenStatLedValue;              // Restore green status LED value

	return (ADCResult >> 2);                 // Return upper 8 bits of result
}
