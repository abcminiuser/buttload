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
uint8_t  InPMMode            = FALSE;

// ======================================================================================

uint32_t PM_GetStoredDataSize(uint8_t Type)
{
	/* This take a **LOT** of code (202 bytes), and is accessed several times throughout
	   the program, so I've put it into a seperate function to save on flash.            */

	uint32_t ProgDataSize = 0;
	uint16_t EEPROMAddress;

	EEPROMAddress = ((Type == TYPE_FLASH)? Prog_DataSize : Prog_EEPROMSize);
	ProgDataSize  = ((uint32_t)eeprom_read_byte_169(&EEPROMAddress) << 24);
	EEPROMAddress++;
	ProgDataSize |= ((uint32_t)eeprom_read_byte_169(&EEPROMAddress) << 16);
	EEPROMAddress++;
	ProgDataSize |= ((uint32_t)eeprom_read_byte_169(&EEPROMAddress) << 8);
	EEPROMAddress++;
	ProgDataSize |= eeprom_read_byte_169(&EEPROMAddress);

	if (ProgDataSize == 0xFFFFFFFF)                                      // Blank EEPROM, return a size  of 0 bytes
	   ProgDataSize = 0x00;

	return ProgDataSize;
}

void PM_SetupDFAddressCounters(uint8_t Type)
{
	uint32_t StartAddress;
	
	MemoryType = Type;
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

void PM_StoreProgramByte(uint8_t Data)
{
	if (CurrBuffByte == DF_INTERNALDF_BUFFBYTES)
	{
		DF_CopyBufferToFlashPage(CurrPageAddress++);
		DF_CopyFlashPageToBuffer(CurrPageAddress);
		DF_BufferWriteEnable(0);
		CurrBuffByte = 0;
	}
	
	SPI_SPITransmit(Data);                                               // Store the byte, dataflash is in write mode due to DF_BufferWriteEnable
	CurrBuffByte++;
	GPageLength++;
}

void PM_InterpretAVRISPPacket(void)
{
	uint16_t EEPROMAddress;

	switch (PacketBytes[0])
	{
		case CMD_ENTER_PROGMODE_ISP:
			MessageSize = 2;
			
			EEPROMAddress = Prog_EnterProgMode;
			
			for (uint8_t PacketB = 0; PacketB <= 11; PacketB++)          // Save the enter programming mode command bytes
			{
				eeprom_write_byte_169(&EEPROMAddress, PacketBytes[PacketB]);
				EEPROMAddress++;
			}
			
			InProgrammingMode = TRUE;                                    // Set the flag, prevent the user from exiting the V2P state machine			
			CurrentMode = PM_NO_SETUP;                                   // Clear the current mode variable

			MAIN_SETSTATUSLED(MAIN_STATLED_RED);
			PacketBytes[1] = STATUS_CMD_OK;

			break;			
		case CMD_LEAVE_PROGMODE_ISP:
			MessageSize = 2;

			PM_CheckEndOfProgramming();                                  // Check if the last command was a program - if so store the program length
			PM_CheckEndOfFuseLockStore();                                // Check if the last command was a fuse/lock byte program - if so store the total number of bytes
			
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

			EEPROMAddress = Prog_EraseChip;                              // Program erase chip command start address
			
			for (uint8_t PacketB = 1; PacketB <= 6; PacketB++)          // Save the erase chip command bytes to EEPROM
			{
				eeprom_write_byte_169(&EEPROMAddress, PacketBytes[PacketB]);
				EEPROMAddress++;
			}

			EEPROMAddress = Prog_DataSize;
										
			for (uint8_t Byte = 0; Byte < 8; Byte++)                    // Clear the program and EEPROM size counters
			{
				if (Byte == 4)
				   EEPROMAddress = Prog_EEPROMSize;

				eeprom_write_byte_169(&EEPROMAddress, 0x00);
				EEPROMAddress++;						
			}
			
			//DF_EraseInternalDF();  TODO: FIX ROUTINE
			
			eeprom_write_byte_169(&Prog_EraseCmdStored, TRUE);
			
			PacketBytes[1] = STATUS_CMD_OK;
			
			break;
		case CMD_READ_OSCCAL_ISP:
			MessageSize = 4;

			PacketBytes[1] = STATUS_CMD_OK;                             // Data byte is encased in CMD_OKs
			PacketBytes[1] = 0x00;                                      // Return 0x00 for the OSCCAL byte
			PacketBytes[3] = STATUS_CMD_OK;                             // Data byte is encased in CMD_OKs

			break;
		case CMD_READ_FUSE_ISP:
		case CMD_READ_LOCK_ISP:
			MessageSize = 4;
	
			if (CurrentMode != PM_LOCKFUSEBITS_READ)                    // First lock or fuse byte being read, set the EEPROM pointer
			{
				PM_CheckEndOfProgramming();                             // Check if the last command was a program - if so store the program length
				PM_CheckEndOfFuseLockStore();                           // Check if the last command was a fuse/lock byte program - if so store the total number of bytes
				
				CurrBuffByte = 0;
				CurrentMode  = PM_LOCKFUSEBITS_READ;
			}
			
			EEPROMAddress = ((PacketBytes[0] == CMD_READ_FUSE_ISP)? Prog_TotalFuseBytes : Prog_TotalLockBytes);

			if (CurrBuffByte > eeprom_read_byte_169(&EEPROMAddress))    // Trying to read more fuse/lock bytes than are stored in memory
			{
				PacketBytes[2] = 0xFF;                                  // Return 0xFF for the fuse/lock byte
			}
			else
			{
				EEPROMAddress  = ((PacketBytes[0] == CMD_READ_FUSE_ISP)? Prog_FuseBytes : Prog_LockBytes) // Starting location
									+ (CurrBuffByte << 2) + (PacketBytes[1] - 1); // The start position of the actual fuse/lock byte to read (4 bytes each)

				PacketBytes[2] = eeprom_read_byte_169(&EEPROMAddress); // Return the fuse/lock byte
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
				EEPROMAddress = Prog_FuseBytes + (CurrBuffByte << 2);
				MemoryType    = TYPE_FUSE;
			}
			else
			{
				EEPROMAddress = Prog_LockBytes + (CurrBuffByte << 2);
				MemoryType    = TYPE_LOCK;
			}				
			
			if (CurrBuffByte < PM_MAX_FUSELOCKBITS)
			{
				for (uint8_t FLByte = 1; FLByte <= 4; FLByte++)
				{
					eeprom_write_byte_169(&EEPROMAddress, PacketBytes[FLByte]);
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
					EEPROMAddress = Prog_WriteProgram;                 // Set the eeprom address to the Program command bytes location
					PM_SetupDFAddressCounters(TYPE_FLASH);
				}
				else                                                  // EEPROM programming mode
				{
					EEPROMAddress = Prog_WriteEEPROM;                  // Set the eeprom address to the EEPROM command bytes location
					PM_SetupDFAddressCounters(TYPE_EEPROM);
				}
				
				DF_BufferWriteEnable(CurrBuffByte);
				CurrentMode = PM_DATAFLASH_WRITE;
				
				for (uint8_t B = 1; B <= 9; B++)                       // Save the command bytes
				{
					eeprom_write_byte_169(&EEPROMAddress, PacketBytes[B]);
					EEPROMAddress++;				
				}
			}

			uint16_t BytesToWrite = ((uint16_t)PacketBytes[1] << 8)
			                      | PacketBytes[2];

			for (uint16_t CurrByte = 0; CurrByte < BytesToWrite; CurrByte++)
				PM_StoreProgramByte(PacketBytes[10 + CurrByte]);

			if (!(GPageLength & PM_PAGELENGTH_FOUNDBIT) && (PacketBytes[3] & ISPCC_PROG_MODE_PAGEDONE) && GPageLength)
			{
				EEPROMAddress = ((MemoryType == TYPE_FLASH)? Prog_PageLength : Prog_EPageLength);
	
				eeprom_write_byte_169(&EEPROMAddress, (uint8_t)(GPageLength >> 8));
				EEPROMAddress++;
				eeprom_write_byte_169(&EEPROMAddress, (uint8_t)GPageLength);		
		
				GPageLength |= PM_PAGELENGTH_FOUNDBIT;                 // Bit 15 is used to indicate if the length has been found
			}

			PacketBytes[1] = STATUS_CMD_OK;
		
			break;
		case CMD_READ_FLASH_ISP:
		case CMD_READ_EEPROM_ISP:	
			if (CurrentMode != PM_DATAFLASH_READ)
			{
				PM_CheckEndOfProgramming();                           // Check if the last command was a program - if so store the program length
				PM_CheckEndOfFuseLockStore();                         // Check if the last command was a fuse/lock byte program - if so store the total number of bytes
				
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

void PM_CheckEndOfProgramming(void)
{
   uint16_t EEPROMAddress;

   if (CurrentMode == PM_DATAFLASH_WRITE)
   {
      if (CurrBuffByte)                               // Data in the dataflash buffer, pending to be written
         DF_CopyBufferToFlashPage(CurrPageAddress);   // Save the buffer

      uint32_t DataSize = ((uint32_t)CurrPageAddress * DF_INTERNALDF_BUFFBYTES) + CurrBuffByte;

      if (MemoryType == TYPE_FLASH)
      {
         EEPROMAddress = Prog_DataSize;       
      }
      else
      {
         EEPROMAddress = Prog_EEPROMSize;
         DataSize -= PM_EEPROM_OFFSET;                // Remove DataFlash starting offset
      }

      eeprom_write_byte_169(&EEPROMAddress, (DataSize >> 24));
      EEPROMAddress++;
      eeprom_write_byte_169(&EEPROMAddress, (DataSize >> 16));             
      EEPROMAddress++;
      eeprom_write_byte_169(&EEPROMAddress, (DataSize >> 8));
      EEPROMAddress++;
      eeprom_write_byte_169(&EEPROMAddress, DataSize);
   }
}

void PM_CheckEndOfFuseLockStore(void)
{
	uint16_t EEPROMAddress;

	if (CurrentMode == PM_LOCKFUSEBITS_WRITE)
	{
		EEPROMAddress = ((MemoryType == TYPE_FUSE)? Prog_TotalFuseBytes : Prog_TotalLockBytes);
		
		eeprom_write_byte_169(&EEPROMAddress, CurrBuffByte); // CurrBuffByte stores the total number of fuse/lock bytes written in this case
	}
}

void PM_SendFuseLockBytes(uint8_t Type)
{
	uint16_t EEPROMAddress;
	uint8_t  TotalBytes;
	
	EEPROMAddress = ((Type == TYPE_FUSE)? Prog_TotalFuseBytes : Prog_TotalLockBytes);

	TotalBytes = eeprom_read_byte_169(&EEPROMAddress); // Get the total number of stored fuse/lock bytes
			
	EEPROMAddress = ((Type == TYPE_FUSE)? Prog_FuseBytes : Prog_LockBytes); // Set the EEPROM pointer to the fuse/lock bytes start (each fuse or lock byte takes four bytes in EEPROM)

	while (TotalBytes--)                              // Write each of the fuse/lock bytes stored in memory to the slave AVR
	{
		for (uint8_t CommandByte = 0; CommandByte < 4; CommandByte++)      // Write each individual command byte
		{
			USI_SPITransmit(eeprom_read_byte_169(&EEPROMAddress));
			EEPROMAddress++;
		}
		
		// Add some delay before programming next byte, if there is one:
		if (TotalBytes)
		   MAIN_Delay10MS(5);
	}
}

void PM_SendEraseCommand(void)
{
	uint16_t EEPROMAddress;

	EEPROMAddress = Prog_EraseChip + 2;               // Start of the erase commands
			
	for (uint8_t B = 0; B < 4 ; B++)                  // Read out the erase chip command bytes
	{
		USI_SPITransmit(eeprom_read_byte_169(&EEPROMAddress)); // Send the erase chip commands
		EEPROMAddress++;
	}
			
	EEPROMAddress = Prog_EraseChip + 1;               // Poll mode flag address
	if (eeprom_read_byte_169(&EEPROMAddress))         // Value of 1 indicates a busy flag test
	{
		do
			USI_SPITransmitWord(0xF000);
		while (USI_SPITransmitWord(0x0000) & 0x01);
	}
	else                                              // Cleared flag means use a predefined delay
	{
		EEPROMAddress = Prog_EraseChip;               // Delay value address			
		MAIN_Delay1MS(eeprom_read_byte_169(&EEPROMAddress)); // Wait the erase delay
	}
}

void PM_CreateProgrammingPackets(uint8_t Type)
{			
	uint32_t BytesRead       = 0;
	uint32_t BytesToRead     = PM_GetStoredDataSize(Type);      // Get the byte size of the stored program
	uint16_t BytesPerProgram;
	uint16_t EEPROMAddress;
	uint16_t PageLength;
	uint8_t  ContinuedPage   = FALSE;

	EEPROMAddress = ((Type == TYPE_FLASH)? Prog_PageLength : Prog_EPageLength);
	PageLength    = ((uint16_t)eeprom_read_byte_169(&EEPROMAddress) << 8);
	EEPROMAddress++;
	PageLength   |= (eeprom_read_byte_169(&EEPROMAddress));
	
	CurrAddress = 0;

	if (Type == TYPE_FLASH)
	{
		EEPROMAddress = Prog_WriteProgram;             // Set the EEPROM pointer to the write flash command bytes location
		DF_ContinuousReadEnable(0, 0);
		PacketBytes[0] = CMD_PROGRAM_FLASH_ISP;
	}
	else
	{
		EEPROMAddress = Prog_WriteEEPROM;              // Set the EEPROM pointer to the write EEPROM command bytes location
		DF_ContinuousReadEnable(PM_EEPROM_OFFSET / DF_INTERNALDF_BUFFBYTES, PM_EEPROM_OFFSET % DF_INTERNALDF_BUFFBYTES); // Start read from the EEPROM offset location
		PacketBytes[0] = CMD_PROGRAM_EEPROM_ISP;
	}

	for (uint8_t B = 1; B <= 9 ; B++)                 // Load in the write data command bytes
	{
		PacketBytes[B] = eeprom_read_byte_169(&EEPROMAddress); // Synthesise a write packet header
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

				BytesRead     += PageLength;                          // Increment the counter
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
					TempB = eeprom_read_byte_169(&Prog_TotalFuseBytes);
					MAIN_IntToStr(((TempB == 0xFF)? 0x00 : TempB), &Buffer[5]);
					break;
				case 3:
					strcpy_P(Buffer, PSTR("LOCK-"));
					TempB = eeprom_read_byte_169(&Prog_TotalLockBytes);
					MAIN_IntToStr(((TempB == 0xFF)? 0x00 : TempB), &Buffer[5]);		
			}
	
			LCD_puts(Buffer);

			MAIN_WaitForJoyRelease();
		}
	}
}
