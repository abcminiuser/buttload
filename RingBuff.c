/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "RingBuff.h"

volatile uint8_t RingBuffer[BUFF_BUFFLEN] = {};
volatile uint8_t BuffElements             = 0;
volatile uint8_t InPos                    = 0;
volatile uint8_t OutPos                   = 0;

// ======================================================================================

ISR(USART0_RX_vect, ISR_BLOCK)
{
	RingBuffer[InPos] = UDR;                   // Store the data
	BuffElements++;                            // Increment the total elements variable

	InPos++;

	if (InPos == BUFF_BUFFLEN)                 // Wrap counter if end of array reached
	  InPos = 0;
}	

// ======================================================================================

void BUFF_InitialiseBuffer(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		InPos  = 0;                            // Set up the IN counter to the start of the buffer
		OutPos = 0;                            // Set up the OUT counter to the start of the buffer
		BuffElements = 0;                      // Reset the buffer elements counter
	}
	END_ATOMIC_BLOCK
}

uint8_t BUFF_GetBuffByte(void)
{
	uint8_t RetrievedData;

	ATOMIC_BLOCK(ATOMIC_ASSUMEON)
	{
		RetrievedData = RingBuffer[OutPos];    // Grab the stored byte into a temp variable
		BuffElements--;                        // Decrement the total elements variable
		
		OutPos++;
		
		if (OutPos == BUFF_BUFFLEN)            // Increment and wrap pointer if end of array reached
		  OutPos = 0;
	}
	END_ATOMIC_BLOCK
		
	return RetrievedData;                      // Return the retrieved data
}
