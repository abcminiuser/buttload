/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
                  www.fourwalledcubicle.com
*/

#include "USART.h"

// ======================================================================================

/*
 NAME:      | USART_Init
 PURPOSE:   | Initializes the USART, ready for data transmission and reception
 ARGUMENTS: | None
 RETURNS:   | None
*/
void USART_Init(void)
{
	// Calibrate the internal RC oscilator
	LCD_puts_f(WaitText);
	OSCCAL_Calibrate();

	// Enable USART subsystem
	PRR &= ~(1 << PRUSART0);
	
    // Set baud rate
#if (USART_BAUDVALUE >> 8)
    UBRRH = (uint8_t)(USART_BAUDVALUE >> 8);
#endif
    UBRRL = (uint8_t)(USART_BAUDVALUE);

	// Reset UCSRA to clear any pre-set bits
	UCSRA = 0;

    // Enable Rx/Tx subsections and recieve complete interrupt
	UCSRB = ((1 << TXEN) | (1 << RXEN) | (1 << RXCIE));
	 
    // Async. mode, 8N1
    UCSRC = (3 << UCSZ0);
	 
	// Initalise ringbuffer
	BUFF_InitializeBuffer();
}

/*
 NAME:      | USART_Tx
 PURPOSE:   | Transmits a byte via the USART, waiting until USART is ready if needed
 ARGUMENTS: | Byte to transmit
 RETURNS:   | None
*/
void USART_Tx(const char data)
{
	while (!(UCSRA & (1 << UDRE)));
	UDR = data;
}

/*
 NAME:      | USART_Rx
 PURPOSE:   | Returns the next byte out of the ring buffer, looping until data ready or timeout expires
 ARGUMENTS: | None
 RETURNS:   | Next character in ringbuffer, or 0 if timeout expires
*/
char USART_Rx(void)
{
	while (!(BuffElements) && !(PacketTimeOut))
	  SLEEPCPU(SLEEP_IDLE);      // Idle: USART interrupt on reception of data, or timeout timer ISR will resume execution

	return BUFF_GetBuffByte();
}
