/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "ProgramManager.h"

// Global Variables:
uint8_t  MemoryType          = 0;
uint8_t  CurrentMode         = PM_NO_SETUP;
uint16_t GPageLength         = 0;

// ======================================================================================

uint32_t PM_GetStoredDataSize(const uint8_t Type)
{
	/* This take a **LOT** of code and is accessed several times throughout
	   the program, so I've put it into a seperate function to save on flash. */

	uint32_t ProgDataSize = 0;

	eeprom_read_block((void*)&ProgDataSize, (const void*)((Type == TYPE_FLASH)? &EEPROMVars.DataSize : &EEPROMVars.EEPROMSize), sizeof(uint32_t));

	if (ProgDataSize == 0xFFFFFFFF)                                      // Blank EEPROM, return a size  of 0 bytes
	  ProgDataSize = 0x00;

	return ProgDataSize;
}

void PM_SetupDFAddressCounters(const uint8_t Type)
{
	uint32_t StartAddress;
	
	MemoryType  = Type;
	GPageLength = 0;

	if (Type == TYPE_FLASH)                                              // Type 1 = Flash
	  StartAddress = (CurrAddress << 1);                               // Convert flash word address to byte address
	else
	  StartAddress = CurrAddress + PM_EEPROM_OFFSET;                   // EEPROM uses byte addresses, and starts at the 257th kilobyte in Dataflash
	
	CurrPageAddress = 0;

	while (StartAddress >= DF_INTERNALDF_BUFFBYTES)                      // This loop is the equivalent of a DIV and a MOD
	{
		StartAddress -= DF_INTERNALDF_BUFFBYTES;                         // Subtract one page's worth of bytes from the desired address
		CurrPageAddress++;
	}
	
	CurrBuffByte = (uint16_t)StartAddress;                               // The buffer byte is the remainder
}

void PM_StoreProgramByte(const uint8_t Data)
{
	if (CurrBuffByte == DF_INTERNALDF_BUFFBYTES)
	{
		DF_CopyBufferToFlashPage(CurrPageAddress++);
		DF_BufferWriteEnable(0);
		CurrBuffByte = 0;
	}
	
	SPI_SPITransmit(Data);                                               // Store the byte, dataflash is in write mode due to DF_BufferWriteEnable
	CurrBuffByte++;
	GPageLength++;
}

