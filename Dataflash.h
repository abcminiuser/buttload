/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef DATAFLASH_H
#define DATAFLASH_H
	// INCLUDES:
	#include <avr/io.h>
	
	#include "Main.h"
	#include "DataflashCommandBytes.h"
	
	// TYPE DEFINITIONS:
	typedef unsigned char (*SPIFuncPtr)(unsigned char);
	
	typedef struct
	{
		uint16_t PageBits;
		uint16_t PageSize;
		uint16_t TotalPages;
		uint16_t CurrPageAddress;
		uint16_t CurrBuffByte;
		uint8_t  UseExernalDF;
	} DFinfo;
	
	// DEFINES AND MACROS:
	#define DF_TOGGLEENABLE()        MACROS{ DF_EnableDataflash(FALSE); DF_EnableDataflash(TRUE); }MACROE
	
	#define DF_BUSYMASK              0x80
	#define DF_INTERNALDF_BUFFBYTES  264
	
	#define DF_MAKELOCALSPIFUNCPTR() SPIFuncPtr LocalSPISendRoutinePtr = DFSPIRoutinePointer;
	#define DF_SENDSPIBYTE(data)     (LocalSPISendRoutinePtr)(data)
	
	#define PageShiftHigh            GPIOR1 // Psuedo variable; linked to the General Purpose Storage register 1 for speed
	#define PageShiftLow             GPIOR2 // Psuedo variable; linked to the General Purpose Storage register 2 for speed
	
	// GLOBAL VARIABLES:
	extern SPIFuncPtr     DFSPIRoutinePointer;
	extern DFinfo         DataflashInfo;
	extern const uint8_t DataFlashError[] PROGMEM;
	
	// PROTOTYPES:
	uint8_t DF_CheckCorrectOnboardChip(void);
	void    DF_GetChipCharacteristics(void);
	void    DF_WaitWhileBusy(void);
	void    DF_CopyBufferToFlashPage(const uint16_t PageAddress);
	void    DF_CopyFlashPageToBuffer(const uint16_t PageAddress);
	void    DF_ContinuousReadEnable(const uint16_t PageAddress, const uint16_t BuffAddress);
	uint8_t DF_ReadBufferByte(const uint16_t BuffAddress);
	void    DF_BufferWriteEnable(const uint16_t BuffAddress);
	void    DF_EraseBlock(const uint16_t BlockToErase);
	void    DF_EnableDataflash(const uint8_t Enabled);
#endif
