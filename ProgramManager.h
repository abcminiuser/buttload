/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef PGMMGR_H
#define PGMMGR_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/eeprom.h>
	#include <avr/pgmspace.h>
	#include <stdlib.h>
	
	#include "Dataflash.h"
	#include "EEPROMVariables.h"
	#include "Main.h"
	
	// DEFINES:
	#define PM_OPT_CLEARFLAGS      (1 << 7)
	#define PM_OPT_FLASH           (1 << 0)
	#define PM_OPT_EEPROM          (1 << 1)
	#define PM_OPT_FUSE            (1 << 2)
	#define PM_OPT_LOCK            (1 << 3)
	
	#define PM_LARGE_PAGE_LENGTH   160
	
	// PROTOTYPES:
	void   PM_ShowStoredItemSizes(void);
	void   PM_StartProgAVR(void);
	void   PM_ChooseProgAVROpts(void);
	void   PM_SetProgramDataType(uint8_t Mask);

	#if defined(INC_FROM_PM)
	  static void   PM_SendFuseLockBytes(const uint8_t Type);
	  static void   PM_SendEraseCommand(void);
	  static void   PM_CreateProgrammingPackets(void);
	#endif
	
#endif
