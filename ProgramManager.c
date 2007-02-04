/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#define  INC_FROM_PM
#include "ProgramManager.h"

// PROGMEM CONSTANTS:
const char ProgTypes[4][5] PROGMEM = {"DATA", "EPRM", "FUSE", "LOCK"};

// ======================================================================================

void PM_ShowStoredItemSizes(void)
{
	char    Buffer[14];
	uint8_t ItemInfoIndex = 0;
	uint8_t TempB;
	
	JoyStatus = JOY_INVALID;                                            // Use an invalid joystick value to force the program to write the
	                                                                    // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			  (ItemInfoIndex == 0)? ItemInfoIndex = 3 : ItemInfoIndex--;
			else if (JoyStatus & JOY_DOWN)
			  (ItemInfoIndex == 3)? ItemInfoIndex = 0 : ItemInfoIndex++;
			else if (JoyStatus & JOY_LEFT)
			  return;
		
			strcpy_P(Buffer, ProgTypes[ItemInfoIndex]);

			switch (ItemInfoIndex)
			{
				case 0:
					ultoa(SM_GetStoredDataSize(TYPE_FLASH), &Buffer[5], 10);
					break;
				case 1:
					ultoa(SM_GetStoredDataSize(TYPE_EEPROM), &Buffer[5], 10);
					break;
				case 2:
					TempB = eeprom_read_byte(&EEPROMVars.TotalFuseBytes);
					ultoa(((TempB == 0xFF)? 0x00 : TempB), &Buffer[5], 10);
					break;
				case 3:
					TempB = eeprom_read_byte(&EEPROMVars.TotalLockBytes);
					ultoa(((TempB == 0xFF)? 0x00 : TempB), &Buffer[5], 10);
			}
	
			Buffer[4] = '-';
			LCD_puts(Buffer);

			MAIN_WaitForJoyRelease();
		}

		SLEEPCPU(SLEEP_POWERSAVE);
	}
}

