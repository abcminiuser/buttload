/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
			  dean_camera@fourwalledcubicle.com
                  www.fourwalledcubicle.com
*/

#define  INC_FROM_VAMM
#include "VirtualAVRMemManager.h"

static volatile uint8_t  VAMMSetup                            = VAMM_SETUP_NA;
static          uint8_t  CurrPageCleared                      = FALSE;
                uint8_t  PageErasedFlags[DF_DATAFLASH_BLOCKS] = {};
                uint8_t  EraseFlagsTransReq                   = FALSE;

// ======================================================================================

/*
 NAME:      | VAMM_EnterStorageMode
 PURPOSE:   | Prepares abstraction layer when storage mode is entered for the managing of data
 ARGUMENTS: | None
 RETURNS:   | None
*/
void VAMM_EnterStorageMode(void)
{
	EraseFlagsTransReq = FALSE;
	VAMMSetup = VAMM_SETUP_NA;

	DF_ContinuousReadEnable(VAMM_PAGEERASED_DF_PAGE, 0); // Last dataflash page contains the erased page flag array

	for (uint16_t ByteNum = 0; ByteNum < sizeof(PageErasedFlags); ByteNum++)
	  PageErasedFlags[ByteNum] = SPI_SPITransmit(0x00);
}

/*
 NAME:      | VAMM_ExitStorageMode
 PURPOSE:   | Performs commands neccesary when storage mode is exited
 ARGUMENTS: | None
 RETURNS:   | None
*/
void VAMM_ExitStorageMode(void)
{
	VAMM_Cleanup();

	if (EraseFlagsTransReq)
	{
		DF_BufferWriteEnable(0);
		
		for (uint16_t ByteNum = 0; ByteNum < sizeof(PageErasedFlags); ByteNum++)
		  SPI_SPITransmit(PageErasedFlags[ByteNum]);

		for (uint16_t ByteNum = 0; ByteNum < (DF_INTERNALDF_BUFFBYTES - sizeof(PageErasedFlags)); ByteNum++)
		  SPI_SPITransmit(0xFF);                            // Fill remainder of buffer with 0xFFs so that compare can execute correctly

		if (DF_BufferCompare(VAMM_PAGEERASED_DF_PAGE) != DF_COMPARE_MATCH) // Compare so that write is only executed if page data is different
		  DF_CopyPage(VAMM_PAGEERASED_DF_PAGE, DF_BUFFER_TO_FLASH); // Last dataflash page contains the erased page flag array
	}	
}

/*
 NAME:      | VAMM_EraseAVRMemory
 PURPOSE:   | Erases the virtual AVR
 ARGUMENTS: | None
 RETURNS:   | None
*/
void VAMM_EraseAVRMemory(void)
{
	for (uint8_t PacketB = 1; PacketB < 7; PacketB++)       // Save the erase chip command bytes to EEPROM
	  eeprom_write_byte(&EEPROMVars.EraseChip[PacketB], PacketBytes[PacketB]);

	eeprom_write_byte(&EEPROMVars.EraseCmdStored, TRUE);

	EraseFlagsTransReq = TRUE;

	for (uint16_t CurrPageBlock = 0; CurrPageBlock < sizeof(PageErasedFlags); CurrPageBlock++)
	  PageErasedFlags[CurrPageBlock] = 0xFF;                // Set all page erased flags in each block
}

/*
 NAME:      | VAMM_SetAddress
 PURPOSE:   | Sets the memory address in the virtual AVR
 ARGUMENTS: | None
 RETURNS:   | None
*/
void VAMM_SetAddress(void)
{
	union
	{
		uint8_t  Bytes[4];
		uint32_t UnsignedLong;
	} StartAddress;

	StartAddress.UnsignedLong = CurrAddress;
	
	// Upper byte is for flags only (lower 24 bits contains the address). Clear the flag byte here:
	StartAddress.Bytes[3] = 0x00;

	VAMM_Cleanup();

	if (MemoryType == TYPE_FLASH)                           // Type 1 = Flash
	  StartAddress.UnsignedLong <<= 1;                      // Convert flash word address to byte address
	else                                                    // Type 2 = EEPROM
	  StartAddress.UnsignedLong  += SM_EEPROM_OFFSET;       // EEPROM uses byte addresses, and starts at the 257th kilobyte in Dataflash

	DataflashInfo.CurrPageAddress = (uint16_t)(StartAddress.UnsignedLong / DF_INTERNALDF_BUFFBYTES);
	DataflashInfo.CurrBuffByte    = (uint16_t)(StartAddress.UnsignedLong % DF_INTERNALDF_BUFFBYTES);

	VAMMSetup = VAMM_SETUP_ADDR_DONE;
}

