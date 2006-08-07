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

#include "USART.h"

/*
	Edit for ButtLoad: I've changed the recieve method so when a character is
	recieved it is loaded into a FIFO (ring) buffer immediatly via an interrupt.
	This ensures that no characters are lost while the processor is busy with
	other tasks such as updating the LCD.
*/

void USART_Init(void)
{
	// Calibrate the internal RC oscilator
	LCD_puts_f(WaitText);

	#ifndef DEBUG
	  OSCCAL_Calibrate();
	#endif

	// Enable USART subsystem
	PRR &= ~(1 << PRUSART0);
	
    // Set baud rate
    UBRRH = (uint8_t)(USART_BAUDVALUE >> 8);
    UBRRL = (uint8_t)(USART_BAUDVALUE);

    // Enable Rx/Tx subsections and recieve complete interrupt
	UCSRB = ((1<<TXEN) | (1<<RXEN) | (1 << RXCIE));
	 
    // Async. mode, 8N1
    UCSRC = (3 << UCSZ0);
	 
	// Initalise ringbuffer
	BUFF_InitialiseBuffer();
}

void USART_Tx(const char data)
{
    while (!(UCSRA & (1 << UDRE)));
    UDR = data;
}

char USART_Rx(void)
{
    while (!(BuffElements) && !(PacketTimeOut)) {};
    return BUFF_GetBuffByte();
}
