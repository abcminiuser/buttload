/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
                  www.fourwalledcubicle.com
*/

#include "RingBuff.h"

volatile uint8_t RingBuffer[BUFF_BUFFLEN] = {};
volatile uint8_t BuffElements             = 0;
volatile uint8_t InPos                    = 0;
volatile uint8_t OutPos                   = 0;

// ======================================================================================

/*
 NAME:      | USART0_RX_vect (ISR, blocking)
 PURPOSE:   | ISR to handle the reception of serial data, placing recieved bytes into a ring buffer
 ARGUMENTS: | None
 RETURNS:   | None
*/
ISR(USART0_RX_vect, ISR_BLOCK)
{
	RingBuffer[InPos] = UDR;                   // Store the data
	BuffElements++;                            // Increment the total elements variable

	InPos++;

	if (InPos == BUFF_BUFFLEN)                 // Wrap counter if end of array reached
	  InPos = 0;
}	

// ======================================================================================

/*
 NAME:      | BUFF_InitializeBuffer
 PURPOSE:   | Resets and initializes the ring buffer ready for byte storage
 ARGUMENTS: | None
 RETURNS:   | None
*/
void BUFF_InitializeBuffer(void)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		InPos  = 0;                            // Set up the IN counter to the start of the buffer
		OutPos = 0;                            // Set up the OUT counter to the start of the buffer
		BuffElements = 0;                      // Reset the buffer elements counter
	}
}

/*
 NAME:      | BUFF_GetBuffByte
 PURPOSE:   | Returns the next byte in the FIFO ring buffer
 ARGUMENTS: | None
 RETURNS:   | Next bytes in the ring buffer
*/
uint8_t BUFF_GetBuffByte(void)
{
	uint8_t RetrievedData;

	if (!(BuffElements))                       // Return 0 if nothing in the buffer
	  return 0;

	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		RetrievedData = RingBuffer[OutPos];    // Grab the stored byte into a temp variable
		BuffElements--;                        // Decrement the total elements variable
		
		OutPos++;
		
		if (OutPos == BUFF_BUFFLEN)            // Increment and wrap pointer if end of array reached
		  OutPos = 0;
	}
		
	return RetrievedData;                      // Return the retrieved data
}
