/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "OSCCal.h"

/* Code taken from Colin Oflynn from AVRFreaks and modified. His code originally used an externally
   divided 32768Hz clock on an external interrupt pin, but I changed that to use the timer 2 async
   mode with an overflow interrupt (clock source is the external 32768Hz crystal on the Butterfly.
   Code will calibrate to 7372800Hz for correct serial transmission at 115200 baud.                 */

volatile static uint16_t ActualCount;

void OSCCAL_Calibrate(void)
{
	uint8_t SREG_Backup;
	uint8_t LoopCount = (0x7F / 2); // Maximum range is 128, and starts from the middle, so 64 is the max number of iterations required
	uint8_t PrevOSCALValues[2] = {};
   
	// Make sure all clock division is turned off (8Mhz RC clock)
	OSCCAL_SETSYSCLOCKSPEED(OSCCAL_CLOCKSPEED_8MHZ);

	// Inital OSCCAL of half its maximum for speed
	OSCCAL = (0x7F / 2);

	// Save the SREG
	SREG_Backup = SREG;
    
	// Disable all timer 1 interrupts
	TIMSK1 = 0;
        
	// Set timer 2 to asyncronous mode (32.768KHz crystal)
	ASSR   = (1 << AS2);
        
	// Timer 2 overflow interrupt enable
	TIMSK2 = (1 << TOIE2);

	// Enable interrupts
	sei();

	// Start both counters with no prescaling
	TCCR1B = (1 << CS10);
	TCCR2A = (1 << CS20);
	 	 
	// Wait until timer 2's external 32.768KHz crystal is stable
	while (ASSR & ((1 << TCN2UB) | (1 << TCR2UB) | (1 << OCR2UB)));
    
	// Clear the timer values
	TCNT1  = 0;
	TCNT2  = 0;
    
	while (LoopCount--)
	{
		// Let it take a few readings (16ms, approx 2 readings)
		_delay_ms(16);

		PrevOSCALValues[1] = PrevOSCALValues[0];
		PrevOSCALValues[0] = OSCCAL;
        
		if (ActualCount > OSCCAL_UPPERCOUNTBOUND)       // Clock is running too fast
			OSCCAL--;
		else if (ActualCount < OSCCAL_LOWERCOUNTBOUND ) // Clock is running too slow
			OSCCAL++;
		else		                                     // Clock is just right
			break;
		
		// If the routine cannot find a value withing the count tollerance,
		// it will cause the OSCCAL to hover around the closest two values.
		// If the current value is the same as the two previous, exit the
		// routine as the best value has been found.
		if (OSCCAL == PrevOSCALValues[1])
		  break;
	}

	// Disable all timer interrupts
	TIMSK1 = 0;
	TIMSK2 = 0;
    
	// Stop the timers
	TCCR1B = 0x00;
	TCCR2A = 0x00;

	// Turn off timer 2 asynchronous mode
	ASSR  &= ~(1 << AS2);

	// Restore SREG
	SREG = SREG_Backup;
        
	return;
}

ISR(TIMER2_OVF_vect, ISR_BLOCK) // Occurs 32768/256 timers per second, or 128Hz
{
	// Stop timer 1 so it can be read
	TCCR1B = 0x00;
    
	// Record timer 1's value
	ActualCount = TCNT1;
	     
	// Reset counters and restart timer 1
	TCNT1  = 0;
	TCNT2  = 0;
	TCCR1B = (1 << CS10);
}
