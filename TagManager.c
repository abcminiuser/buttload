/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "TagManager.h"

uint8_t  TagExists  = FALSE;
uint32_t DFPos      = 0;

// ======================================================================================

void TM_ShowTags(void)
{
	DF_ContinuousReadEnable(0, 0);
	TagExists = FALSE;
	DFPos = 0;
		
	TM_FindNextTag();
	if (!(TagExists))
	{
		DF_EnableDataflash(FALSE);
		return;
	}

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
	}
	
	DF_EnableDataflash(FALSE);
}

void TM_FindNextTag(void)
{
	uint8_t  Buffer[21];
	uint8_t  HeadBuff[4]      = BT_TAGHEADER;
	uint32_t ProgDataSize     = PM_GetStoredDataSize(TYPE_FLASH);
	uint8_t  TotalOkHeadBytes = 0;
	uint8_t  TagByte;
	uint8_t  BytesRead;
	
	MAIN_SETSTATUSLED(MAIN_STATLED_ORANGE);    // Orange = busy
	LCD_puts_f(WaitText);

	while (DFPos < ProgDataSize)
	{
		BytesRead = 1;
		
		TagByte = SPI_SPITransmit(0x00);       // Get next byte from dataflash
		if (TagByte == HeadBuff[TotalOkHeadBytes++])
		{
			if (TotalOkHeadBytes == 4)
			{
				uint8_t HB;
			
				for (HB = 0; HB < 20; HB++)
				{
					TagByte = SPI_SPITransmit(0x00);
					Buffer[HB] = TagByte;
					
					if (TagByte == 0x00)
					  break;
				}
				
				Buffer[20] = '\0';             // Make sure string is null-terminated

				TagExists = TRUE;
				BytesRead = (HB + 2);

				LCD_puts(Buffer);
				MAIN_SETSTATUSLED(MAIN_STATLED_GREEN); // Green = ready
				return;
			}
		}
		else
		{
			TotalOkHeadBytes = ((TagByte == HeadBuff[0])? 1 : 0);
		}

		DFPos += BytesRead;
	}
	
	DF_ContinuousReadEnable(0, 0);
	DFPos = 0;
	
	if (TagExists == FALSE)
	{
		MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);  // Green = ready
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
