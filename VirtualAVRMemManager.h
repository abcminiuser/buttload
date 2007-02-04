/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef VAMM_H
#define VAMM

	// INCLUDES:
	#include <avr/pgmspace.h>

	#include "Main.h"
	#include "LCD_Driver.h"
	#include "Dataflash.h"
	#include "EEPROMVariables.h"

	// DEFINES:
	#define VAMM_SETUP_NA          0
	#define VAMM_SETUP_WRITE       1
	#define VAMM_SETUP_READ        2
	#define VAMM_SETUP_ADDR_DONE   3
	
	#define VAMM_FLAG_CHECK        FALSE
	#define VAMM_FLAG_CLEAR        TRUE

	// EXTERNAL VARIABLES:
	extern uint8_t EraseFlagsTransReq;
	
	// PROTOTYPES:
	void    VAMM_EnterStorageMode(void);
	void    VAMM_ExitStorageMode(void);
	
	void    VAMM_EraseAVRMemory(void);
	void    VAMM_SetAddress(void);

	void    VAMM_StoreByte(const uint8_t Data);
	uint8_t VAMM_ReadByte(void) ATTR_WARN_UNUSED_RESULT;

	#if defined(INC_FROM_VAMM) 
	  static uint8_t VAMM_CheckSetCurrPageCleared(const uint8_t ClearPageErasedFlag);
	  static void    VAMM_Cleanup(void);
	#endif
	
#endif
