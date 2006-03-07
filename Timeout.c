#include "Timeout.h"

volatile uint8_t  Ticks   = 0;
volatile uint8_t  TimeOut = FALSE;

// ======================================================================================

// Timeout = ((F_CPU / 64) / (65535 * TIMEOUT_TICKSBEFORETIMEOUT)) per second
ISR(TIMER1_OVF_vect, ISR_NOBLOCK)
{
	if (Ticks++ == TIMEOUT_TICKSBEFORETIMEOUT)
	{
	   Ticks   = 0;
	   TimeOut = TRUE;
	}
}

// ======================================================================================

void TIMEOUT_SetupTimeoutTimer(void)
{
	TCCR1A = 0;
	TIMEOUT_TIMER_OFF();
	TCCR1C = 0;
	
	TIMSK1 = (1 << TOIE1); // Turn timer 1 overflow interrupt on
}
