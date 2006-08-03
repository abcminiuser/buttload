/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

/*
	Compatible with most Atmel dataflash memory devices. This is a re-written, bare-bones
	version of the generic Atmel Dataflash driver, with some specific ButtLoad routines
	added in.
	
	This uses a lot of abstraction to perform the dataflash tasks. A global pointer to a
	routine for sending the SPI bytes is kept in DFSPIRoutinePointer which may be changed
	by the program to redirect the output (in ButtLoad this is changed between the SPI bus
	and the USI bus which is set to SPI mode. This global pointer is then copied to a local
	pointer by DF_MAKELOCALSPIFUNCPTR() to prevent it from being re-copied before every use
	of SPI_SENDBYTE. As a consequence this macro must be in all routines which make use of
	the DF_SENDSPIBYTE macro.
*/

#include "Dataflash.h"

//                   DataFlash Size:       512k,   1M,   2M,   4M,   8M,  16M,  32M,  64M
const uint8_t  DF_PageBits[]   PROGMEM = {    9,    9,    9,    9,    9,   10,   10,   11}; // Index of internal page address bits
const uint16_t DF_PageSize[]   PROGMEM = {  264,  264,  264,  264,  264,  528,  528, 1056}; // Index of page sizes
const uint16_t DF_Pages[]      PROGMEM = {  256,  512, 1024, 2048, 4096, 4096, 8192, 8192}; // Index of total pages

const uint8_t DataFlashError[] PROGMEM = "DATAFLASH ERROR";

DFinfo     DataflashInfo       = {0,0,0,0,0,0,SPI_SPITransmit};

// ======================================================================================

uint8_t DF_CheckCorrectOnboardChip(void)         // Ensures onboard Butterfly dataflash is working and the correct type
{
	DF_GetChipCharacteristics();

	if (DataflashInfo.TotalPages != 2048)
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

void DF_GetChipCharacteristics(void)
{
	uint8_t DataIndex;
	
	DF_MAKELOCALSPIFUNCPTR();

	DF_TOGGLEENABLE();
	
	DF_SENDSPIBYTE(DFCB_STATUSREG);               // Send the get status register command
	
	DataIndex     = ((DF_SENDSPIBYTE(0x00) & 0x38) >> 3); // Bits 3, 4 and 5 contain the lookup table index

	DataflashInfo.PageBits   = pgm_read_byte(&DF_PageBits[DataIndex]);	// Get number of internal page address bits from look-up table
	DataflashInfo.PageSize   = pgm_read_word(&DF_PageSize[DataIndex]);  // Get the size of the page (in bytes)
	DataflashInfo.TotalPages = pgm_read_word(&DF_Pages[DataIndex]);     // Get the total number of pages in the connected dataflash

	PageShiftHigh = (16 - DataflashInfo.PageBits);
	PageShiftLow  = (DataflashInfo.PageBits - 8);
}

void DF_WaitWhileBusy(void)
{
	DF_MAKELOCALSPIFUNCPTR();

	DF_TOGGLEENABLE();
	
	DF_SENDSPIBYTE(DFCB_STATUSREG);               // Send the get status register command
	
	while (!(DF_SENDSPIBYTE(0x00) & DF_BUSYMASK));
}

void DF_CopyBufferToFlashPage(const uint16_t PageAddress)
{
	DF_MAKELOCALSPIFUNCPTR();

	DF_TOGGLEENABLE();

	DF_SENDSPIBYTE(DFCB_BUF1TOFLASHWE);           // Send the buffer copy command code
	DF_SENDSPIBYTE((uint8_t)(PageAddress >> PageShiftHigh)); // Send the upper part of the page address
	DF_SENDSPIBYTE((uint8_t)(PageAddress << PageShiftLow));  // Send the lower part of the page address
	DF_SENDSPIBYTE(0x00);                         // Send a dummy byte	
	
	DF_WaitWhileBusy();
}

void DF_CopyFlashPageToBuffer(const uint16_t PageAddress)
{
	DF_MAKELOCALSPIFUNCPTR();

	DF_TOGGLEENABLE();

	DF_SENDSPIBYTE(DFCB_FLASHTOBUF1TRANSFER);     // Send the memory copy command code
	DF_SENDSPIBYTE((uint8_t)(PageAddress >> PageShiftHigh)); // Send the upper part of the page address
	DF_SENDSPIBYTE((uint8_t)(PageAddress << PageShiftLow));  // Send the lower part of the page address
	DF_SENDSPIBYTE(0x00);                         // Send a dummy byte	
	
	DF_WaitWhileBusy();
}

void DF_BufferWriteEnable(const uint16_t BuffAddress)
{
	DF_MAKELOCALSPIFUNCPTR();

	DF_TOGGLEENABLE();

	DF_SENDSPIBYTE(DFCB_BUF1WRITE);               // Send the buffer write command code
	DF_SENDSPIBYTE(0x00);                         // Send a dummy byte
	DF_SENDSPIBYTE((uint8_t)(BuffAddress >> 8));  // Send the buffer high address	
	DF_SENDSPIBYTE((uint8_t)(BuffAddress));       // Send the buffer low address	
}

void DF_ContinuousReadEnable(const uint16_t PageAddress, const uint16_t BuffAddress)
{
	DF_MAKELOCALSPIFUNCPTR();

	DF_TOGGLEENABLE();
	
	DF_SENDSPIBYTE(DFCB_CONTARRAYREAD);
	DF_SENDSPIBYTE((uint8_t)(PageAddress >> PageShiftHigh));
	DF_SENDSPIBYTE((uint8_t)((PageAddress << PageShiftLow) + (BuffAddress >> 8)));
	DF_SENDSPIBYTE((uint8_t)(BuffAddress));
	
	for (uint8_t DByte = 0; DByte < 4; DByte++)  // Perform 4 dummy writes to intiate the DataFlash address pointers
	   DF_SENDSPIBYTE(0x00);                         
}

uint8_t DF_ReadBufferByte(const uint16_t BuffAddress)
{
	DF_MAKELOCALSPIFUNCPTR();

	DF_TOGGLEENABLE();
	
	DF_SENDSPIBYTE(DFCB_BUF1READ);
	DF_SENDSPIBYTE((uint8_t)(BuffAddress >> 8));
	DF_SENDSPIBYTE((uint8_t)(BuffAddress));
	DF_SENDSPIBYTE(0x00);
	
	return DF_SENDSPIBYTE(0x00);                         
}

void DF_EraseBlock(const uint16_t BlockToErase)
{
	DF_MAKELOCALSPIFUNCPTR();

	DF_TOGGLEENABLE();

	DF_SENDSPIBYTE(DFCB_BLOCKERASE);                   // Send block erase command
	DF_SENDSPIBYTE((uint8_t)(BlockToErase >> 8));
	DF_SENDSPIBYTE((uint8_t)(BlockToErase));
	DF_SENDSPIBYTE(0x00);

	DF_WaitWhileBusy();
}

void DF_EnableDataflash(const uint8_t Enabled)
{
	if (Enabled == TRUE)
	{
		if (DataflashInfo.UseExernalDF == TRUE)
		  MAIN_ResetCSLine(MAIN_RESETCS_EXTDFACTIVE);
		else
		  PORTB &= ~(1 << 0);
	}
	else
	{
		if (DataflashInfo.UseExernalDF == TRUE)
		  MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE);
		else
		  PORTB |= (1 << 0);
	}
}
