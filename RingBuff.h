/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef RINGBUFF_H
#define RINGBUFF_H
	// INCLUDES:
	#include "Main.h"
   
	// DEFINES:
	#define BUFF_BUFFLEN  64

	// EXTERN VARIABLES:
	extern volatile uint8_t BuffElements; // Holds the number of elements in the buffer

	// PROTOTYPES:
	void     BUFF_InitialiseBuffer(void);
	uint8_t  BUFF_GetBuffByte(void);
#endif
