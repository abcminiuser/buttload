#ifndef USART_H
#define USART_H
	//***************************************************************************
	//
	//  File........: usart.c
	//
	//  Author(s)...: ATMEL Norway
	//
	//  Target(s)...: ATmega169
	//
	//  Compiler....: IAR EWAAVR 2.28a
	//
	//  Description.: AVR Butterfly USART routines
	//
	//  Revisions...: 1.0
	//
	//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
	//
	//  20030116 - 1.0  - Created                                       - LHM
	//
	//***************************************************************************
	
	// INCLUDES:
	#include <avr/io.h>
	
	#include "LCD_Driver.h"
	#include "RingBuff.h"
	#include "Timeout.h"
	
	// MACROS AND DEFINES:	
	#define USART_BAUDRATE        115200
	#define USART_BAUDVALUE       (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

	#define USART_OFF()           MACROS{ UCSRB = 0x00; }MACROE
	
	// PROTOTYPES:
	void USART_Init(void);
	void USART_Tx(const char data);
	char USART_Rx(void);
#endif
