#include "Timeout.h"

volatile uint8_t  Ticks   = 0;
volatile uint8_t  TimeOut = FALSE;

// ======================================================================================

// Timeout = ((F_CPU / 1024) / (240 * TIMEOUT_TICKSBEFORETIMEOUT)) per second
ISR(TIMER2_COMP_vect, ISR_NOBLOCK)
{
	if (Ticks++ == TIMEOUT_TICKSBEFORETIMEOUT)
	{
	   Ticks   = 0;
	   TimeOut = TRUE;
	}
}
