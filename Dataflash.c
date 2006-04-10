/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

/*
	Compatible with most Atmel dataflash memory devices. This is a re-written, bare-bones
	version of the generic Atmel Dataflash driver, with some specific ButtLoad routines
	added in.
*/

#include "Dataflash.h"

// Look-up table for these sizes ->     512k, 1M, 2M, 4M, 8M, 16M, 32M, 64M
const uint8_t DF_PageBits[]  PROGMEM = {  9,  9,  9,  9,  9,  10,  10,  11};             // Index of internal page address bits

// Look-up table for these sizes ->     512k,  1M,  2M,  4M,  8M, 16M, 32M,  64M
const uint16_t DF_PageSize[] PROGMEM = {264, 264, 264, 264, 264, 528, 528, 1056};        // Index of page sizes

// Look-up table for these sizes ->     512k,   1M,   2M,   4M,   8M,  16M,  32M,  64M
const uint16_t DF_Pages[]    PROGMEM = { 256, 512, 1024, 2048, 4096, 4096, 8192, 8192};  // Index of total pages

const uint8_t DataFlashError[] PROGMEM = "DATAFLASH ERROR";

SPIFuncPtr DFSPIRoutinePointer = SPI_SPITransmit;
DFinfo     DataflashInfo;
uint16_t   CurrPageAddress;
uint16_t   CurrBuffByte;
uint8_t    UseExernalDF;

// ======================================================================================

uint8_t DF_CheckCorrectOnboardChip(void) // Ensures onboard Butterfly dataflash is working and the correct type
{
	DF_GetChipCharacteristics();

	if (DataflashInfo.TotalPages == 2048)
	{
		SPI_SPIOFF();
		MAIN_ShowError(DataFlashError);
		
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

uint8_t DF_GetChipCharacteristics(void)
{
	uint8_t StatusBits;
	uint8_t DataIndex;
	
	DF_TOGGLEENABLE();
	
	DF_SENDSPIBYTE(StatusReg);                    // Send the get status register command
	
	StatusBits = DF_SENDSPIBYTE(0x00);
	DataIndex  = ((StatusBits & 0x38) >> 3);

	DataflashInfo.PageBits   = pgm_read_byte(&DF_PageBits[DataIndex]);	// Get number of internal page address bits from look-up table
	DataflashInfo.PageSize   = pgm_read_word(&DF_PageSize[DataIndex]);  // Get the size of the page (in bytes)
	DataflashInfo.TotalPages = pgm_read_word(&DF_Pages[DataIndex]);     // Get the total number of pages in the connected dataflash

	PageShiftHigh = (16 - DataflashInfo.PageBits);
	PageShiftLow  = (DataflashInfo.PageBits - 8);
	
	return StatusBits;
}

void DF_CopyBufferToFlashPage(uint16_t PageAddress)
{
	DF_TOGGLEENABLE();

	DF_SENDSPIBYTE(Buf1ToFlashWE);                // Send the buffer copy command code
	DF_SENDSPIBYTE((uint8_t)(PageAddress >> PageShiftHigh)); // Send the upper part of the page address
	DF_SENDSPIBYTE((uint8_t)(PageAddress << PageShiftLow));  // Send the lower part of the page address
	DF_SENDSPIBYTE(0x00);                         // Send a dummy byte	
	
	while (DF_BUSY());	
}

void DF_CopyFlashPageToBuffer(uint16_t PageAddress)
{
	DF_TOGGLEENABLE();

	DF_SENDSPIBYTE(FlashToBuf1Transfer);          // Send the memory copy command code
	DF_SENDSPIBYTE((uint8_t)(PageAddress >> PageShiftHigh)); // Send the upper part of the page address
	DF_SENDSPIBYTE((uint8_t)(PageAddress << PageShiftLow));  // Send the lower part of the page address
	DF_SENDSPIBYTE(0x00);                         // Send a dummy byte	
	
	while (DF_BUSY());	
}

void DF_BufferWriteEnable(uint16_t BuffAddress)
{
	DF_TOGGLEENABLE();

	DF_SENDSPIBYTE(Buf1Write);                    // Send the buffer write command code
	DF_SENDSPIBYTE(0x00);                         // Send a dummy byte
	DF_SENDSPIBYTE((uint8_t)(BuffAddress >> 8));  // Send the buffer high address	
	DF_SENDSPIBYTE((uint8_t)(BuffAddress));       // Send the buffer low address	
}

void DF_ContinuousReadEnable(uint16_t PageAddress, uint16_t BuffAddress)
{
	DF_TOGGLEENABLE();
	
	DF_SENDSPIBYTE(ContArrayRead);
	DF_SENDSPIBYTE((uint8_t)(PageAddress >> PageShiftHigh));
	DF_SENDSPIBYTE((uint8_t)((PageAddress << PageShiftLow) + (BuffAddress >> 8)));
	DF_SENDSPIBYTE((uint8_t)(BuffAddress));
	
	for (uint8_t DByte = 0; DByte < 4; DByte++)  // Perform 4 dummy writes to intiate the DataFlash address pointers
	   DF_SENDSPIBYTE(0x00);                         
}

uint8_t DF_ReadBufferByte(uint16_t BuffAddress)
{
	DF_TOGGLEENABLE();
	
	DF_SENDSPIBYTE(Buf1Read);
	DF_SENDSPIBYTE((uint8_t)(BuffAddress >> 8));
	DF_SENDSPIBYTE((uint8_t)(BuffAddress));
	DF_SENDSPIBYTE(0x00);
	
	return DF_SENDSPIBYTE(0x00);                         
}

void DF_EraseBlock(uint16_t BlockToErase)
{
	DF_TOGGLEENABLE();

	DF_SENDSPIBYTE(BlockErase);                   // Send block erase command
	DF_SENDSPIBYTE((uint8_t)(BlockToErase >> 8));
	DF_SENDSPIBYTE((uint8_t)(BlockToErase));
	DF_SENDSPIBYTE(0x00);

	while (DF_BUSY());
}

void DF_EnableDataflash(uint8_t Enabled)
{
	if (Enabled == TRUE)
	{
		if (UseExernalDF == TRUE)
		  MAIN_ResetCSLine(MAIN_RESETCS_EXTDFACTIVE);
		else
		  PORTB &= ~(1 << 0);
	}
	else
	{
		if (UseExernalDF == TRUE)
		  MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE);
		else
		  PORTB |= (1 << 0);
	}
}