void PM_StartProgAVR(void)
{
	uint8_t StoredLocksFuses;
	uint8_t ProgOptions = eeprom_read_byte(&EEPROMVars.PGOptions);

	if (!(ProgOptions) || (ProgOptions > 15))
	{
		MAIN_ShowError(PSTR("NOTHING SELECTED"));
		return;
	}

	LCD_puts_f(WaitText);
	SPI_SPIInit();
	
	if (!(DF_CheckCorrectOnboardChip()))
	  return;

	TIMEOUT_SLEEP_TIMER_OFF();

	USI_SPIInitMaster();
	MAIN_ResetCSLine(MAIN_RESETCS_ACTIVE);       // Capture the RESET line of the slave AVR
			
	for (uint8_t PacketB = 0; PacketB < 12; PacketB++) // Read the enter programming mode command bytes
	  PacketBytes[PacketB] = eeprom_read_byte(&EEPROMVars.EnterProgMode[PacketB]);
		
	CurrAddress      = 0;
	ProgrammingFault = ISPCC_NO_FAULT;
	
	ISPCC_EnterChipProgrammingMode();            // Try to sync with the slave AVR
	if (InProgrammingMode)                       // ISPCC_EnterChipProgrammingMode alters the InProgrammingMode flag
	{						
		if ((ProgOptions & PM_OPT_FLASH) && (ProgrammingFault == ISPCC_NO_FAULT))
		{
			MAIN_ShowProgType('C');
			
			if (!(eeprom_read_byte(&EEPROMVars.EraseCmdStored) == TRUE))
			{
				ProgrammingFault = ISPCC_FAULT_NOERASE;
				MAIN_ShowError(PSTR("NO ERASE CMD"));
			}
			else
			{
				PM_SendEraseCommand();
			}
		}

		if ((ProgOptions & PM_OPT_FLASH) && (ProgrammingFault == ISPCC_NO_FAULT))
		{
			MAIN_ShowProgType('D');

			if (!(SM_GetStoredDataSize(TYPE_FLASH))) // Check to make sure a program is present in memory
			{
				ProgrammingFault = ISPCC_FAULT_NODATATYPE;					
				MAIN_ShowError(PSTR("NO DATA"));
			}
			else
			{
				PM_CreateProgrammingPackets(TYPE_FLASH);
			}
		}
	
		if ((ProgOptions & PM_OPT_EEPROM) && (ProgrammingFault == ISPCC_NO_FAULT))
		{
			MAIN_ShowProgType('E');

			if (!(SM_GetStoredDataSize(TYPE_EEPROM))) // Check to make sure EEPROM data is present in memory
			{
				ProgrammingFault = ISPCC_FAULT_NODATATYPE;
				MAIN_ShowError(PSTR("NO EEPROM"));
			}
			else
			{
				PM_CreateProgrammingPackets(TYPE_EEPROM);
			}
		}

		if ((ProgOptions & PM_OPT_FUSE) && (ProgrammingFault == ISPCC_NO_FAULT))
		{
			MAIN_ShowProgType('F');
			
			StoredLocksFuses = eeprom_read_byte(&EEPROMVars.TotalFuseBytes);
			if (!(StoredLocksFuses) || (StoredLocksFuses == 0xFF))
			{
				ProgrammingFault = ISPCC_FAULT_NODATATYPE;
				MAIN_ShowError(PSTR("NO FUSE BYTES"));
			}
			else
			{
				PM_SendFuseLockBytes(TYPE_FUSE);
			}
		}

		if ((ProgOptions & PM_OPT_LOCK) && (ProgrammingFault == ISPCC_NO_FAULT))
		{
			if (ProgOptions & PM_OPT_FUSE)               // If fusebytes have already been written, we need to re-enter programming mode to latch them
			{
				MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE); // Release the RESET line of the slave AVR
				MAIN_Delay10MS(1);
				MAIN_ResetCSLine(MAIN_RESETCS_ACTIVE);   // Capture the RESET line of the slave AVR
				ISPCC_EnterChipProgrammingMode();        // Try to sync with the slave AVR
			}

			MAIN_ShowProgType('L');
		
			StoredLocksFuses = eeprom_read_byte(&EEPROMVars.TotalLockBytes);
			if (!(StoredLocksFuses) || (StoredLocksFuses == 0xFF))
			{
				ProgrammingFault = ISPCC_FAULT_NODATATYPE;
				MAIN_ShowError(PSTR("NO LOCK BYTES"));
			}
			else
			{
				PM_SendFuseLockBytes(TYPE_LOCK);
			}
		}

		if (ProgrammingFault == ISPCC_FAULT_TIMEOUT)
		  MAIN_ShowError(PSTR("TIMEOUT"));

		if (eeprom_read_byte(&EEPROMVars.StartupMode) != 1) // Supress final PROG DONE/FAIL message if in production mode
		{
			if (ProgrammingFault != ISPCC_NO_FAULT)
			{
				LCD_puts_f(PSTR("PROG FAILED"));
				TG_PlayToneSeq(TONEGEN_SEQ_PROGFAIL);
			}
			else
			{
				LCD_puts_f(PSTR("PROG DONE"));
				TG_PlayToneSeq(TONEGEN_SEQ_PROGDONE);		
			}
	
			LCD_WAIT_FOR_SCROLL_DONE();          // Loop until the message has finished scrolling completely
		}
	}
	else
	{
		MAIN_ShowError(SyncErrorMessage);
	}
	
	TOUT_SetupSleepTimer();
	MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE);     // Release the RESET line and allow the slave AVR to run	
	USI_SPIOff();
	DF_ENABLEDATAFLASH(FALSE);
	SPI_SPIOFF();
	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);       // Set status LEDs to green (ready)
}

void PM_ChooseProgAVROpts(void)
{
	char    Buffer[7];
	uint8_t ProgOptions     = eeprom_read_byte(&EEPROMVars.PGOptions);
	uint8_t SelectedOpt     = 0;
	uint8_t SelectedOptMask = 0x01;

	if (ProgOptions > 15)
	  ProgOptions = 0;

	MAIN_WaitForJoyRelease();

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte(&EEPROMVars.PGOptions, ProgOptions);
				return;
			}
			else if (JoyStatus & JOY_PRESS)
			{
				ProgOptions  ^= SelectedOptMask;
			}
			else if (JoyStatus & JOY_UP)
			{
				(SelectedOpt == 0)? SelectedOpt = ARRAY_UPPERBOUND(ProgTypes) : SelectedOpt--;
			}
			else if (JoyStatus & JOY_DOWN)
			{
				(SelectedOpt == ARRAY_UPPERBOUND(ProgTypes))? SelectedOpt = 0 : SelectedOpt++;
			}
			
			SelectedOptMask = pgm_read_byte(&BitTable[SelectedOpt]);
		
			strcpy_P(Buffer, ProgTypes[SelectedOpt]);
			Buffer[4] = '>';
			Buffer[5] = ((ProgOptions & SelectedOptMask) ? 'Y' : 'N');
			Buffer[6] = 0x00;

			LCD_puts(Buffer);

			MAIN_WaitForJoyRelease();
		}

		SLEEPCPU(SLEEP_POWERSAVE);
	}
}

