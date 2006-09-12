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
	the SPI_SPITransmit macro.
*/

#include "Dataflash.h"

const char     DataFlashError[] PROGMEM = "DATAFLASH ERROR";
DFinfo         DataflashInfo            = {0, 0};

// ======================================================================================

uint8_t DF_CheckCorrectOnboardChip(void)          // Ensures onboard Butterfly dataflash is working and the correct type
{
	DF_TOGGLEENABLE();
	
	SPI_SPITransmit(DFCB_STATUSREG);              // Send the get status register command
	
	if (((SPI_SPITransmit(0x00) & DF_DFINFOMASK)) != (3 << 3)) // Bits 3, 4 and 5 contain the dataflash type info
	{
		DF_EnableDataflash(FALSE);
		SPI_SPIOFF();
		MAIN_ShowError(DataFlashError);
		
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void DF_WaitWhileBusy(void)
{
	DF_TOGGLEENABLE();
	
	SPI_SPITransmit(DFCB_STATUSREG);
	
	while (!(SPI_SPITransmit(0x00) & DF_BUSYMASK));
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

uint8_t DF_ReadBufferByte(const uint16_t BuffAddress)
{
	DF_TOGGLEENABLE();
	
	SPI_SPITransmit(DFCB_BUF1READ);
	SPI_SPITransmit((uint8_t)(BuffAddress >> 8));
	SPI_SPITransmit((uint8_t)(BuffAddress));
	SPI_SPITransmit(0x00);
	
	return SPI_SPITransmit(0x00);                         
}

void DF_EnableDataflash(const uint8_t Enabled)
{
	if (Enabled == TRUE)
	  PORTB &= ~(1 << 0);
	else
	  PORTB |= (1 << 0);
}
