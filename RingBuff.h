/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
                  dean_camera@hotmail.com
            http://home.pacific.net.au/~sthelena/
*/

#ifndef RINGBUFF_H
#define RINGBUFF_H

	// INCLUDES:
	#include <avr/io.h>
	#include <avr/interrupt.h>
	
	#include "ISRMacro.h"
	#include "GlobalMacros.h"
	#include "Atomic.h"
	
	// DEFINES:
	#define BUFF_BUFFLEN  64

	// EXTERN VARIABLES:
	extern volatile uint8_t BuffElements; // Holds the number of elements in the buffer

	// PROTOTYPES:
	void     BUFF_InitializeBuffer(void);
	uint8_t  BUFF_GetBuffByte(void) ATTR_WARN_UNUSED_RESULT;
	
#endif
