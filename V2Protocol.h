/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef V2PROTOCOL_H
#define V2PROTOCOL_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/pgmspace.h>
	#include <util/delay.h>
	
	#include "AVRISPCommandBytes.h"
	#include "AVRISPCommandInterpreter.h"
	#include "LCD_Driver.h"
	#include "Timeout.h"
	#include "USART.h"
	#include "EEPROMVariables.h"
	#include "ProgramManager.h"

	// TYPE DEFINITIONS:
	typedef void (*FuncPtr)(void);
	
	// DEFINES AND MACROS:
	#define V2P_MAXBUFFSIZE              275 // Maximum message size length (275 is the same as the STK500)
	
	#define V2P_STATE_IDLE               0
	#define V2P_STATE_START              1
	#define V2P_STATE_GETSEQUENCENUM     2
	#define V2P_STATE_GETMESSAGESIZE1    3
	#define V2P_STATE_GETMESSAGESIZE2    4
	#define V2P_STATE_GETTOKEN           5
	#define V2P_STATE_GETDATA            6
	#define V2P_STATE_GETCHECKSUM        7
	#define V2P_STATE_BADCHKSUM          8
	#define V2P_STATE_TIMEOUT            9
	#define V2P_STATE_PACKERR            10
	#define V2P_STATE_PACKOK             11
	
	#define V2P_LOAD_EXTENDED_ADDR_FLAG  (1UL << 31)
	#define V2P_LOAD_EXTENDED_ADDR_CMD   0x4D
	#define V2P_LOAD_EXTENDED_ADDR_MASK  0x00FF0000UL
	#define V2P_LOAD_EXTENDED_ADDR_SHIFT 16
	
	#define V2P_HW_VERSION               2
	#define V2P_SW_VERSION_MAJOR         2
	#define V2P_SW_VERSION_MINOR_DEFAULT 10
	
	// EXTERNAL VARIABLES:
	extern uint8_t   PacketBytes[V2P_MAXBUFFSIZE];
	extern uint16_t  MessageSize;
	extern uint8_t   InProgrammingMode;
	extern uint32_t  CurrAddress;
	
	// PROTOTYPES:
	void    V2P_RunStateMachine(FuncPtr PacketDecodeFunction);
	void    V2P_SendPacket(void);
	void    V2P_IncrementCurrAddress(void);
	void    V2P_CheckForExtendedAddress(void);

	#if defined(INC_FROM_V2P)
	  static uint8_t V2P_GetChecksum(void) ATTR_WARN_UNUSED_RESULT;
	  static void    V2P_GetSetParameter(void);
	#endif
	
#endif