void PM_InterpretAVRISPPacket(void)
{
	uint8_t* EEPROMAddress;

	switch (PacketBytes[0])
	{
		case CMD_ENTER_PROGMODE_ISP:
			MessageSize = 2;
						
			for (uint8_t PacketB = 0; PacketB <= 11; PacketB++)          // Save the enter programming mode command bytes
			  eeprom_write_byte(&EEPROMVars.EnterProgMode[PacketB], PacketBytes[PacketB]);
			
			InProgrammingMode = TRUE;                                    // Set the flag, prevent the user from exiting the V2P state machine			
			CurrentMode = PM_NO_SETUP;                                   // Clear the current mode variable

			MAIN_SETSTATUSLED(MAIN_STATLED_RED);
			PacketBytes[1] = STATUS_CMD_OK;

			break;			
		case CMD_LEAVE_PROGMODE_ISP:
			MessageSize = 2;

			PM_CheckEndOfFuseLockData();                                 // Check for remaining bytes to be stored and general cleanup
			
			InProgrammingMode = FALSE;                                   // Clear the flag, allow the user to exit the V2P state machine

			DF_EnableDataflash(FALSE);

			MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);
			PacketBytes[1] = STATUS_CMD_OK;

			break;
		case CMD_READ_SIGNATURE_ISP:
			MessageSize = 4;

			PacketBytes[1] = STATUS_CMD_OK;                              // Data byte is encased in CMD_OKs
			PacketBytes[2] = 0x01;                                       // Signature bytes all return "01" in storage mode
			PacketBytes[3] = STATUS_CMD_OK;                              // Data byte is encased in CMD_OKs

			break;
		case CMD_CHIP_ERASE_ISP:
			MessageSize = 2;

			for (uint8_t PacketB = 1; PacketB <= 6; PacketB++)          // Save the erase chip command bytes to EEPROM
			  eeprom_write_byte(&EEPROMVars.EraseChip[PacketB], PacketBytes[PacketB]);

			for (uint8_t Byte = 0; Byte < 8; Byte++)                    // Clear the program and EEPROM size counters
			{
				eeprom_write_byte(&EEPROMVars.DataSize[Byte], 0x00);
				eeprom_write_byte(&EEPROMVars.EEPROMSize[Byte], 0x00);
			}
						
			eeprom_write_byte(&EEPROMVars.EraseCmdStored, TRUE);
			
			PacketBytes[1] = STATUS_CMD_OK;
			
			break;
		case CMD_READ_OSCCAL_ISP:
			MessageSize = 4;

			PacketBytes[1] = STATUS_CMD_OK;                             // Data byte is encased in CMD_OKs
			PacketBytes[2] = 0x00;                                      // Return 0x00 for the OSCCAL byte
			PacketBytes[3] = STATUS_CMD_OK;                             // Data byte is encased in CMD_OKs

			break;
		case CMD_READ_FUSE_ISP:
		case CMD_READ_LOCK_ISP:
			MessageSize = 4;
	
			if (CurrentMode != PM_LOCKFUSEBITS_READ)                    // First lock or fuse byte being read, set the EEPROM pointer
			{
				PM_CheckEndOfFuseLockData();                            // Check for remaining bytes to be stored and general cleanup
				
				CurrBuffByte = 0;
				CurrentMode  = PM_LOCKFUSEBITS_READ;
			}
			
			if (CurrBuffByte > eeprom_read_byte((PacketBytes[0] == CMD_READ_FUSE_ISP)? &EEPROMVars.TotalFuseBytes : &EEPROMVars.TotalLockBytes))  // Trying to read more fuse/lock bytes than are stored in memory
			{
				PacketBytes[2] = 0xFF;                                 // Return 0xFF for the fuse/lock byte
			}
			else
			{
				PacketBytes[2] = eeprom_read_byte((uint8_t*)(((PacketBytes[0] == CMD_READ_FUSE_ISP)? &EEPROMVars.FuseBytes : &EEPROMVars.LockBytes) // Starting location
									                         + (CurrBuffByte << 2) + (PacketBytes[1] - 1)));                                        // The start position of the actual fuse/lock byte to read (4 bytes each));
			}

			PacketBytes[1] = STATUS_CMD_OK;                            // Data byte is encased in CMD_OKs
			PacketBytes[3] = STATUS_CMD_OK;                            // Data byte is encased in CMD_OKs

			CurrBuffByte++;

			break;
		case CMD_PROGRAM_FUSE_ISP:
		case CMD_PROGRAM_LOCK_ISP:
			MessageSize = 3;

			if (CurrentMode != PM_LOCKFUSEBITS_WRITE)                  // First lock or fuse byte being written, set the EEPROM pointer
			{
				CurrentMode   = PM_LOCKFUSEBITS_WRITE;
				CurrBuffByte  = 0;                                     // CurrBuffByte is used to store the total fuse/lock bytes written
			}

			if (PacketBytes[0] == CMD_PROGRAM_FUSE_ISP)
			{
				EEPROMAddress = (uint8_t*)(&EEPROMVars.FuseBytes + (CurrBuffByte << 2));
				MemoryType    = TYPE_FUSE;
			}
			else
			{
				EEPROMAddress = (uint8_t*)(&EEPROMVars.LockBytes + (CurrBuffByte << 2));
				MemoryType    = TYPE_LOCK;
			}				
			
			if (CurrBuffByte < PM_MAX_FUSELOCKBITS)
			{
				for (uint8_t FLByte = 1; FLByte <= 4; FLByte++)
				{
					eeprom_write_byte(EEPROMAddress, PacketBytes[FLByte]);
					EEPROMAddress++;
				}

				CurrBuffByte++;                                        // Increment the total fuse/lock bytes written counter
			}
			
			PacketBytes[1] = STATUS_CMD_OK;                            // Two CMD_OKs are always returned
			PacketBytes[2] = STATUS_CMD_OK;                            // Two CMD_OKs are always returned

			break;
		case CMD_PROGRAM_FLASH_ISP:
		case CMD_PROGRAM_EEPROM_ISP:
			MessageSize = 2;

			if (CurrentMode != PM_DATAFLASH_WRITE)                     // First programming packet
			{
				if (PacketBytes[0] == CMD_PROGRAM_FLASH_ISP)           // Flash programming mode
				{
					EEPROMAddress = (uint8_t*)&EEPROMVars.WriteProgram; // Set the eeprom address to the Program command bytes location
					PM_SetupDFAddressCounters(TYPE_FLASH);
				}
				else                                                   // EEPROM programming mode
				{
					EEPROMAddress = (uint8_t*)&EEPROMVars.WriteEEPROM;  // Set the eeprom address to the EEPROM command bytes location
					PM_SetupDFAddressCounters(TYPE_EEPROM);
				}
				
				DF_BufferWriteEnable(CurrBuffByte);
				CurrentMode = PM_DATAFLASH_WRITE;
				
				for (uint8_t B = 1; B <= 9; B++)                       // Save the command bytes
				  eeprom_write_byte(EEPROMAddress, PacketBytes[B]);
			}

			uint16_t BytesToWrite = ((uint16_t)PacketBytes[1] << 8)
			                      | PacketBytes[2];

			for (uint16_t CurrByte = 0; CurrByte < BytesToWrite; CurrByte++)
				PM_StoreProgramByte(PacketBytes[10 + CurrByte]);

			if (!(GPageLength & PM_PAGELENGTH_FOUNDBIT) && (PacketBytes[3] & ISPCC_PROG_MODE_PAGEDONE) && GPageLength)
			{
				eeprom_write_word(((MemoryType == TYPE_FLASH)? &EEPROMVars.PageLength : &EEPROMVars.EPageLength), GPageLength);
		
				GPageLength |= PM_PAGELENGTH_FOUNDBIT;                 // Bit 15 is used to indicate if the length has been found
			}

			PacketBytes[1] = STATUS_CMD_OK;
		
			break;
		case CMD_READ_FLASH_ISP:
		case CMD_READ_EEPROM_ISP:	
			if (CurrentMode != PM_DATAFLASH_READ)
			{
				PM_CheckEndOfFuseLockData();                           // Check for remaining bytes to be stored and general cleanup
				
				PM_SetupDFAddressCounters((PacketBytes[0] == CMD_READ_FLASH_ISP)? TYPE_FLASH : TYPE_EEPROM);
				DF_ContinuousReadEnable(CurrPageAddress, CurrBuffByte);
				
				CurrentMode = PM_DATAFLASH_READ;
				CurrAddress = 0;
			}

			uint16_t BytesToRead = ((uint16_t)PacketBytes[1] << 8)    // \. Load in the number of bytes that is to
			                     | PacketBytes[2];                    // /  be read into a temp variable (MSB first)
						
			uint16_t BytesInMem  = PM_GetStoredDataSize((PacketBytes[0] == CMD_READ_FLASH_ISP)? TYPE_FLASH : TYPE_EEPROM);

			for (uint16_t ReadByte = 0; ReadByte < BytesToRead; ReadByte++)
			{
			   PacketBytes[2 + ReadByte] = ((CurrAddress < BytesInMem)? SPI_SPITransmit(0x00) : 0xFF); // Read in the next dataflash byte if present
			   V2P_IncrementCurrAddress();
			}
			
			MessageSize = BytesToRead + 3;

			PacketBytes[1]               = STATUS_CMD_OK; // Return data should be encompassed in STATUS_CMD_OKs
			PacketBytes[2 + BytesToRead] = STATUS_CMD_OK; // Return data should be encompassed in STATUS_CMD_OKs
		
			break;
		default:
			MessageSize = 1;
			
			PacketBytes[1] = STATUS_CMD_UNKNOWN;
	}

	V2P_SendPacket();                                   // Send the response packet
}

