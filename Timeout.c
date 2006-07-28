#include "Timeout.h"

const uint8_t AutoSleepTOValues[5] PROGMEM = {  0,  15, 30,  60, 120};

volatile uint8_t  PacketTimeOutTicks   = 0;
volatile uint8_t  PacketTimeOut        = FALSE;

volatile uint16_t SleepTimeOutTicks    = 0;
volatile uint16_t TicksBeforeAutoSleep = 0;

// ======================================================================================

// Packet Timeout = ((F_CPU / 1024) / (240 * TIMEOUT_TICKSBEFORETIMEOUT)) per second
ISR(TIMER2_COMP_vect, ISR_NOBLOCK)
{
	if (PacketTimeOutTicks++ == TIMEOUT_PACKET_TIMEOUTTICKS)
	{
		PacketTimeOutTicks = 0;
		PacketTimeOut      = TRUE;
	}
}

// Autosleep Timeout = ((F_CPU / 1024) / (7200 * TIMEOUT_TICKSBEFORETIMEOUT)) per second
ISR(TIMER1_COMPA_vect, ISR_NOBLOCK)
{
	if (SleepTimeOutTicks++ == TicksBeforeAutoSleep)
	{
		TIMEOUT_SLEEP_TIMER_OFF();
		FUNCSleepMode();
		TOUT_SetupSleepTimer();
	}
}

// ======================================================================================

void TOUT_SetupSleepTimer(void)
{
	uint8_t NewTicksIndex = eeprom_read_byte(&EEPROMVars.AutoSleepValIndex);

	if (NewTicksIndex == 0xFF) // Blank EEPROM protection
	  NewTicksIndex = 4;

	TIMSK1 = (1 << OCIE1A);    // Compare interrupt enabled
	TCCR1B = (1 << WGM12);     // CTC mode
	OCR1A  = 7200;             // Match at 7200 * 1024 cycles (results in a compare rate of 1Hz)

	TicksBeforeAutoSleep = pgm_read_byte(&AutoSleepTOValues[NewTicksIndex]);

	TIMEOUT_SLEEP_TIMEOUT_RESET();
	
	if (NewTicksIndex)
	  TIMEOUT_SLEEP_TIMER_ON();
	else
	  TIMEOUT_SLEEP_TIMER_OFF();
}
