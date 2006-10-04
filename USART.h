/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef USART_H
#define USART_H

	// INCLUDES:
	#include <avr/io.h>
	
	#include "LCD_Driver.h"
	#include "RingBuff.h"
	#include "Timeout.h"
	
	// MACROS AND DEFINES:	
	#define USART_BAUDRATE        115200UL
	#define USART_BAUDVALUE       (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

	#define USART_OFF()           MACROS{ UCSRB = 0x00; PRR |= (1 << PRUSART0); }MACROE
	
	// PROTOTYPES:
	void USART_Init(void);
	void USART_Tx(const char data);
	char USART_Rx(void);
	
#endif
