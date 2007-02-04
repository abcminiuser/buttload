/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
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
	#define DF_TOGGLEENABLE()        MACROS{ DF_ENABLEDATAFLASH(FALSE); DF_ENABLEDATAFLASH(TRUE); }MACROE
	#define DF_ENABLEDATAFLASH(x)    MACROS{ if (x == TRUE) { PORTB &= ~(1 << 0); } else { PORTB |= (1 << 0); } }MACROE
	
	#define DF_BUSYMASK              0x80
	#define DF_DFINFOMASK            0x38
	#define DF_INTERNALDF_BUFFBYTES  264

	#define DF_PAGESHIFT_HIGH        (16 - 9)
	#define DF_PAGESHIFT_LOW         ( 9 - 8)
	
	#define DF_BUFFER_TO_FLASH       DFCB_BUF1TOFLASHWE
	#define DF_FLASH_TO_BUFFER       DFCB_FLASHTOBUF1TRANSFER
	
	// GLOBAL VARIABLES:
	extern DFinfo                    DataflashInfo;
	
	// PROTOTYPES:
	uint8_t DF_CheckCorrectOnboardChip(void) ATTR_WARN_UNUSED_RESULT;
	void    DF_CopyPage(const uint16_t PageAddress, uint8_t Operation);
	void    DF_ContinuousReadEnable(const uint16_t PageAddress, const uint16_t BuffAddress);
	void    DF_BufferWriteEnable(const uint16_t BuffAddress);

	#if defined(INC_FROM_DF)
	  void    DF_WaitWhileBusy(void);
	#endif
	
#endif
