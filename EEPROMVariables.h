/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

// INCLUDES:
#include <avr/io.h>

#include "EEPROM169.h"

// EXTERN VARIABLES:
extern const uint16_t Param_ResetPolarity;
extern const uint16_t Param_SCKDuration;
extern const uint16_t Prog_WriteProgram;
extern const uint16_t Prog_WriteEEPROM;
extern const uint16_t Prog_EraseCmdStored;
extern const uint16_t Prog_EraseChip;
extern const uint16_t Prog_DataSize;
extern const uint16_t Prog_EEPROMSize;
extern const uint16_t Prog_EnterProgMode;
extern const uint16_t Prog_TotalFuseBytes;
extern const uint16_t Prog_TotalLockBytes;
extern const uint16_t Prog_FuseBytes;
extern const uint16_t Prog_LockBytes;
extern const uint16_t Prog_PageLength;
extern const uint16_t Prog_EPageLength;
extern const uint16_t Sys_LCDContrast;
extern const uint16_t Sys_MagicNumber;