void PM_SetProgramDataType(uint8_t Mask)
{
	uint8_t ProgOptions = eeprom_read_byte(&EEPROMVars.PGOptions);

	if (ProgOptions > 15)
	  ProgOptions = 0;

	if (Mask & PM_OPT_CLEARFLAGS)
	  ProgOptions  = 0;
	else
	  ProgOptions |= (Mask & ~(1 << 7));
	  
	eeprom_write_byte(&EEPROMVars.PGOptions, ProgOptions);
}

static void PM_SendFuseLockBytes(const uint8_t Type)
{
	uint8_t  TotalBytes;
	uint8_t* EEPROMAddress;

	if (Type == TYPE_FUSE)
	{
		TotalBytes    = eeprom_read_byte(&EEPROMVars.TotalFuseBytes);
		EEPROMAddress = &EEPROMVars.FuseBytes[0][0];
	}
	else
	{
		TotalBytes    = eeprom_read_byte(&EEPROMVars.TotalLockBytes);
		EEPROMAddress = &EEPROMVars.LockBytes[0][0];	
	}

	while (TotalBytes--)                                                // Write each of the fuse/lock bytes stored in memory to the slave AVR
	{
		for (uint8_t CommandByte = 0; CommandByte < 4; CommandByte++)   // Write each individual command byte
		{
			USI_SPITransmit(eeprom_read_byte(EEPROMAddress));
			EEPROMAddress++;
		}
		
		// Add some delay before programming next byte, if there is one:
		if (TotalBytes)
		  MAIN_Delay10MS(5);
	}
}

static void PM_SendEraseCommand(void)
{			
	for (uint8_t B = 3; B < 7 ; B++)                                    // Read out the erase chip command bytes
	  USI_SPITransmit(eeprom_read_byte(&EEPROMVars.EraseChip[B]));      // Send the erase chip commands
			
	if (eeprom_read_byte(&EEPROMVars.EraseChip[2]))                     // Value of 1 indicates a busy flag test
	{
		TCNT1  = 0;                                                     // Clear timer 1
		TCCR1B = ((1 << CS12) | (1 << CS10));                           // Start timer 1 with a Fcpu/1024 clock

		do
		  USI_SPITransmitWord(0xF000);
		while ((USI_SPITransmitWord(0x0000) & 0x01) && (TCNT1 < ISPCC_COMM_TIMEOUT));

		if (TCNT1 >= ISPCC_COMM_TIMEOUT)
		  ProgrammingFault = ISPCC_FAULT_TIMEOUT;
	
		TCCR1B = 0;                                                      // Stop timer 1
	}
	else                                                                 // Cleared flag means use a predefined delay
	{		
		MAIN_Delay1MS(eeprom_read_byte(&EEPROMVars.EraseChip[1]));       // Wait the erase delay
	}
}

