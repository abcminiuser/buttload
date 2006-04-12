/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef EEVARS_H
#define EEVARS_H

// INCLUDES:
#include <avr/io.h>
#include <avr/eeprom.h>

// TYPE DEFINITIONS:
typedef struct
{
	uint8_t  ResetPolarity;
	uint8_t  SCKDuration;
	uint8_t  WriteProgram[9];
	uint8_t  WriteEEPROM[9];
	uint8_t  EraseCmdStored;
	uint8_t  EraseChip[6];
	uint8_t  DataSize[4];
	uint8_t  EEPROMSize[4];
	uint8_t  EnterProgMode[12];
	uint8_t  TotalFuseBytes;
	uint8_t  TotalLockBytes;
	uint8_t  FuseBytes[40];
	uint8_t  LockBytes[40];
	uint16_t PageLength;
	uint16_t EPageLength;
	uint8_t  LCDContrast;
	uint8_t  MagicNumber;	
} EEPROMVarsType;

// EXTERN VARIABLES:
extern EEPROMVarsType EEPROMVars EEMEM;

#endif
