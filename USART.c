/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "USART.h"

// ======================================================================================

void USART_Init(void)
{
	// Calibrate the internal RC oscilator
	LCD_puts_f(WaitText);
	OSCCAL_Calibrate();

	// Enable USART subsystem
	PRR &= ~(1 << PRUSART0);
	
    // Set baud rate
    UBRRH = (uint8_t)(USART_BAUDVALUE >> 8);
    UBRRL = (uint8_t)(USART_BAUDVALUE);

	// Reset UCSRA to clear any pre-set bits
	UCSRA = 0;

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
	uint8_t AtomicBuffElements;
	
	do
	{
		cli();
		AtomicBuffElements = BuffElements;
		sei();
	}
	while (!(AtomicBuffElements) && !(PacketTimeOut));

	return BUFF_GetBuffByte();
}