void PM_CheckEndOfFuseLockData(void)
{
	if (CurrentMode == PM_DATAFLASH_WRITE)
	{
		if (CurrBuffByte)                               // Data in the dataflash buffer, pending to be written
		  DF_CopyBufferToFlashPage(CurrPageAddress);    // Save the remaining buffer bytes

		uint32_t DataSize = ((CurrPageAddress * DF_INTERNALDF_BUFFBYTES) + CurrBuffByte);

		if (MemoryType == TYPE_FLASH)
		{
			eeprom_write_block((const void*)&DataSize, (void*)&EEPROMVars.DataSize, sizeof(uint32_t));
		}
		else
		{
			DataSize -= PM_EEPROM_OFFSET;                // Remove DataFlash EEPROM start offset
			eeprom_write_block((const void*)&DataSize, (void*)&EEPROMVars.EEPROMSize, sizeof(uint32_t));
		}
	}
	else if (CurrentMode == PM_LOCKFUSEBITS_WRITE)
	{
		// CurrBuffByte stores the total number of fuse/lock bytes written in this case:
		eeprom_write_byte(((MemoryType == TYPE_FUSE)? &EEPROMVars.TotalFuseBytes : &EEPROMVars.TotalLockBytes), CurrBuffByte);
	}
}

void PM_SendFuseLockBytes(const uint8_t Type)
{
	uint8_t* EEPROMAddress;
	uint8_t  TotalBytes;

	if (Type == TYPE_FUSE)
	{
		TotalBytes    = eeprom_read_byte(&EEPROMVars.TotalFuseBytes);
		EEPROMAddress = EEPROMVars.FuseBytes;
	}
	else
	{
		TotalBytes    = eeprom_read_byte(&EEPROMVars.TotalLockBytes);
		EEPROMAddress = EEPROMVars.LockBytes;	
	}

	while (TotalBytes--)                              // Write each of the fuse/lock bytes stored in memory to the slave AVR
	{
		for (uint8_t CommandByte = 0; CommandByte < 4; CommandByte++)      // Write each individual command byte
		{
			USI_SPITransmit(eeprom_read_byte(EEPROMAddress));
			EEPROMAddress++;
		}
		
		// Add some delay before programming next byte, if there is one:
		if (TotalBytes)
		  MAIN_Delay10MS(5);
	}
}