/*
 NAME:      | VAMM_StoreByte
 PURPOSE:   | Stores a byte in the virtual AVR at the current address
 ARGUMENTS: | Byte to store
 RETURNS:   | None
*/
void VAMM_StoreByte(const uint8_t Data)
{
	if ((VAMMSetup != VAMM_SETUP_WRITE) || (DataflashInfo.CurrBuffByte == DF_INTERNALDF_BUFFBYTES)) // End of dataflash page or not set up for writing yet
	{
		VAMM_SetAddress();

		VAMM_CheckSetCurrPageCleared(VAMM_FLAG_CHECK);

		if (CurrPageCleared)                                // Page is erased, so clear the buffer
		{
			DF_BufferWriteEnable(0);
		
			for (uint16_t ClearByte = 0; ClearByte < DF_INTERNALDF_BUFFBYTES; ClearByte++) // Clear the Dataflash buffer
			  SPI_SPITransmit(0xFF);

			VAMM_CheckSetCurrPageCleared(VAMM_FLAG_CLEAR);
		}
		else
		{
			DF_CopyPage(DataflashInfo.CurrPageAddress, DF_FLASH_TO_BUFFER); // Page contains data already, copy it into the buffer
		}

		DF_BufferWriteEnable(DataflashInfo.CurrBuffByte);   // Set write point to the correct address in the current page

		VAMMSetup = VAMM_SETUP_WRITE;
	}

	if ((DataflashInfo.CurrBuffByte & 0x01) || (MemoryType == TYPE_EEPROM))
	  V2P_IncrementCurrAddress();

	DataflashInfo.CurrBuffByte++;

	SPI_SPITransmit(Data);                                  // Store the byte, dataflash is in write mode due to DF_BufferWriteEnable
}

/*
 NAME:      | VAMM_ReadByte
 PURPOSE:   | Reads a byte in the virtual AVR at the current address
 ARGUMENTS: | None
 RETURNS:   | Byte at current address within the virtual AVR's memory
*/
uint8_t VAMM_ReadByte(void)
{
	if ((VAMMSetup != VAMM_SETUP_READ) || (DataflashInfo.CurrBuffByte == DF_INTERNALDF_BUFFBYTES))
	{
		VAMM_SetAddress();

		VAMM_CheckSetCurrPageCleared(VAMM_FLAG_CHECK);
		DF_ContinuousReadEnable(DataflashInfo.CurrPageAddress, DataflashInfo.CurrBuffByte);
		
		VAMMSetup = VAMM_SETUP_READ;
	}

	if ((DataflashInfo.CurrBuffByte & 0x01) || (MemoryType == TYPE_EEPROM))
	  V2P_IncrementCurrAddress();

	DataflashInfo.CurrBuffByte++;

	return ((CurrPageCleared)? 0xFF : SPI_SPITransmit(0x00));
}

/*
 NAME:      | VAMM_ReadConsec
 PURPOSE:   | Reads bytes consecutively from the dataflash, without modifying CurrAddress
 ARGUMENTS: | None
 RETURNS:   | Byte at next address within the dataflash
*/
uint8_t VAMM_ReadConsec(void)
{
	if (VAMMSetup != VAMM_SETUP_READ)
	{
		VAMM_SetAddress();

		VAMM_CheckSetCurrPageCleared(VAMM_FLAG_CHECK);
		DF_ContinuousReadEnable(DataflashInfo.CurrPageAddress, DataflashInfo.CurrBuffByte);

		VAMMSetup = VAMM_SETUP_READ;
	}	

	if (DataflashInfo.CurrBuffByte == DF_INTERNALDF_BUFFBYTES)
	{
		DataflashInfo.CurrPageAddress++;
		DataflashInfo.CurrBuffByte = 0;

		VAMM_CheckSetCurrPageCleared(VAMM_FLAG_CHECK);
	}

	DataflashInfo.CurrBuffByte++;

	return ((CurrPageCleared)? 0xFF : SPI_SPITransmit(0x00));
}

/*
 NAME:      | VAMM_CheckSetCurrPageCleared
 PURPOSE:   | Checks or sets the flag corresponding to if the current dataflash page is empty
 ARGUMENTS: | Boolean flag; set if erased flag should be cleared
 RETURNS:   | Boolean flag; set if current dataflash page is cleared
*/
static void VAMM_CheckSetCurrPageCleared(const uint8_t ClearPageErasedFlag)
{
	uint8_t* BlockPointer = &PageErasedFlags[DataflashInfo.CurrPageAddress >> 3];
	uint8_t  BlockBit     = pgm_read_byte(&BitTable[DataflashInfo.CurrPageAddress & 0x07]);

	if (ClearPageErasedFlag)
	{
		*BlockPointer &= ~BlockBit;
		EraseFlagsTransReq = TRUE;
	}
	else
	{
		CurrPageCleared = (*BlockPointer & BlockBit);
	}
}

/*
 NAME:      | VAMM_Cleanup
 PURPOSE:   | Cleans up after data storage - writes any data in the dataflash buffer to the main dataflash memory
 ARGUMENTS: | None
 RETURNS:   | None
*/
static void VAMM_Cleanup(void)
{
	if (VAMMSetup == VAMM_SETUP_WRITE)                      // Save partially written page if in write mode
	{
		if (DF_BufferCompare(DataflashInfo.CurrPageAddress) != DF_COMPARE_MATCH) // Compare so that write is only executed if page data is different
		  DF_CopyPage(DataflashInfo.CurrPageAddress, DF_BUFFER_TO_FLASH);
	}

	VAMMSetup = VAMM_SETUP_NA;
}
