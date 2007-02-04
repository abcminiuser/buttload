/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef MAIN_H
#define MAIN_H

	// DEBUG SWITCHES
	#define DEBUG_JTAGON
	#define DEBUG_MEMFILLON
	#define DEBUG_BYTEORDERTEST
	// END DEBUG SWITCHES
	
	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/pgmspace.h>
	#include <avr/version.h>
	#include <Delay.h>
	
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
	#include "TagManager.h"
	#include "Support/ButtLoadTag.h"
	#include "ToneGeneration.h"
	#include "VirtualAVRMemManager.h"
	#include "EEPROMVariables.h"
	
	#if defined(INC_FROM_MAIN)
		// LIB C VERSION CHECK:
		#if (!defined(__AVR_LIBC_VERSION__) || (__AVR_LIBC_VERSION__ < 10401UL)) // In future AVRLibC version requirements may be increased with changes
			#error AVRLibC Version 1.4.1 or higher is required to compile this project.
		#endif
	
		// DEBUG MODE CHECKS:
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

	#define JoyStatus                  GPIOR1 // Psudo-variable, GPIO register for speed

	extern EEPROMVarsType EEPROMVars   EEMEM;

	extern const uint8_t BitTable[]    PROGMEM;

	// DEFINES AND MACROS:
	#define MAIN_STATUSLED_PORT        PORTF
	#define MAIN_STATUSLED_PIN         PINF
	
	#define MAIN_SETSTATUSLED(mask)    MACROS{ MAIN_STATUSLED_PORT = ((MAIN_STATUSLED_PORT & ~MAIN_STATLED_ORANGE) | (mask)); }MACROE
	#define MAIN_TOGGLESTATUSLED(mask) MACROS{ MAIN_STATUSLED_PIN |= mask; }MACROE

	#define MAIN_STATLED_GREEN         (1 << 4)
	#define MAIN_STATLED_RED           (1 << 5)
	#define MAIN_STATLED_ORANGE        (MAIN_STATLED_GREEN | MAIN_STATLED_RED)
	#define MAIN_STATLED_OFF           0
	
	#define MAIN_RESET_ACTIVE          0
	#define MAIN_RESET_INACTIVE        1
		
	// PROTOTYPES:
	void MAIN_SetTargetResetLine(const uint8_t ActiveInactive);
	void MAIN_WaitForJoyRelease(void);
	void MAIN_IntToStr(uint16_t IntV, char *Buff);
	void MAIN_ShowProgType(const uint8_t Letter);
	void MAIN_ShowError(const char *pFlashStr);
	
	void MAIN_Delay10MS(uint8_t loops);
	void MAIN_Delay1MS(uint8_t loops);
	
	void MAIN_SleepMode(void);

	#if defined(INC_FROM_MAIN)
	  static void MAIN_ShowAbout(void);

	  static void MAIN_AVRISPMode(void);
	  static void MAIN_ProgramAVR(void);
	  static void MAIN_StoreProgram(void);
	  static void MAIN_ChangeSettings(void);

	  static void MAIN_ClearMem(void);
	  static void MAIN_SetContrast(void);
	  static void MAIN_SetISPSpeed(void);
	  static void MAIN_SetResetMode(void);
	  static void MAIN_SetFirmMinorVer(void);
	  static void MAIN_SetAutoSleepTimeOut(void);
	  static void MAIN_SetToneVol(void);
	  static void MAIN_SetStartupMode(void);
	  static void MAIN_StorageInfo(void);
	  static void MAIN_GoBootloader(void) ATTR_NO_RETURN;
	#endif

	void MAIN_Util_RAMFill(void) ATTR_INIT_SECTION(0);
	void MAIN_Util_ByteOrderTest(void) ATTR_INIT_SECTION(3);
#endif
