#include "Timeout.h"

volatile uint8_t  PacketTimeOutTicks   = 0;
volatile uint8_t  PacketTimeOut        = FALSE;

volatile uint16_t SleepTimeOutTicks    = 0;
volatile uint16_t TicksBeforeAutoSleep = 0;

const uint8_t   AutoSleepTOValues[5] PROGMEM = {   0,   15,  30,  60,  120};

// ======================================================================================

// Packet Timeout = ((F_CPU / 1024) / (240 * TIMEOUT_TICKSBEFORETIMEOUT)) per second
ISR(TIMER2_COMP_vect, ISR_NOBLOCK)
{
	if (PacketTimeOutTicks++ == TIMEOUT_PACKET_TIMEOUTTICKS)
	{
		PacketTimeOutTicks   = 0;
		PacketTimeOut        = TRUE;
	}
}

// Autosleep Timeout = (TIMEOUT_TICKSBEFORETIMEOUT / 10) secs between timeouts
ISR(TIMER1_OVF_vect, ISR_NOBLOCK)
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

	if (NewTicksIndex == 0xFF)
	  NewTicksIndex = 4;

	TIMSK1 = (1 << TOIE1);
	TicksBeforeAutoSleep = (pgm_read_byte(&AutoSleepTOValues[NewTicksIndex]) * 10);

	TIMEOUT_SLEEP_TIMEOUT_RESET();
	
	if (NewTicksIndex)
	  TIMEOUT_SLEEP_TIMER_ON();
	else
	  TIMEOUT_SLEEP_TIMER_OFF();
}
