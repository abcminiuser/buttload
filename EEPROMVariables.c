/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

/*
 When I wrote this, I was using a pre-version 1.4.0 of AVRLibC. This was unable
 to access the EEPROM of the Mega169 and concequently I had to implement this
 manual address solution. Crude but it works; I plan to replace this in a future
 version.
*/

#include "EEPROMVariables.h"

//               EEPROM VARIABLE      ADDR     SIZE
const uint16_t Param_ResetPolarity = 0;    // 1
const uint16_t Param_SCKDuration   = 1;    // 1
const uint16_t Prog_WriteProgram   = 2;    // 9
const uint16_t Prog_WriteEEPROM    = 11;   // 9
const uint16_t Prog_EraseCmdStored = 20;   // 1
const uint16_t Prog_EraseChip      = 21;   // 6
const uint16_t Prog_DataSize       = 27;   // 4
const uint16_t Prog_EEPROMSize     = 31;   // 4
const uint16_t Prog_EnterProgMode  = 35;   // 12
const uint16_t Prog_TotalFuseBytes = 47;   // 1
const uint16_t Prog_TotalLockBytes = 48;   // 1
const uint16_t Prog_FuseBytes      = 49;   // 40
const uint16_t Prog_LockBytes      = 89;   // 40
const uint16_t Prog_PageLength     = 129;  // 2
const uint16_t Prog_EPageLength    = 131;  // 2
const uint16_t Sys_LCDContrast     = 132;  // 1
const uint16_t Sys_MagicNumber     = 511;  // 1  <- NB: MUST be the last variable in EEPROM
