/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef RINGBUFF_H
	#define RINGBUFF_H

	// Includes:
	#include "Main.h"
	#include <avr/io.h>
   
	// Configuration:
	#define BuffLen  64
	typedef uint8_t BuffType; // Replace "uint8_t" with desired buffer storage type
	typedef uint8_t ElemType; // Replace "uint8_t" with the smallest datatype that can hold BuffLen

	// Extern Variables:
	extern volatile ElemType BuffElements; // Holds the number of elements in the buffer

	// Prototypes:
	void     BUFF_InitialiseBuffer(void);
	BuffType BUFF_GetBuffByte(void);
#endif
