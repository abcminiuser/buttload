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

/*****************************************************************************
*
*   Function name : USART_Init
*
*   Returns :       None
*
*   Parameters :    unsigned int baudrate
*
*   Purpose :       Initialize the USART
*
*****************************************************************************/
void USART_Init(void)
{
    // Set baud rate
    UBRRH = (uint8_t)(USART_BAUDVALUE >> 8);
    UBRRL = (uint8_t)(USART_BAUDVALUE);

    // Double speed
	UCSRA = (USART_DOUBLESPEED << U2X);

    // Enable recieve complete interrupt
	UCSRB = (1 << RXCIE);
	 
    // Async. mode, 8N1
    UCSRC = (3 << UCSZ0);
	 
	 // Initalise ringbuffer
	BUFF_InitialiseBuffer();
}

/*****************************************************************************
*
*   Function name : Usart_Tx
*
*   Returns :       None
*
*   Parameters :    char data: byte to send
*
*   Purpose :       Send one byte through the USART
*
*****************************************************************************/
void USART_Tx(char data)
{
    while (!(UCSRA & (1 << UDRE)));
    UDR = data;
}

/*****************************************************************************
*
*   Function name : Usart_Tx
*
*   Returns :       None
*
*   Parameters :    char data: byte to send
*
*   Purpose :       Send one byte through the USART
*
*****************************************************************************/
void USART_TxString(char *data)
{
	while (*data != '\0')
	  USART_Tx(*data++);
}

/*****************************************************************************
*
*   Function name : Usart_Rx
*
*   Returns :       char: byte received
*
*   Parameters :    None
*
*   Purpose :       Receives one byte from the USART
*
*****************************************************************************/
char USART_Rx(void)
{
    while (!(BuffElements) && !(TimeOut)) {};
    return BUFF_GetBuffByte();
}
