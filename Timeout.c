/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "Timeout.h"

const    uint8_t  AutoSleepTOValues[5] PROGMEM = {   0,  15,  30,  60, 120};

volatile uint8_t  PacketTimeOutTicks  = 0;
volatile uint8_t  PacketTimeOut       = FALSE;

volatile uint8_t  SleepTimeOutSecs    = 0;
volatile uint8_t  SecsBeforeAutoSleep = 0;

// ======================================================================================

// Packet Timeout: 32Hz
ISR(TIMER0_COMP_vect, ISR_NOBLOCK)
{
	if (PacketTimeOutTicks++ == TIMEOUT_PACKET_TIMEOUTTICKS)
	{
		PacketTimeOutTicks = 0;
		PacketTimeOut      = TRUE;
	}
}

// Autosleep Timeout: 1Hz
ISR(TIMER2_COMP_vect, ISR_NOBLOCK)
{
	if (((SecsBeforeAutoSleep) && (SleepTimeOutSecs++ == SecsBeforeAutoSleep))
	   || (AN_GetADCValue(AN_CHANNEL_SLEEP) > AN_SLEEP_TRIGGER_VALUE))
	{
		MAIN_SleepMode();
	}
}

// ======================================================================================

void TOUT_SetupSleepTimer(void)
{
	uint8_t NewTicksIndex = eeprom_read_byte(&EEPROMVars.AutoSleepValIndex);

	if (NewTicksIndex > ARRAY_UPPERBOUND(AutoSleepTOValues))                // Blank EEPROM protection
	  NewTicksIndex = 4;

	TIMEOUT_SLEEP_TIMER_OFF();

	TIFR2  = ((1 << OCF2A) | (1 << TOV2)); // Clear any pending timer ISR flags
	TIMSK2 = (1 << OCIE2A);                // Compare interrupt enabled
	OCR2A  = TIMEOUT_HZ_TO_COMP(1, TIMEOUT_SRC_RTC, 256);                   // Compare value of 128
	
	SecsBeforeAutoSleep = pgm_read_byte(&AutoSleepTOValues[NewTicksIndex]); // Set new timeout value

	TIMEOUT_SLEEP_TIMEOUT_RESET();
	TIMEOUT_SLEEP_TIMER_ON();
}
