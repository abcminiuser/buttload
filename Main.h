/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef MAIN_H
#define MAIN_H

	// DEBUG SWITCHES
	//#define DEBUG_JTAGON
	// END DEBUG SWITCHES
	
	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/pgmspace.h>
	#include <avr/version.h>
	#include <util/delay.h>
	
	#include "Analogue.h"
	#include "GlobalMacros.h"
	#include "ISRMacro.h"
	#include "OSCCal.h"
	#include "LCD_Driver.h"
	#include "V2Protocol.h"
	#include "USART.h"
	#include "SPI.h"
	#include "USI.h"
	#include "Dataflash.h"
	#include "ProgramManager.h"
	#include "StorageManager.h"
	#include "EEPROMVariables.h"
	#include "TagManager.h"
	#include "ButtLoadTag.h"
	#include "ToneGeneration.h"
	
	// LIB C VERSION CHECK:
	#if (!defined(__AVR_LIBC_VERSION__) || (__AVR_LIBC_VERSION__ < 10401UL)) // In future AVRLibC version requirements may be increased with changes
		#error AVRLibC Version 1.4.1 or higher is required to compile this project.
	#endif
	
	// DEBUG MODE CHECKS:
	#if defined(INC_FROM_MAIN)
		#if defined(DEBUG_JTAGON)
			#warning DEBUG_JTAGON option is activated - JTAG system is enabled. Remove before releasing.
		#endif
	
		#if defined(DEBUG_DBFUNCSON)
			#warning DEBUG_DBFUNCSON option is activated - Debug routines are enabled. Remove before releasing.
		#endif
	#endif
	
	// EXTERNAL VARIABLES:
	extern const char WaitText[];
	extern const char VersionInfo[];
	extern const char USISpeedVals[];
	extern const char USISpeedIndex[];

	#define JoyStatus                GPIOR1 // Psudo-variable, GPIO register for speed

	extern EEPROMVarsType EEPROMVars EEMEM;
	
	// DEFINES AND MACROS:
	#define MAIN_SETSTATUSLED(mask)  MACROS{ PORTF = ((PORTF & ~MAIN_STATLED_ORANGE) | (mask)); }MACROE
	#define MAIN_STATLED_GREEN       (1 << 4)
	#define MAIN_STATLED_RED         (1 << 5)
	#define MAIN_STATLED_ORANGE      (MAIN_STATLED_GREEN | MAIN_STATLED_RED)
	#define MAIN_STATLED_OFF         0
	
	#define MAIN_RESETCS_ACTIVE      0
	#define MAIN_RESETCS_INACTIVE    1
	
	// PROTOTYPES:
	void MAIN_ResetCSLine(const uint8_t ActiveInactive);
	void MAIN_WaitForJoyRelease(void);
	void MAIN_IntToStr(uint16_t IntV, char* Buff);
	void MAIN_ShowProgType(const uint8_t Letter);
	void MAIN_ShowError(const char *pFlashStr);
	
	void MAIN_Delay10MS(uint8_t loops);
	void MAIN_Delay1MS(uint8_t loops);
	
	void MAIN_AVRISPMode(void);
	void MAIN_ProgramAVR(void);
	void MAIN_StoreProgram(void);
	void MAIN_StorageInfo(void);
	void MAIN_ChangeSettings(void);
	void MAIN_SleepMode(void);

	void MAIN_ShowAbout(void);

	void MAIN_ClearMem(void);
	void MAIN_SetContrast(void);
	void MAIN_SetISPSpeed(void);
	void MAIN_SetResetMode(void);
	void MAIN_SetFirmMinorVer(void);
	void MAIN_SetAutoSleepTimeOut(void);
	void MAIN_SetToneVol(void);
	void MAIN_SetStartupMode(void);
	void MAIN_GoBootloader(void) __attribute__((noreturn));

#endif
