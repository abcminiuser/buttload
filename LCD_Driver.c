/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

/*
	This is a basic driver for the Butterfly LCD. It offers the ability to
	change the contrast and display strings (scrolling or static) from flash
	or SRAM memory only.
	
	This has been completly rewritten from the Atmel code; in this version, as
	much processing as possible is performed by the string display routines
	rather than the interrupt so that the interrupt executes as fast as possible.
*/

#include "LCD_Driver.h"

uint8_t  TextBuffer[LCD_TEXTBUFFER_SIZE + 7];
uint8_t  SegBuffer[LCD_SEGBUFFER_SIZE];
uint8_t  StrStart;
uint8_t  StrEnd;
uint8_t  ScrollMode;
uint8_t  ScrollCount;
uint8_t  DelayCount;
uint8_t  UpdateLCD;

uint16_t LCD_SegTable[] PROGMEM =
{
    0xEAA8,     // '*'
    0x2A80,     // '+'
    0x4000,     // ','
    0x0A00,     // '-'
    0x0A51,     // '.' Degree sign
    0x4008,     // '/'
    0x5559,     // '0'
    0x0118,     // '1'
    0x1e11,     // '2
    0x1b11,     // '3
    0x0b50,     // '4
    0x1b41,     // '5
    0x1f41,     // '6
    0x0111,     // '7
    0x1f51,     // '8
    0x1b51,     // '9'
    0x0000,     // ':' (Not defined)
    0x0000,     // ';' (Not defined)
    0x8008,     // '<'
    0x1A00,     // '='
    0x4020,     // '>'
    0x0000,     // '?' (Not defined)
    0x0000,     // '@' (Not defined)
    0x0f51,     // 'A' (+ 'a')
    0x3991,     // 'B' (+ 'b')
    0x1441,     // 'C' (+ 'c')
    0x3191,     // 'D' (+ 'd')
    0x1e41,     // 'E' (+ 'e')
    0x0e41,     // 'F' (+ 'f')
    0x1d41,     // 'G' (+ 'g')
    0x0f50,     // 'H' (+ 'h')
    0x2080,     // 'I' (+ 'i')
    0x1510,     // 'J' (+ 'j')
    0x8648,     // 'K' (+ 'k')
    0x1440,     // 'L' (+ 'l')
    0x0578,     // 'M' (+ 'm')
    0x8570,     // 'N' (+ 'n')
    0x1551,     // 'O' (+ 'o')
    0x0e51,     // 'P' (+ 'p')
    0x9551,     // 'Q' (+ 'q')
    0x8e51,     // 'R' (+ 'r')
    0x9021,     // 'S' (+ 's')
    0x2081,     // 'T' (+ 't')
    0x1550,     // 'U' (+ 'u')
    0x4448,     // 'V' (+ 'v')
    0xc550,     // 'W' (+ 'w')
    0xc028,     // 'X' (+ 'x')
    0x2028,     // 'Y' (+ 'y')
    0x5009,     // 'Z' (+ 'z')
    0x1441,     // '['
    0x8020,     // '\'
    0x1111,     // ']'
    0x0000,     // '^' (Not defined)
    0x1000      // '_'
};

// ======================================================================================

void LCD_Init(void)
{
	LCDCCR = 0x0F;

    // Select asynchronous clock source, enable all COM pins and enable all segment pins.
    LCDCRB  = (1<<LCDCS) | (3<<LCDMUX0) | (7<<LCDPM0);

    // Set LCD prescaler to give a framerate of 32,0 Hz
    LCDFRR  = (0<<LCDPS0) | (7<<LCDCD0);    

	// Enable LCD and set low power waveform
    LCDCRA  = (1<<LCDEN) | (1<<LCDAB); 

    //Enable LCD start of frame interrupt
    LCDCRA |= (1<<LCDIE);
}

void LCD_puts_f(const uint8_t *FlashData)
{
	uint8_t StrBuff[LCD_TEXTBUFFER_SIZE];

	strcpy_P(StrBuff, FlashData);
	LCD_puts(StrBuff);
}

void LCD_puts(uint8_t *Data)
{
	uint8_t LoadB;
	
	for (LoadB = 0; LoadB < 20; LoadB++)
	{
		uint8_t CByte = *(Data++);
	
		if ((CByte >= '*') && (CByte <= 'z'))
		  TextBuffer[LoadB] = ((CByte == ' ')? 0xFF : (CByte - '*'));
		else if (CByte == 0x00)
		  break;
		else
		  TextBuffer[LoadB] = 0xFF;		
	}

	ScrollMode  = ((LoadB > 6)? TRUE : FALSE);
	ScrollCount = 0;

	for (uint8_t Nulls = 0; Nulls < 7; Nulls++)
		TextBuffer[LoadB++] = 0xFF;
	
	TextBuffer[LoadB] = 0x00;
	StrStart   = 0;
	StrEnd     = LoadB;	
	DelayCount = LCD_DELAYCOUNT_DEFAULT;

	UpdateLCD  = TRUE;
}

void LCD_WriteChar(uint8_t Byte, uint8_t Digit)
{
	uint16_t SegData  = 0x00;
	uint8_t  *BuffPtr = (&SegBuffer[0] + (Digit >> 1));

	if (Byte != 0xFF)
	  SegData = pgm_read_word(&LCD_SegTable[Byte]);	

	for (uint8_t BNib = 0; BNib < 4; BNib++)
	{
		uint8_t Mask          = 0xF0;
		uint8_t MaskedSegData = (SegData & 0x0000F);
	
		if (Digit & 0x01)
		{
			Mask = 0x0F;
			MaskedSegData <<= 4;
		}
		
		*BuffPtr = ((*BuffPtr & Mask) | MaskedSegData);

		SegData >>= 4;
		BuffPtr  += 5;
	}
}

ISR(LCD_vect, ISR_NOBLOCK)
{
	if (ScrollMode)
	{
		if (DelayCount)
		{
			DelayCount--;
		}
		else
		{
			if (!(ScrollCount))
			  UpdateLCD = TRUE;
			else
			  ScrollCount--;
		}
	}

	if (UpdateLCD)
	{
		for (uint8_t Character = 0; Character < 6; Character++)
		{
			uint8_t Byte = (StrStart + Character);

			if (Byte >= StrEnd)
			  Byte = TextBuffer[Byte - StrEnd];
			else
			  Byte = TextBuffer[Byte];
			
			LCD_WriteChar(Byte, Character);
		}
		
		if (StrStart++ == StrEnd)
		  StrStart = 1;
		
		ScrollCount = LCD_SCROLLCOUNT_DEFAULT;
		UpdateLCD = FALSE;
	}

	for (uint8_t LCDChar = 0; LCDChar < LCD_SEGBUFFER_SIZE; LCDChar++)
	  *(pLCDREG + LCDChar) = SegBuffer[LCDChar];
}
