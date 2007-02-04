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

#define INC_FROM_DRIVER
#include "LCD_Driver.h"

//                           LCD Text            + Nulls for scrolling + Null Termination
volatile char     TextBuffer[LCD_TEXTBUFFER_SIZE + LCD_DISPLAY_SIZE    + 1] = {};
volatile uint8_t  StrStart        = 0;
volatile uint8_t  StrEnd          = 0;
volatile uint8_t  ScrollFlags     = 0;
volatile uint8_t  ScrollCount     = 0;
volatile uint8_t  UpdateDisplay   = FALSE;

const    uint16_t LCD_SegTable[] PROGMEM =
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
	// Set the initial contrast level to maximum:
	LCD_CONTRAST_LEVEL(0x0F);

    // Select asynchronous clock source, enable all COM pins and enable all segment pins:
    LCDCRB  = (1<<LCDCS) | (3<<LCDMUX0) | (7<<LCDPM0);

    // Set LCD prescaler to give a framerate of 32Hz:
    LCDFRR  = (7<<LCDCD0);    

	// Enable LCD and set low power waveform, enable start of frame interrupt:
    LCDCRA  = (1<<LCDEN) | (1<<LCDAB) | (1<<LCDIE);
}

void LCD_puts_f(const char *FlashData)
{
	/* Rather than create a new buffer here (wasting RAM), the TextBuffer global
	   is re-used as a temp buffer. Once the ASCII data is loaded in to TextBuffer,
	   LCD_puts is called with it to post-process it into the correct format for the
	   LCD interrupt.                                                                */

	strcpy_P((char*)&TextBuffer[0], FlashData);
	LCD_puts((char*)&TextBuffer[0]);
}

void LCD_puts(const char *Data)
{
	uint8_t LoadB       = 0;
	uint8_t CurrByte;

	do
	{
		CurrByte = *(Data++);
		
		switch (CurrByte)
		{
			case '*'...'z':	                   // Valid character, load it into the array
				TextBuffer[LoadB++] = (CurrByte - '*');
				break;
			case 0x00:                         // Null termination of the string - ignore for now so the nulls can be appended below
				break;
			case ' ':                          // Space or invalid character, use 0xFF to display a blank
			default:
				TextBuffer[LoadB++] = LCD_SPACE_OR_INVALID_CHAR;
		}
	}
	while (CurrByte && (LoadB < LCD_TEXTBUFFER_SIZE));

	ScrollFlags = ((LoadB > LCD_DISPLAY_SIZE)? LCD_FLAG_SCROLL : 0x00);

	for (uint8_t Nulls = 0; Nulls < 7; Nulls++)
	  TextBuffer[LoadB++] = LCD_SPACE_OR_INVALID_CHAR; // Load in nulls to ensure that when scrolling, the display clears before wrapping
	
	TextBuffer[LoadB] = 0x00;                  // Null-terminate string
	
	StrStart      = 0;
	StrEnd        = LoadB;
	ScrollCount   = LCD_SCROLLCOUNT_DEFAULT + LCD_DELAYCOUNT_DEFAULT;
	UpdateDisplay = TRUE;
}

ISR(LCD_vect, ISR_BLOCK)
{
	if (ScrollFlags & LCD_FLAG_SCROLL)
	{
		if (!(ScrollCount--))
		{
			UpdateDisplay = TRUE;
			ScrollCount   = LCD_SCROLLCOUNT_DEFAULT;
		}
	}

	if (UpdateDisplay)
	{
		for (uint8_t Character = 0; Character < LCD_DISPLAY_SIZE; Character++)
		{
			uint8_t Byte = (StrStart + Character);

			if (Byte >= StrEnd)
			  Byte -= StrEnd;
			
			LCD_WriteChar(TextBuffer[Byte], Character);
		}
		
		if ((StrStart + (LCD_DISPLAY_SIZE + 1)) == StrEnd)
		  ScrollFlags |= LCD_FLAG_SCROLL_DONE;
		
		if (StrStart++ == StrEnd)
		  StrStart     = 1;

		UpdateDisplay  = FALSE;                         // Clear LCD management flags, LCD update is complete
	}
}

static inline void LCD_WriteChar(const uint8_t Byte, const uint8_t Digit)
{
	uint16_t SegData  = 0x0000;

	if (Byte != LCD_SPACE_OR_INVALID_CHAR)     // Null indicates invalid character or space
	  SegData = pgm_read_word(&LCD_SegTable[Byte]);	

	for (uint8_t BNib = 0; BNib < 4; BNib++)
	{
		uint8_t *BuffPtr      = (uint8_t*)(LCD_LCDREGS_START + (5 * BNib) + (Digit >> 1));
		uint8_t MaskedSegData = (SegData & 0x0000F);
	
		if (Digit & 0x01)
		  *BuffPtr = ((*BuffPtr & 0x0F) | (MaskedSegData << 4));
		else
		  *BuffPtr = ((*BuffPtr & 0xF0) | MaskedSegData);

		SegData >>= 4;
	}	
}
