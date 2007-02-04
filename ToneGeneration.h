/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef TONEGENERATION_H
#define TONEGENERATION_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/pgmspace.h>
	
	#include "EEPROMVariables.h"
	#include "GlobalMacros.h"
	#include "Main.h"
	
	// DEFINES AND MACROS:
	#define TONEGEN_SEQ_STARTUP    &ToneSeq_Startup[0]
	#define TONEGEN_SEQ_SYNCFAIL   &ToneSeq_SyncFail[0]
	#define TONEGEN_SEQ_PROGDONE   &ToneSeq_ProgDone[0]
	#define TONEGEN_SEQ_PROGFAIL   &ToneSeq_ProgFail[0]
	#define TONEGEN_SEQ_WAITWRITE  &ToneSeq_VolTest[0]
	#define TONEGEN_SEQ_VOLTEST    &ToneSeq_VolTest[0]
	#define TONEGEN_SEQ_SLEEP      &ToneSeq_Sleep[0]
	#define TONEGEN_SEQ_RESUME     &ToneSeq_Resume[0]
	#define TONEGEN_SEQ_ERROR      &ToneSeq_Error[0]
	
	#define TONEGEN_GET_TONE_VOL() MACROS{ ToneVol = eeprom_read_byte(&EEPROMVars.ToneVolume); }MACROE
	
	// EXTERNAL VARIABLES:
	extern const uint8_t  ToneSeq_Startup[]  PROGMEM;
	extern const uint8_t  ToneSeq_SyncDone[] PROGMEM;
	extern const uint8_t  ToneSeq_SyncFail[] PROGMEM;
	extern const uint8_t  ToneSeq_ProgDone[] PROGMEM;
	extern const uint8_t  ToneSeq_ProgFail[] PROGMEM;
	extern const uint8_t  ToneSeq_VolTest[]  PROGMEM;
	extern const uint8_t  ToneSeq_Sleep[]    PROGMEM;
	extern const uint8_t  ToneSeq_Resume[]   PROGMEM;	
	extern const uint8_t  ToneSeq_Error[]    PROGMEM;	
	
	extern uint8_t  ToneVol;
	
	// PROTOTYPES:
	void TG_PlayToneSeq(const uint8_t* Sequence);

#endif
