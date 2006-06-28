/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

// Includes:
#include "RingBuff.h"

// Global Variables:
volatile uint8_t RingBuffer[BUFF_BUFFLEN] = {};
volatile uint8_t BuffElements             = 0;
volatile uint8_t InPos                    = 0;
volatile uint8_t OutPos                   = 0;

// ======================================================================================

ISR(USART0_RX_vect, ISR_BLOCK)
{
	RingBuffer[InPos++] = UDR;             // Store the data, increment the buffer in position
	BuffElements++;                        // Increment the total elements variable

	if (InPos == BUFF_BUFFLEN)
	  InPos = 0;                           // Wrap counter if end of array reached
}	

// ======================================================================================

void BUFF_InitialiseBuffer(void)
{
	InPos  = 0;                            // Set up the IN counter to the start of the buffer
	OutPos = 0;                            // Set up the OUT counter to the start of the buffer

	BuffElements = 0;                      // Reset the buffer elements counter
}

uint8_t BUFF_GetBuffByte(void)
{
	if (!(BuffElements))                   // No elements in the buffer
	  return 0;

	uint8_t RetrievedData = RingBuffer[OutPos++]; // Grab the stored byte into a temp variable
	BuffElements--;                        // Decrement the total elements variable
	
	if (OutPos == BUFF_BUFFLEN)
	  OutPos = 0;                          // Wrap pointer if end of array reached
		
	return RetrievedData;                 // Return the retrieved data
}
