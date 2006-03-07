/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef DATAFLASH_H
#define DATAFLASH_H

// TYPE DEFINITIONS:
typedef unsigned char (*SPIFuncPtr)(unsigned char);

typedef struct
{
	unsigned char PageBits;
	unsigned int  PageSize;
	unsigned int  TotalPages;
} DFinfo;

// INCLUDES:
#include <avr/io.h>

#include "Main.h"
#include "DataflashCommandBytes.h"

// DEFINES AND MACROS:
#define DF_TOGGLEENABLE()        DF_EnableDataflash(FALSE); asm volatile ("nop"::); DF_EnableDataflash(TRUE)
#define DF_BUSY()                !(DF_GetChipCharacteristics() & DF_BUSYMASK)

#define DF_BUSYMASK              0x80
#define DF_INTERNALDF_BUFFBYTES  264

#define DF_SENDSPIBYTE(data)     (DFSPIRoutinePointer)(data)

#define PageShiftHigh            GPIOR1 // Psuedo variable; linked to the General Purpose Storage register 1 for speed
#define PageShiftLow             GPIOR2 // Psuedo variable; linked to the General Purpose Storage register 2 for speed

// GLOBAL VARIABLES:
extern SPIFuncPtr     DFSPIRoutinePointer;
extern DFinfo         DataflashInfo;
extern uint16_t       CurrPageAddress;
extern uint16_t       CurrBuffByte;
extern uint8_t        UseExernalDF;
extern const uint8_t DataFlashError[] PROGMEM;

// PROTOTYPES:
uint8_t DF_CheckCorrectOnboardChip(void);
uint8_t DF_GetChipCharacteristics(void);
void    DF_CopyBufferToFlashPage(uint16_t PageAddress);
void    DF_CopyFlashPageToBuffer(uint16_t PageAddress);
void    DF_ContinuousReadEnable(uint16_t PageAddress, uint16_t BuffAddress);
uint8_t DF_ReadBufferByte(uint16_t BuffAddress);
void    DF_BufferWriteEnable(uint16_t BuffAddress);
void    DF_ErasePage(uint16_t PageAddress);
void    DF_EnableDataflash(uint8_t Enabled);

#endif
