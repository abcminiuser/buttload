/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
                  www.fourwalledcubicle.com
*/

#include "Timeout.h"

const    uint8_t  AutoSleepTOValues[5] PROGMEM = {   0,  15,  30,  60, 120};

volatile uint8_t  PacketTimeOutTicks  = 0;
volatile uint8_t  PacketTimeOut       = FALSE;

volatile uint8_t  SleepTimeOutSecs    = 0;
volatile uint8_t  SecsBeforeAutoSleep = 0;

// ======================================================================================

/*
 NAME:      | TIMER0_COMP_vect (ISR, non-blocking)
 PURPOSE:   | ISR to manage a timeout for recieving V2Protocol packets
 ARGUMENTS: | None
 RETURNS:   | None
*/
ISR(TIMER0_COMP_vect, ISR_NOBLOCK)
{
	// Packet Timeout: 32Hz
	
	if (PacketTimeOutTicks++ == TIMEOUT_PACKET_TIMEOUTTICKS)
	{
		PacketTimeOutTicks = 0;
		PacketTimeOut      = TRUE;
	}
}

/*
 NAME:      | TIMER2_COMP_vect (ISR, non-blocking)
 PURPOSE:   | ISR to manage a timeout for automatically sleeping after periods of user inactivity
 ARGUMENTS: | None
 RETURNS:   | None
*/
ISR(TIMER2_COMP_vect, ISR_NOBLOCK)
{
	// Autosleep Timeout: 1Hz

	if ((SecsBeforeAutoSleep) && (SleepTimeOutSecs++ == SecsBeforeAutoSleep))
	  MAIN_SleepMode();
}

// ======================================================================================

/*
 NAME:      | TOUT_SetupSleepTimer
 PURPOSE:   | Configures timer and globals and starts timer for automatically sleeping after user inactivity
 ARGUMENTS: | None
 RETURNS:   | None
*/
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
