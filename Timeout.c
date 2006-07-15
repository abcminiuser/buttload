#include "Timeout.h"

const uint8_t AutoSleepTOValues[5] PROGMEM = {   0,   15,  30,  60,  120};

volatile uint8_t  PacketTimeOutTicks   = 0;
volatile uint8_t  PacketTimeOut        = FALSE;

volatile uint16_t SleepTimeOutTicks    = 0;
volatile uint16_t TicksBeforeAutoSleep = 0;

// ======================================================================================

// Packet Timeout = ((F_CPU / 1024) / (240 * TIMEOUT_TICKSBEFORETIMEOUT)) per second
ISR(TIMER2_COMP_vect, ISR_NOBLOCK)
{
	uint8_t* ErrStrPtr = NULL;

	if (PacketTimeOutTicks++ == TIMEOUT_PACKET_TIMEOUTTICKS)
	{
		PacketTimeOutTicks = 0;
		PacketTimeOut      = TRUE;
	}

	// Packet Timeout is used to check for internal faults:
	if (UCSRA & (1 << DOR))
	  ErrStrPtr = PSTR("DATA OVR");
	else if (UCSRA & (1 << FE))
	  ErrStrPtr = PSTR("FRAME ERR");
	else if (BuffElements == BUFF_BUFFLEN)
	  ErrStrPtr = PSTR("BUFF OVF");

	if (ErrStrPtr != NULL) // Local error string pointer non-null (an error has occured)
	  CRASHPROGRAM(ErrStrPtr);
}

// Autosleep Timeout = (TicksBeforeAutoSleep / 10) secs between timeouts
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

	if (NewTicksIndex == 0xFF) // Blank EEPROM protection
	  NewTicksIndex = 4;

	TIMSK1 = (1 << TOIE1);
	TicksBeforeAutoSleep = ((pgm_read_byte(&AutoSleepTOValues[NewTicksIndex]) << 1) * 5); // ((x << 1) * 5) == (x * 10)

	TIMEOUT_SLEEP_TIMEOUT_RESET();
	
	if (NewTicksIndex)
	  TIMEOUT_SLEEP_TIMER_ON();
	else
	  TIMEOUT_SLEEP_TIMER_OFF();
}