void PM_SendEraseCommand(void)
{			
	for (uint8_t B = 2; B < 6 ; B++)                  // Read out the erase chip command bytes
	  USI_SPITransmit(eeprom_read_byte(&EEPROMVars.EraseChip[B])); // Send the erase chip commands
			
	if (eeprom_read_byte(&EEPROMVars.EraseChip[1]))   // Value of 1 indicates a busy flag test
	{
		do
			USI_SPITransmitWord(0xF000);
		while (USI_SPITransmitWord(0x0000) & 0x01);
	}
	else                                              // Cleared flag means use a predefined delay
	{		
		MAIN_Delay1MS(eeprom_read_byte(&EEPROMVars.EraseChip[0])); // Wait the erase delay
	}
}

void PM_CreateProgrammingPackets(const uint8_t Type)
{			
	uint32_t BytesRead       = 0;
	uint32_t BytesToRead     = PM_GetStoredDataSize(Type);      // Get the byte size of the stored program
	uint16_t BytesPerProgram;
	uint16_t PageLength;
	uint8_t* EEPROMAddress;
	uint8_t  ContinuedPage   = FALSE;

	PageLength  = eeprom_read_word((Type == TYPE_FLASH)? &EEPROMVars.PageLength : &EEPROMVars.EPageLength);
	CurrAddress = 0;

	if (Type == TYPE_FLASH)
	{
		EEPROMAddress = (uint8_t*)&EEPROMVars.WriteProgram;       // Set the EEPROM pointer to the write flash command bytes location
		DF_ContinuousReadEnable(0, 0);
		PacketBytes[0] = CMD_PROGRAM_FLASH_ISP;
	}
	else
	{
		EEPROMAddress = (uint8_t*)&EEPROMVars.WriteEEPROM;        // Set the EEPROM pointer to the write EEPROM command bytes location
		DF_ContinuousReadEnable(PM_EEPROM_OFFSET / DF_INTERNALDF_BUFFBYTES, PM_EEPROM_OFFSET % DF_INTERNALDF_BUFFBYTES); // Start read from the EEPROM offset location
		PacketBytes[0] = CMD_PROGRAM_EEPROM_ISP;
	}

	for (uint8_t B = 1; B <= 9 ; B++)                 // Load in the write data command bytes
	{
		PacketBytes[B] = eeprom_read_byte(EEPROMAddress); // Synthesise a write packet header
		EEPROMAddress++;                               // Increment the EEPROM location counter
	}
	
	BytesPerProgram = ((uint16_t)PacketBytes[1] << 8)
	                | PacketBytes[2];

	while (BytesRead < BytesToRead)
	{
		if (PacketBytes[3] & ISPCC_PROG_MODE_PAGE)
		{
			if (PageLength > 160) // Max 160 bytes at a time
			{
				if (!(ContinuedPage))                      // Start of a new page, program in the first 150 bytes
				{
					BytesPerProgram = 160;
					PacketBytes[3] &= ~ISPCC_PROG_MODE_PAGEDONE;		
					ContinuedPage = TRUE;
				}
				else                                       // Middle of a page, program in the remainder
				{
					BytesPerProgram = PageLength - 160;
					PacketBytes[3] |= ISPCC_PROG_MODE_PAGEDONE;
					ContinuedPage = FALSE;
				}
				
				for (uint16_t LoadB = 0; LoadB < BytesPerProgram; LoadB++)
					PacketBytes[10 + LoadB] = SPI_SPITransmit(0x00);  // Load in the page				

				PacketBytes[1] = (uint8_t)(BytesPerProgram >> 8);
				PacketBytes[2] = (uint8_t)(BytesPerProgram);

				BytesRead += BytesPerProgram;                         // Increment the counter
			}
			else
			{
				for (uint16_t LoadB = 0; LoadB < PageLength; LoadB++)
					PacketBytes[10 + LoadB] = SPI_SPITransmit(0x00);  // Load in the page
			
				PacketBytes[1]  = (uint8_t)(PageLength >> 8);
				PacketBytes[2]  = (uint8_t)(PageLength);
				PacketBytes[3] |= ISPCC_PROG_MODE_PAGEDONE;

				BytesRead += PageLength;                            // Increment the counter
			}
		}
		else
		{
			if ((BytesRead + BytesPerProgram) > BytesToRead)        // Less than a whole BytesPerProgram left of data to write
			{
				BytesPerProgram = BytesToRead - BytesRead;          // Next lot of bytes will be the remaining data length
				PacketBytes[1] = (uint8_t)(BytesPerProgram >> 8);   // \. Save the new length
				PacketBytes[2] = (uint8_t)(BytesPerProgram);        // /  into the data packet
			}

			for (uint16_t LoadB = 0; LoadB < BytesPerProgram; LoadB++)
				PacketBytes[10 + LoadB] = SPI_SPITransmit(0x00);    // Load in the page
			
			BytesRead += BytesPerProgram;                           // Increment the counter
		}
	
		if (!(BytesRead & 0xFFFF) && (BytesRead & 0x00FF0000))      // Extended address required
		{
			USI_SPITransmit(V2P_LOAD_EXTENDED_ADDR_CMD);            // Load extended address command
			USI_SPITransmit(0x00);
			USI_SPITransmit((BytesRead & 0x00FF0000) >> 16);        // The 3rd byte of the long holds the extended address
			USI_SPITransmit(0x00);
		}

		ISPCC_ProgramChip();                                        // Start the program cycle
	}
}

