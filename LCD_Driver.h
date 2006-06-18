/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef LCDDRIVER_H
#define LCDDRIVER_H
	// INCLUDES:
	#include <avr/io.h>
	#include <avr/pgmspace.h>
	#include <avr/interrupt.h>
	
	#include "Main.h"
	
	// DEFINES:
	#define pLCDREG                   ((uint8_t*)&LCDDR0)
	
	#define LCD_CONTRAST_LEVEL(level) MACROS{ LCDCCR = (0x0F & level); }MACROE
	#define LCD_SCROLLCOUNT_DEFAULT   3
	#define LCD_DELAYCOUNT_DEFAULT    10
	#define LCD_TEXTBUFFER_SIZE       20
	#define LCD_SEGBUFFER_SIZE        20
	
	// PROTOTYPES:
	void LCD_puts(const uint8_t *Data);
	void LCD_puts_f(const uint8_t *FlashData);
	void LCD_Init(void);
	#ifdef INC_FROM_DRIVER
		static inline void LCD_WriteChar(const uint8_t Byte, const uint8_t Digit) __attribute__((always_inline));
	#endif

#endif
