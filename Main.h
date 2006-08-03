/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef MAIN_H
#define MAIN_H

	// DEBUG SWITCH
//	#define DEBUG
	// END DEBUG SWITCH
	
	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/pgmspace.h>
	#include <avr/version.h>
	#include <util/delay.h>
	
	#include "GlobalMacros.h"
	#include "ISRMacro.h"
	#include "OSCCal.h"
	#include "LCD_Driver.h"
	#include "V2Protocol.h"
	#include "USART.h"
	#include "SPI.h"
	#include "USI.h"
	#include "Dataflash.h"
	#include "ProgDataflash.h"
	#include "ProgramManager.h"
	#include "RingBuff.h"
	#include "EEPROMVariables.h"
	#include "TagManager.h"
	#include "ButtLoadTag.h"
	
	// LIB C VERSION CHECK:
	#if (!defined(__AVR_LIBC_VERSION__) || (__AVR_LIBC_VERSION__ < 10401UL)) // In future AVRLibC version requirements may be increased with changes
		#error AVRLibC Version 1.4.1 or higher is required to compile this project.
	#endif
	
	// DEBUG MODE CHECK:
	#if defined(DEBUG) && defined(INC_FROM_MAIN)
		#warning DEBUG mode is activated - JTAG system is enabled. Remove before releasing.
	#endif
	
	// EXTERNAL VARIABLES:
	extern const uint8_t WaitText[];
	extern const uint8_t VersionInfo[];
	
	extern volatile uint8_t JoyStatus;

	extern EEPROMVarsType EEPROMVars EEMEM;
	
	// DEFINES AND MACROS:
	#define MAIN_SETSTATUSLED(mask)  MACROS{ PORTF = ((PORTF & ~MAIN_STATLED_ORANGE) | (mask)); }MACROE
	#define MAIN_STATLED_GREEN       (1 << 4)
	#define MAIN_STATLED_RED         (1 << 5)
	#define MAIN_STATLED_ORANGE      (MAIN_STATLED_GREEN | MAIN_STATLED_RED)
	#define MAIN_STATLED_OFF         0
	
	#define MAIN_RESETCS_ACTIVE      0
	#define MAIN_RESETCS_INACTIVE    1
	#define MAIN_RESETCS_EXTDFACTIVE 2
	
	// PROTOTYPES:
	void MAIN_ResetCSLine(const uint8_t ActiveInactive);
	void MAIN_WaitForJoyRelease(void);
	void MAIN_IntToStr(uint16_t IntV, uint8_t* Buff);
	void MAIN_ShowProgType(const uint8_t Letter);
	void MAIN_ShowError(const uint8_t *pFlashStr);
	
	void MAIN_Delay10MS(uint8_t loops);
	void MAIN_Delay1MS(uint8_t loops);
	
	void FUNCChangeSettings(void);
	void FUNCShowAbout(void);
	void FUNCAVRISPMode(void);
	void FUNCProgramDataflash(void);
	void FUNCProgramAVR(void);
	void FUNCStoreProgram(void);
	void FUNCClearMem(void);
	void FUNCSetContrast(void);
	void FUNCSetISPSpeed(void);
	void FUNCSetResetMode(void);
	void FUNCSetFirmMinorVer(void);
	void FUNCSetAutoSleepTimeOut(void);
	void FUNCSleepMode(void);
	void FUNCStorageInfo(void);
	void FUNCGoBootloader(void) __attribute__((noreturn));
	
	#ifdef DEBUG
	  void DFUNCManCalib(void);
	#endif
#endif