void PM_ShowStoredItemSizes(void)
{
	uint8_t Buffer[14];
	uint8_t ItemInfoIndex = 0;
	uint8_t TempB;
	
	JoyStatus = 1;
	
	while (1)
	{
		if (JoyStatus)                           // Joystick is in the non-center position
		{
			if (JoyStatus & JOY_UP)              // Previous item
				(ItemInfoIndex == 0)? ItemInfoIndex = 3 : ItemInfoIndex--;
			else if (JoyStatus & JOY_DOWN)      // Next item
				(ItemInfoIndex == 3)? ItemInfoIndex = 0 : ItemInfoIndex++;
			else if (JoyStatus & JOY_LEFT)
				return;
		
			switch (ItemInfoIndex)
			{
				case 0:
					strcpy_P(Buffer, PSTR("DATA-"));
					ultoa(PM_GetStoredDataSize(TYPE_FLASH), &Buffer[5], 10);
					break;
				case 1:
					strcpy_P(Buffer, PSTR("EPRM-"));
					ultoa(PM_GetStoredDataSize(TYPE_EEPROM), &Buffer[5], 10);
					break;
				case 2:
					strcpy_P(Buffer, PSTR("FUSE-"));
					TempB = eeprom_read_byte(&EEPROMVars.TotalFuseBytes);
					MAIN_IntToStr(((TempB == 0xFF)? 0x00 : TempB), &Buffer[5]);
					break;
				case 3:
					strcpy_P(Buffer, PSTR("LOCK-"));
					TempB = eeprom_read_byte(&EEPROMVars.TotalLockBytes);
					MAIN_IntToStr(((TempB == 0xFF)? 0x00 : TempB), &Buffer[5]);		
			}
	
			LCD_puts(Buffer);

			MAIN_WaitForJoyRelease();
		}
	}
}