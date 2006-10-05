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
	
	#include "ISRMacro.h"
	#include "GlobalMacros.h"
	
	// DEFINES:
	#define pLCDREG                   ((uint8_t*)&LCDDR0)
	
	#define LCD_CONTRAST_LEVEL(level) MACROS{ LCDCCR = (0x0F & level); }MACROE
	#define LCD_SCROLLCOUNT_DEFAULT   3
	#define LCD_DELAYCOUNT_DEFAULT    10
	#define LCD_TEXTBUFFER_SIZE       20
	#define LCD_SEGBUFFER_SIZE        19
	
	/*                        DIRTY HACK ALERT!!
	   This will display the "val" number of arrows running atop the Butterfly's
	   display. It will be cleared if the currently displayed text is scrolling
	   or changed.
	*/
	#define LCD_BARGRAPH(val)         MACROS{                      \
	                                  switch (val)                 \
                                      {                            \
										default:                   \
	                                 		LCDDR3 |= (1 << 0);    \
	                                 	case 4:                    \
	                                 		LCDDR1 |= (1 << 6);    \
	                                 	case 3:                    \
	                                 		LCDDR1 |= (1 << 2);    \
	                                 	case 2:                    \
	                                 		LCDDR0 |= (1 << 5);    \
	                                 	case 1:                    \
	                                 		LCDDR0 |= (1 << 1);    \
										case 0:                    \
											break;                 \
	                                  }                            \
									  }MACROE

	// PROTOTYPES:
	void LCD_puts(const char *Data);
	void LCD_puts_f(const char *FlashData);
	void LCD_Init(void);
	#if defined(INC_FROM_DRIVER)
		static inline void LCD_WriteChar(const uint8_t Byte, const char Digit) __attribute__((always_inline));
	#endif

#endif
