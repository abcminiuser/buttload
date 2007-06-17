/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
             dean [at] fourwalledcubicle [dot] com
                  www.fourwalledcubicle.com
*/

#ifndef RINGBUFF_H
#define RINGBUFF_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	
	#include "Atomic.h"
	#include "GlobalMacros.h"
	#include "ISRMacro.h"
	
	// DEFINES:
	#define BUFF_BUFFLEN  64

	// EXTERN VARIABLES:
	extern volatile char    RingBuffer[BUFF_BUFFLEN];
	extern volatile uint8_t InPos;
	extern volatile uint8_t OutPos;
	extern volatile uint8_t BuffElements;
	
	// PROTOTYPES:
	void BUFF_InitializeBuffer(void);
	
	// INLINE FUNCTIONS:
	static inline char BUFF_GetBuffByte(void) ATTR_ALWAYS_INLINE ATTR_WARN_UNUSED_RESULT;
	static inline char BUFF_GetBuffByte(void)
	{
		uint8_t RetrievedData;

		if (!(BuffElements))
		  return 0;

		ATOMIC_BLOCK(ATOMIC_FORCEON)
		{
			RetrievedData = RingBuffer[OutPos];
			BuffElements--;
			
			OutPos++;
			
			if (OutPos == BUFF_BUFFLEN)
			  OutPos = 0;
		}
			
		return RetrievedData;
	}
	
#endif
