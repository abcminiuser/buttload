/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef DATAFLASH_H
#define DATAFLASH_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/pgmspace.h>
	
	#include "SPI.h"
	#include "DataflashCommandBytes.h"
	
	// TYPE DEFINITIONS:
	typedef unsigned char (*SPIFuncPtr)(unsigned char);
	
	typedef struct
	{
		uint16_t   CurrPageAddress;
		uint16_t   CurrBuffByte;
	} DFinfo;
	
	// DEFINES AND MACROS:
	#define DF_TOGGLEENABLE()        MACROS{ DF_EnableDataflash(FALSE); DF_EnableDataflash(TRUE); }MACROE
	
	#define DF_BUSYMASK              0x80
	#define DF_DFINFOMASK            0x38
	#define DF_INTERNALDF_BUFFBYTES  264
		
	#define DF_PAGESHIFT_HIGH        (16 - 9)
	#define DF_PAGESHIFT_LOW         ( 9 - 8)
	
	// GLOBAL VARIABLES:
	extern DFinfo         DataflashInfo;
	
	// PROTOTYPES:
	uint8_t DF_CheckCorrectOnboardChip(void);
	void    DF_WaitWhileBusy(void);
	void    DF_CopyBufferToFlashPage(const uint16_t PageAddress);
	void    DF_CopyFlashPageToBuffer(const uint16_t PageAddress);
	void    DF_ContinuousReadEnable(const uint16_t PageAddress, const uint16_t BuffAddress);
	uint8_t DF_ReadBufferByte(const uint16_t BuffAddress);
	void    DF_BufferWriteEnable(const uint16_t BuffAddress);
	void    DF_EnableDataflash(const uint8_t Enabled);
	
#endif
