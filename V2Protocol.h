/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef V2PROTOCOL_H
#define V2PROTOCOL_H

#define HW_VERSION                2 /* Replace these values with the most current */
#define SW_VERSION_MAJOR          2 /* numbers to fool AVRStudio into believing   */
#define SW_VERSION_MINOR          4 /* the firmware is the latest version         */

// INCLUDES:
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "Main.h"
#include "lcd_driver.h"
#include "Timeout.h"
#include "USART.h"
#include "eeprom169.h"
#include "EEPROMVariables.h"
#include "AVRISPCommandBytes.h"
#include "AVRISPCommandInterpreter.h"

// DEFINES AND MACROS:
#define V2P_MAXBUFFSIZE           275 // Maximum message size length (275 is the same as the STK500)

#define V2P_STATE_IDLE              0
#define V2P_STATE_START             1
#define V2P_STATE_GETSEQUENCENUM    2
#define V2P_STATE_GETMESSAGESIZE1   3
#define V2P_STATE_GETMESSAGESIZE2   4
#define V2P_STATE_GETTOKEN          5
#define V2P_STATE_GETDATA           6
#define V2P_STATE_GETCHECKSUM       7
#define V2P_STATE_PACKOK            8
#define V2P_STATE_PACKERR           9

#define V2P_LOAD_EXTENDED_ADDR_CMD  0x4D

// EXTERNAL VARIABLES:
extern FuncPtr   InterpretPacketRoutine;
extern uint8_t   PacketBytes[V2P_MAXBUFFSIZE];
extern uint16_t  MessageSize;
extern uint8_t   InProgrammingMode;
extern uint32_t  CurrAddress;

// PROTOTYPES:
void    V2P_RunStateMachine(void);
void    V2P_InterpretPacket(void);
void    V2P_SendPacket(void);
uint8_t V2P_GetChecksum(void);
void    V2P_GetSetParamater(void);
void    V2P_IncrementCurrAddress(void);
void    V2P_CheckForExtendedAddress(void);

#endif
