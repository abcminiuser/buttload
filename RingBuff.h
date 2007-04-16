/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
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
	extern volatile uint8_t BuffElements; // Holds the number of elements in the buffer

	// PROTOTYPES:
	void     BUFF_InitializeBuffer(void);
	char     BUFF_GetBuffByte(void) ATTR_WARN_UNUSED_RESULT;
	
#endif
