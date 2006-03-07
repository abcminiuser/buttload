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
#define pLCDREG ((unsigned char *)(0xEC))

#define LCD_CONTRAST_LEVEL(level) LCDCCR = (0x0F & level)
#define LCD_SCROLLCOUNT_DEFAULT   3
#define LCD_DELAYCOUNT_DEFAULT    10
#define LCD_TEXTBUFFER_SIZE       20
#define LCD_SEGBUFFER_SIZE        20

// PROTOTYPES:
void LCD_puts(uint8_t *Data);
void LCD_puts_f(const uint8_t *FlashData);
void LCD_Init(void);
void LCD_WriteChar(uint8_t Byte, uint8_t Digit);

#endif