static void PM_CreateProgrammingPackets(const uint8_t Type)
{			
	uint32_t BytesRead        = 0;
	uint32_t BytesToRead      = SM_GetStoredDataSize(Type);              // Get the byte size of the stored program
	uint16_t BytesPerProgram;
	uint16_t PageLength       = eeprom_read_word((Type == TYPE_FLASH)? &EEPROMVars.PageLength : &EEPROMVars.EPageLength);
	uint16_t BytesPerProgress = (BytesToRead / LCD_BARGRAPH_SIZE);
	uint8_t  ContinuedPage    = FALSE;
	uint8_t* EEPROMAddress;

	CurrAddress = 0;

	if (Type == TYPE_FLASH)
	{
		EEPROMAddress = (uint8_t*)&EEPROMVars.WriteProgram;             // Set the EEPROM pointer to the write flash command bytes location
		DF_ContinuousReadEnable(0, 0);
		PacketBytes[0] = AICB_CMD_PROGRAM_FLASH_ISP;
	}
	else
	{
		EEPROMAddress = (uint8_t*)&EEPROMVars.WriteEEPROM;              // Set the EEPROM pointer to the write EEPROM command bytes location
		DF_ContinuousReadEnable(SM_EEPROM_OFFSET / DF_INTERNALDF_BUFFBYTES, SM_EEPROM_OFFSET % DF_INTERNALDF_BUFFBYTES); // Start read from the EEPROM offset location
		PacketBytes[0] = AICB_CMD_PROGRAM_EEPROM_ISP;
	}

	for (uint8_t B = 0; B < 10; B++)                                    // Load in the write data command bytes
	{
		PacketBytes[B] = eeprom_read_byte(EEPROMAddress);               // Synthesise a write packet header
		EEPROMAddress++;                                                // Increment the EEPROM location counter
	}
	
	BytesPerProgram = ((uint16_t)PacketBytes[1] << 8)
	                | PacketBytes[2];

	while (BytesRead < BytesToRead)
	{
		if (PacketBytes[3] & ISPCC_PROG_MODE_PAGE)
		{
			if (PageLength > 160)                                       // Max 160 bytes at a time
			{
				if (!(ContinuedPage))                                   // Start of a new page, program in the first 150 bytes
				{
					BytesPerProgram  = 160;
					PacketBytes[3]  &= ~ISPCC_PROG_MODE_PAGEDONE;		
					ContinuedPage    = TRUE;
				}
				else                                                    // Middle of a page, program in the remainder
				{
					BytesPerProgram  = PageLength - 160;
					PacketBytes[3]  |= ISPCC_PROG_MODE_PAGEDONE;
					ContinuedPage    = FALSE;
				}
				
				for (uint16_t LoadB = 0; LoadB < BytesPerProgram; LoadB++)
				  PacketBytes[10 + LoadB] = SPI_SPITransmit(0x00);      // Load in the page				

				PacketBytes[1] = (uint8_t)(BytesPerProgram >> 8);
				PacketBytes[2] = (uint8_t)(BytesPerProgram);

				BytesRead += BytesPerProgram;                           // Increment the counter
			}
			else
			{
				for (uint16_t LoadB = 0; LoadB < PageLength; LoadB++)
				  PacketBytes[10 + LoadB] = SPI_SPITransmit(0x00);      // Load in the page
			
				PacketBytes[1]  = (uint8_t)(PageLength >> 8);
				PacketBytes[2]  = (uint8_t)(PageLength);
				PacketBytes[3] |= ISPCC_PROG_MODE_PAGEDONE;

				BytesRead += PageLength;                                // Increment the counter
			}
		}
		else
		{
			if ((BytesRead + BytesPerProgram) > BytesToRead)            // Less than a whole BytesPerProgram left of data to write
			{
				BytesPerProgram = BytesToRead - BytesRead;              // Next lot of bytes will be the remaining data length
				PacketBytes[1]  = (uint8_t)(BytesPerProgram >> 8);      // \. Save the new length
				PacketBytes[2]  = (uint8_t)(BytesPerProgram);           // /  into the data packet
			}

			for (uint16_t LoadB = 0; LoadB < BytesPerProgram; LoadB++)
			  PacketBytes[10 + LoadB] = SPI_SPITransmit(0x00);          // Load in the page
		
			BytesRead += BytesPerProgram;                               // Increment the counter
		}
	
		if (!(BytesRead & 0x0000FFFF) && (BytesRead & 0x00FF0000))      // Extended address required
		{
			USI_SPITransmit(V2P_LOAD_EXTENDED_ADDR_CMD);                // Load extended address command
			USI_SPITransmit(0x00);
			USI_SPITransmit((BytesRead & 0x00FF0000) >> 16);            // The 3rd byte of the long holds the extended address
			USI_SPITransmit(0x00);
		}

		ISPCC_ProgramChip();                                            // Start the program cycle
		LCD_BARGRAPH((uint8_t)(BytesRead / BytesPerProgress));          // Show the progress onto the LCD

		if (ProgrammingFault)                                           // Error out early if there's a problem
		  return;
	}
}

