/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
                  www.fourwalledcubicle.com
*/

#ifndef VAMM_H
#define VAMM

	// INCLUDES:
	#include <avr/pgmspace.h>

	#include "Main.h"
	#include "LCD_Driver.h"
	#include "Dataflash.h"
	#include "EEPROMVariables.h"
	#include "V2Protocol.h"

	// DEFINES:
	#define VAMM_PAGEERASED_DF_PAGE   (DF_DATAFLASH_PAGES - 1)
	
	#define VAMM_SETUP_NA             0
	#define VAMM_SETUP_WRITE          1
	#define VAMM_SETUP_READ           2
	#define VAMM_SETUP_ADDR_DONE      3
	
	#define VAMM_FLAG_CHECK           FALSE
	#define VAMM_FLAG_CLEAR           TRUE

	// EXTERNAL VARIABLES:
	extern uint8_t EraseFlagsTransReq;
	extern uint8_t PageErasedFlags[DF_DATAFLASH_BLOCKS];
	
	// PROTOTYPES:
	void    VAMM_EnterStorageMode(void);
	void    VAMM_ExitStorageMode(void);
	
	void    VAMM_EraseAVRMemory(void);
	void    VAMM_SetAddress(void);

	void    VAMM_StoreByte(const uint8_t Data);
	uint8_t VAMM_ReadByte(void) ATTR_WARN_UNUSED_RESULT;
	uint8_t VAMM_ReadConsec(void) ATTR_WARN_UNUSED_RESULT;

	#if defined(INC_FROM_VAMM) 
	  static void VAMM_CheckSetCurrPageCleared(const uint8_t ClearPageErasedFlag);
	  static void VAMM_Cleanup(void);
	#endif
	
#endif
