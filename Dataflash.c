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

#define  INC_FROM_DF
#include "Dataflash.h"

const char     DataFlashError[] PROGMEM = "DATAFLASH ERROR";
DFinfo         DataflashInfo            = {CurrPageAddress: 0, CurrBuffByte: 0};

// ======================================================================================

uint8_t DF_CheckCorrectOnboardChip(void)          // Ensures onboard Butterfly dataflash is working and the correct type
{
	DF_TOGGLEENABLE();
	
	SPI_SPITransmit(DFCB_STATUSREG);              // Send the get status register command
	
	if (((SPI_SPITransmit(0x00) & DF_DFINFOMASK)) != (3 << 3)) // Bits 3, 4 and 5 contain the dataflash type info
	{
		DF_ENABLEDATAFLASH(FALSE);
		SPI_SPIOFF();
		MAIN_ShowError(DataFlashError);
		
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void DF_CopyBufferToFlashPage(const uint16_t PageAddress)
{
	DF_TOGGLEENABLE();
	
	SPI_SPITransmit(DFCB_BUF1TOFLASHWE);
	SPI_SPITransmit((uint8_t)(PageAddress >> DF_PAGESHIFT_HIGH));
	SPI_SPITransmit((uint8_t)(PageAddress << DF_PAGESHIFT_LOW));
	SPI_SPITransmit(0x00);
		
	DF_WaitWhileBusy();
}

void DF_CopyFlashPageToBuffer(const uint16_t PageAddress)
{
	DF_TOGGLEENABLE();

	SPI_SPITransmit(DFCB_FLASHTOBUF1TRANSFER);
	SPI_SPITransmit((uint8_t)(PageAddress >> DF_PAGESHIFT_HIGH));
	SPI_SPITransmit((uint8_t)(PageAddress << DF_PAGESHIFT_LOW));
	SPI_SPITransmit(0x00);
	
	DF_WaitWhileBusy();
}

void DF_BufferWriteEnable(const uint16_t BuffAddress)
{
	DF_TOGGLEENABLE();

	SPI_SPITransmit(DFCB_BUF1WRITE);
	SPI_SPITransmit(0x00);
	SPI_SPITransmit((uint8_t)(BuffAddress >> 8));
	SPI_SPITransmit((uint8_t)(BuffAddress));
}

void DF_ContinuousReadEnable(const uint16_t PageAddress, const uint16_t BuffAddress)
{
	DF_TOGGLEENABLE();
	
	SPI_SPITransmit(DFCB_CONTARRAYREAD);
	SPI_SPITransmit((uint8_t)(PageAddress >> DF_PAGESHIFT_HIGH));
	SPI_SPITransmit((uint8_t)((PageAddress << DF_PAGESHIFT_LOW) + (BuffAddress >> 8)));
	SPI_SPITransmit((uint8_t)(BuffAddress));
	
	for (uint8_t DByte = 0; DByte < 4; DByte++)  // Perform 4 dummy writes to intiate the DataFlash address pointers
	  SPI_SPITransmit(0x00);
}

void DF_WaitWhileBusy(void)
{
	DF_TOGGLEENABLE();
	
	SPI_SPITransmit(DFCB_STATUSREG);
	
	while (!(SPI_SPITransmit(0x00) & DF_BUSYMASK));
}
