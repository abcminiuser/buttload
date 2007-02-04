/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#define  INC_FROM_TM
#include "TagManager.h"

uint8_t  TagExists       = FALSE;
uint32_t DFDataBytesLeft = 0x0000;

// ======================================================================================

void TM_ShowTags(void)
{
	DF_ContinuousReadEnable(0, 0);
	TagExists = FALSE;
	DFDataBytesLeft = SM_GetStoredDataSize(TYPE_FLASH);

	TM_FindNextTag();
	if (!(TagExists))
	{
		DF_ENABLEDATAFLASH(FALSE);
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

		IDLECPU();
	}
	
	DF_ENABLEDATAFLASH(FALSE);
}

static void TM_FindNextTag(void)
{
	char     Buffer[21];
	char     HeadBuff[4]      = BT_TAGHEADER;
	char     TagByte;
	uint8_t  TotalOkHeadBytes = 0;
	uint8_t  DFBytesRead      = 0;
	
	MAIN_SETSTATUSLED(MAIN_STATLED_ORANGE);    // Orange = busy
	LCD_puts_f(WaitText);

	while ((DFDataBytesLeft - DFBytesRead) != 0)
	{
		DFBytesRead = 1;
		
		TagByte = SPI_SPITransmit(0x00);       // Get next byte from dataflash
		if (TagByte == HeadBuff[TotalOkHeadBytes++])
		{
			if (TotalOkHeadBytes == 4)
			{
				uint8_t BuffPos;
			
				for (BuffPos = 0; BuffPos < 20; BuffPos++)
				{
					TagByte = SPI_SPITransmit(0x00);
					Buffer[BuffPos] = TagByte;
					
					if (TagByte == 0x00)
					  break;
				}
				
				Buffer[20] = '\0';             // Make sure string is null-terminated

				TagExists   = TRUE;
				DFBytesRead = (BuffPos + 2);

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
