/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef PGMMGR_H
#define PGMMGR_H

// INCLUDES:
#include <avr/io.h>
#include <stdlib.h>

#include "Main.h"
#include "Dataflash.h"
#include "AVRISPCommandBytes.h"
#include "EEPROMVariables.h"

// DEFINES:
#define PM_NO_SETUP            0
#define PM_DATAFLASH_WRITE     1
#define PM_DATAFLASH_READ      2
#define PM_LOCKFUSEBITS_WRITE  3
#define PM_LOCKFUSEBITS_READ   4

#define PM_PAGELENGTH_FOUNDBIT (uint16_t)(1 << 15)

#define PM_MAX_FUSELOCKBITS    10

#define PM_EEPROM_OFFSET       (uint32_t)(1024UL * 257UL)

// EXTERNAL VARIABLES:
extern uint8_t  MemoryType;
extern uint8_t  CurrentMode;

// PROTOTYPES:
uint32_t PM_GetStoredDataSize(const uint8_t Type);
void     PM_SetupDFAddressCounters(const uint8_t Type);
void     PM_StoreProgramByte(const uint8_t Data);
void     PM_InterpretAVRISPPacket(void);
void     PM_CheckEndOfFuseLockData(void);
void     PM_SendFuseLockBytes(const uint8_t Type);
void     PM_SendEraseCommand(void);
void     PM_CreateProgrammingPackets(const uint8_t Type);
void     PM_ShowStoredItemSizes(void);

#endif
