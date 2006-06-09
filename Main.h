/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef MAIN_H
#define MAIN_H
	// DEBUG SWITCH
	//#define DEBUG
	// END DEBUG SWITCH
	
	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/pgmspace.h>
	#include <avr/interrupt.h>
	#include <avr/version.h>
	#include <util/delay.h>
	#include <string.h>
	#include <inttypes.h>
	
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
	#if (!defined(__AVR_LIBC_VERSION__) || (__AVR_LIBC_VERSION__ < 10401UL)) // In future requirements may be increased with changes
		#error AVRLibC Version 1.4.1 or higher is required to compile this project.
	#endif
	
	// DEBUG MODE CHECK:
	#if defined(DEBUG)
		#warning DEBUG mode is activated - JTAG system is enabled. Remove before releasing.
	#endif
	
	// EXTERNAL VARIABLES:
	extern const uint8_t WaitText[];
	extern const uint8_t VersionInfo[];
	
	extern EEPROMVarsType EEPROMVars EEMEM;
	#define JoyStatus GPIOR0 // Pseudo-variable; "JoyStatus" becomes an alias for the General Purpose IO Storage Register 0
	
	// DEFINES AND MACROS:
	#define VERSION_MAJOR            1
	#define VERSION_MINOR            4
	#define VERSION_VSTRING          {'V','0' + VERSION_MAJOR,'-','0' + VERSION_MINOR, '\0'}
	
	#define MAGIC_NUM                0b01111010 // Magic number, used for first-run detection or upgrade incompatibility checks
	
	#define MAIN_SETSTATUSLED(mask)  MACROS{ PORTF = ((PORTF & ~MAIN_STATLED_ORANGE) | (mask)); }MACROE
	#define MAIN_STATLED_GREEN       (1 << 4)
	#define MAIN_STATLED_RED         (1 << 5)
	#define MAIN_STATLED_ORANGE      (MAIN_STATLED_GREEN | MAIN_STATLED_RED)
	#define MAIN_STATLED_OFF         0
	
	#define MAIN_RESETCS_ACTIVE      0
	#define MAIN_RESETCS_INACTIVE    1
	#define MAIN_RESETCS_EXTDFACTIVE 2
	
	#define JOY_LEFT                 (1 << 2)
	#define JOY_RIGHT                (1 << 3)
	#define JOY_UP                   (1 << 6)
	#define JOY_DOWN                 (1 << 7)
	#define JOY_PRESS                (1 << 4)
	#define JOY_BMASK                ((1 << 4) | (1 << 6) | (1 << 7))
	#define JOY_EMASK                ((1 << 2) | (1 << 3))
	
	#define TYPE_EEPROM              0
	#define TYPE_FLASH               1
	#define TYPE_FUSE                2
	#define TYPE_LOCK                3
	
	#define TRUE                     1
	#define FALSE                    0
	
	#define ARRAY_UPPERBOUND(array)  ((sizeof(array) / sizeof(array[0])) - 1)
	
	#define MACROS                   do
	#define MACROE                   while (0)
	
	#define SLEEP()                  MACROS{ asm volatile ("sleep"::); }MACROE
	
	// PROTOTYPES:
	void MAIN_ResetCSLine(const uint8_t ActiveInactive);
	void MAIN_WaitForJoyRelease(void);
	void MAIN_IntToStr(uint16_t IntV, uint8_t* Buff);
	void MAIN_ShowProgType(const uint8_t Letter);
	void MAIN_ShowError(const uint8_t *pFlashStr);
	
	void MAIN_Delay10MS(uint8_t loops);
	void MAIN_Delay1MS(uint8_t loops);
	void MAIN_CrashProgram(uint8_t *ErrTxtPtr) __attribute__((noreturn));
	
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
	void FUNCGoBootloader(void);
#endif
