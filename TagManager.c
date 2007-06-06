/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
                  www.fourwalledcubicle.com
*/

#define  INC_FROM_TM
#include "TagManager.h"

static uint8_t  TagExists       = FALSE;
static uint32_t DFDataBytesLeft = 0x00000000;

// ======================================================================================

/*
 NAME:      | TM_ShowTags
 PURPOSE:   | Manages the showing of tags in the stored program data (if present) onto the Butterfly's LCD
 ARGUMENTS: | None
 RETURNS:   | None
*/
void TM_ShowTags(void)
{
	DF_ContinuousReadEnable(0, 0);
	TagExists = FALSE;
	DFDataBytesLeft = SM_GetStoredDataSize(TYPE_FLASH);

	TM_FindNextTag();
	if (!(TagExists))
		return;

	for (;;)
	{
		if (JoyStatus)                         // Joystick is in the non-center position
		{
			if (JoyStatus & JOY_DOWN)          // Next tag
			  TM_FindNextTag();
			else if (JoyStatus & JOY_LEFT)
			  break;
			
			MAIN_WaitForJoyRelease();
		}

		SLEEPCPU(SLEEP_POWERSAVE);
	}
}

/*
 NAME:      | TM_FindNextTag (static)
 PURPOSE:   | Finds and displays the next tag in the stored program data (if present) - wraps to start if needed
 ARGUMENTS: | None
 RETURNS:   | None
*/
static void TM_FindNextTag(void)
{
	char       Buffer[21];
	const char HeadBuff[4]      = BT_TAGHEADER;
	char       TagByte;
	uint8_t    TotalOkHeadBytes = 0;
	
	MAIN_SETSTATUSLED(MAIN_STATLED_ORANGE);    // Orange = busy
	LCD_puts_f(WaitText);

	while (DFDataBytesLeft)
	{		
		TagByte = TM_GetNextByte();            // Get next byte from dataflash
		
		if (TagByte == HeadBuff[TotalOkHeadBytes++])
		{
			if (TotalOkHeadBytes == 4)
			{
				for (uint8_t BuffPos = 0; BuffPos < 20; BuffPos++)
				{
					TagByte = TM_GetNextByte();
					Buffer[BuffPos] = TagByte;
					
					if (TagByte == '\0')
					  break;
				}
				
				Buffer[20] = '\0';             // Make sure string is null-terminated

				TagExists  = TRUE;

				LCD_puts(Buffer);
				MAIN_SETSTATUSLED(MAIN_STATLED_GREEN); // Green = ready
				return;
			}
		}
		else
		{
			TotalOkHeadBytes = ((TagByte == HeadBuff[0])? 1 : 0);
		}
	}
	
	DFDataBytesLeft = SM_GetStoredDataSize(TYPE_FLASH);
	DF_ContinuousReadEnable(0, 0);
	
	if (TagExists == FALSE)
	{
		MAIN_SETSTATUSLED(MAIN_STATLED_GREEN); // Green = ready
		MAIN_ShowError(PSTR("NO TAGS"));
	}
	else
	{
		/* The following line _is_ recursion, but the function will only ever call itself
		   a maximum of one time. The function will call itself upon skipping from the last
		   tag stored in the program data to the first; to guard against infinite recursion
		   if no tags are present the system will error out if the TagExists flag is empty
		   after a full data read. Once a tag has been read and displayed onto the LCD, the
           function returns to the main tag handling routine.                               */
		TM_FindNextTag();
	}
}

static uint8_t TM_GetNextByte(void)
{
	DFDataBytesLeft--;
	
	return SPI_SPITransmit(0x00);
}
