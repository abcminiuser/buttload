/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "OSCCal.h"

/* Code taken from Colin Oflynn from AVRFreaks and modified. His code originally used an externally
   divided 32768Hz clock on an external interrupt pin, but I changed that to use the timer 2 async
   mode with an overflow interrupt (clock source is the external 32768Hz crystal on the Butterfly.
   Code will calibrate to 7372800Hz for correct serial transmission at high baud rates.             */

void OSCCAL_Calibrate(void)
{
	uint8_t LoopCount = (0x7F / 2); // Maximum range is 128, and starts from the middle, so 64 is the max number of iterations required
    uint8_t SREGSave  = SREG;       // Save the SREG to preserve global interrupt status
	
	// Make sure all clock division is turned off (8MHz RC clock)
	CLKPR  = (1 << CLKPCE);
	CLKPR  = 0x00;

	// Inital OSCCAL of half its maximum
	OSCCAL = (0x7F / 2);
    
	// Disable timer interrupts
	TIMSK1 = 0;
	TIMSK0 = 0;
        
	// Set timer 2 to asyncronous mode (32.768KHz crystal)
	ASSR   = (1 << AS2);

	// Start both counters with no prescaling
	TCCR1B = (1 << CS10);
	TCCR2A = (1 << CS20);
	 	 
	// Wait until timer 2's external 32.768KHz crystal is stable
	while (ASSR & ((1 << TCN2UB) | (1 << TCR2UB) | (1 << OCR2UB)));
    
	// Clear the timer values
	TCNT1  = 0;
	TCNT2  = 0;
	
	// Kill interrupts to prevent them from messing with the results
	cli();

	while (LoopCount--)
	{
		// Wait until timer 2 overflows
		while (!(TIFR2 & (1 << TOV2)));
	
		// Stop timer 1 so it can be read
		TCCR1B = 0x00;
		
		// Check timer value against ideal constant
		if (TCNT1 > OSCCAL_TARGETCOUNT)      // Clock is running too fast
		  OSCCAL--;
		else if (TCNT1 < OSCCAL_TARGETCOUNT) // Clock is running too slow
		  OSCCAL++;
		
		// Clear timer 2 overflow flag
		TIFR2 |= (1 << TOV2);

		// Restart timer 1
		TCCR1B = (1 << CS10);

		// Reset counters
		TCNT1  = 0;
		TCNT2  = 0;
	}

	// Stop the timers
	TCCR1B = 0x00;
	TCCR2A = 0x00;

	// Turn off timer 2 asynchronous mode
	ASSR  &= ~(1 << AS2);
    
	// Restore previous SREG value
	SREG = SREGSave;
	
	return;
}
