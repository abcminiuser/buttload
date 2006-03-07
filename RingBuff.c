/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

/* Modified version of my RingBuffer library, avaliable from AVRFreaks.net */

// Includes:
#include <avr/io.h>
#include "RingBuff.h"

// Global Variables:
volatile BuffType       *StoreLoc;
volatile BuffType       *RetrieveLoc;
volatile BuffType       RingBuffer[BuffLen];
volatile ElemType       BuffElements;

// Routines:
void BUFF_InitialiseBuffer(void)
{
	StoreLoc    = (BuffType*)&RingBuffer[0]; // Set up the IN pointer to the start of the buffer
	RetrieveLoc = (BuffType*)&RingBuffer[0]; // Set up the OUT pointer to the start of the buffer

	BuffElements = 0;                     // Reset the buffer elements counter
}

void BUFF_StoreBuffByte(BuffType DataToStore)
{
	if (BuffElements == BuffLen)          // Buffer full
	{
		MAIN_ShowError(PSTR("BUFF OVERFLOW"));
		return;
	}
		
	*StoreLoc = DataToStore;              // Store the data

	StoreLoc++;                           // Increment the IN pointer to the next element
	BuffElements++;                       // Increment the total elements variable

	if (StoreLoc == (BuffType*)&RingBuffer[BuffLen])
		StoreLoc = (BuffType*)&RingBuffer[0]; // Wrap pointer if end of array reached
}	

BuffType BUFF_GetBuffByte(void)
{
	if (!(BuffElements))                  // No elements in the buffer
		return 0;

	BuffType RetrievedData = *RetrieveLoc; // Grab the stored byte into a temp variable

	RetrieveLoc++;                         // Increment the OUT pointer to the next element if flag set
	BuffElements--;                        // Decrement the total elements variable
	
	if (RetrieveLoc == (BuffType*)&RingBuffer[BuffLen])
		RetrieveLoc = (BuffType*)&RingBuffer[0]; // Wrap pointer if end of array reached
		
	return RetrievedData;                 // Return the retrieved data
}
