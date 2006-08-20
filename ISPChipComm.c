/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "ISPChipComm.h"

const uint8_t SignonData[]       PROGMEM = {0x40, 0x28, 0x23, 0x29, 0x2A, 0x53, 0x43, 0x52, 0x45, 0x57, 0x20, 0x52, 0x45, 0x54, 0x52, 0x4f, 0x44, 0x41, 0x4e, 0x2A, 0x00};

const uint8_t SyncErrorMessage[] PROGMEM = "SYNC ERROR";

// ======================================================================================

void ISPCC_EnterChipProgrammingMode(void)
{
	uint8_t ByteDelay = PacketBytes[5];
	uint8_t Attempts  = PacketBytes[4];
	uint8_t Response;

	MAIN_SETSTATUSLED(MAIN_STATLED_ORANGE); // Orange = Busy

	MAIN_Delay1MS(PacketBytes[2]);          // Wait before continuing, amount specified in the packet

	if ((!(Attempts)) || (Attempts > 100))  // If no attempts or too high a value is specified, a fixed number is chosen
	   Attempts = 24;
		
	while (Attempts--)
	{
		USI_SPITransmit(PacketBytes[8]);
		MAIN_Delay1MS(ByteDelay);
		USI_SPITransmit(PacketBytes[9]);
		MAIN_Delay1MS(ByteDelay);
			
		Response = USI_SPITransmit(PacketBytes[10]);
		MAIN_Delay1MS(ByteDelay);

		if (PacketBytes[7] == ISPCC_POLL_MODE_AVR)
		  USI_SPITransmit(PacketBytes[11]);
		else
		  Response = USI_SPITransmit(PacketBytes[11]);
				
		if (!(PacketBytes[7]) || (Response == PacketBytes[6])) // Polling disabled, or returned value matches expected poll value
		{
			MAIN_Delay1MS(ByteDelay);

			InProgrammingMode = TRUE;
			MAIN_SETSTATUSLED(MAIN_STATLED_RED);
			PacketBytes[1] = AICB_STATUS_CMD_OK;
			return;
		}
		
		MAIN_Delay1MS(ByteDelay);
		USI_SPIToggleClock();               // Out of sync, shift in one bit and try again
	}

	// If function hasn't returned by now, all the attempts have failed. Show this by
	// resetting the status leds to green (ready) and send a fail message.

	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);
	PacketBytes[1] = AICB_STATUS_CMD_FAILED;
}

void ISPCC_ProgramChip(void)
{
	uint16_t PollAddress  = 0;
	uint8_t  ProgMode     = PacketBytes[3];
	uint8_t  WriteCommand = PacketBytes[5];
	uint16_t StartAddress = (uint16_t)CurrAddress;
	uint16_t BytesToWrite = ((uint16_t)PacketBytes[1] << 8)
	                      | PacketBytes[2];
	uint8_t  CmdMemType   = PacketBytes[0];  
	uint8_t  PollType;
	uint8_t  ByteToWrite;

	if (ProgMode & ISPCC_PROG_MODE_PAGE)                 // Page writing mode
	{
		for (uint16_t WriteByte = 0; WriteByte < BytesToWrite; WriteByte++) // Transmit the page bytes
		{
			ByteToWrite = PacketBytes[10 + WriteByte];
		
			if (CmdMemType == AICB_CMD_PROGRAM_FLASH_ISP) // Flash write mode - word addresses so MSB/LSB masking 
			  USI_SPITransmit(WriteCommand | ((WriteByte & 0x01)? ISPCC_HIGH_BYTE_WRITE : ISPCC_LOW_BYTE_WRITE));
			else                                          // EEPROM write mode - byte addresses so no masking 
			  USI_SPITransmit(WriteCommand);

			USI_SPITransmitWord(CurrAddress & 0xFFFF);    // Only the LSW of the address should be sent
			USI_SPITransmit(ByteToWrite);                 // Send one of the new bytes to be written

			if (!(PollAddress))
			{
				if ((PacketBytes[8] != ByteToWrite)       // Can do polling
				   && ((CmdMemType == AICB_CMD_PROGRAM_FLASH_ISP) || ((CmdMemType == AICB_CMD_PROGRAM_EEPROM_ISP) && (PacketBytes[9] != ByteToWrite))))
				{
					PollAddress = (CurrAddress & 0xFFFF); // Save the current address
				
					if (CmdMemType == AICB_CMD_PROGRAM_FLASH_ISP)
					  PollAddress = ((PollAddress << 1) + (WriteByte & 0x01));
				}
			}

			// Flash addresses are in words; only increment address on odd byte, OR if it's the EEPROM being programmed (byte addresses)
			if ((WriteByte & 0x01) || (CmdMemType == AICB_CMD_PROGRAM_EEPROM_ISP))
			  V2P_IncrementCurrAddress();
		}

		PollType = ProgMode;

		if (ProgMode & ISPCC_PROG_MODE_PAGEDONE)         // If this packet is the end of a page, we need to send the program page command
		{
			USI_SPITransmit(PacketBytes[6]);             // Send the write program memory page command
			USI_SPITransmitWord(StartAddress);           // Send the page address word
			USI_SPITransmit(0x00);

			if (!(PollAddress))                          // No polling address
			  PollType = ((ProgMode & ~ISPCC_PAGE_POLLTYPE_MASK) | ISPCC_PAGE_POLLTYPE_WAIT);

			ISPCC_PollForProgComplete(PollType, PollAddress);
		}
	}
	else                                                 // Flash Word writing mode or EEPROM byte writing mode
	{
		for (uint16_t WriteByte = 0; WriteByte < BytesToWrite; WriteByte++)
		{
			ByteToWrite = PacketBytes[10 + WriteByte];

			if (CmdMemType == AICB_CMD_PROGRAM_FLASH_ISP)
			  USI_SPITransmit(WriteCommand | ((WriteByte & 0x01)? ISPCC_HIGH_BYTE_WRITE : ISPCC_LOW_BYTE_WRITE));
			else
			  USI_SPITransmit(WriteCommand);					
					
			USI_SPITransmitWord(CurrAddress & 0xFFFF);    // Transmit the current address to the slave AVR
			USI_SPITransmit(ByteToWrite);                 // Send one of the new bytes to be written

			PollType = ProgMode;

			if ((PacketBytes[8] != ByteToWrite)           // Can do polling
			   && ((CmdMemType == AICB_CMD_PROGRAM_FLASH_ISP) || ((CmdMemType == AICB_CMD_PROGRAM_EEPROM_ISP) && (PacketBytes[9] != ByteToWrite))))
			{
				PollAddress = (CurrAddress & 0xFFFF);     // Save the current address;

				if (CmdMemType == AICB_CMD_PROGRAM_FLASH_ISP)
				  PollAddress = ((PollAddress << 1) + (WriteByte & 0x01));
			}
			else
			{
				PollType = ((ProgMode & ~ISPCC_WORD_POLLTYPE_MASK) | ISPCC_WORD_POLLTYPE_WAIT);
			}					

			// Flash addresses are in words; only increment address on the odd byte, OR if it's the EEPROM being programmed (byte addresses)
			if ((WriteByte & 0x01) || (CmdMemType == AICB_CMD_PROGRAM_EEPROM_ISP))
			  V2P_IncrementCurrAddress();

			ISPCC_PollForProgComplete(PollType, PollAddress);
		}
	}
}

void ISPCC_PollForProgComplete(const uint8_t PollData, uint16_t PollAddr)
{
	uint8_t PollType;
	uint8_t ProgCommand;

	if (PollData & ISPCC_PROG_MODE_PAGE)
	  PollType = ((PollData & ISPCC_PAGE_POLLTYPE_MASK) >> ISPCC_PAGE_POLLTYPE_MASKSHIFT);
	else
	  PollType = ((PollData & ISPCC_WORD_POLLTYPE_MASK) >> ISPCC_WORD_POLLTYPE_MASKSHIFT);	

	switch (PollType & ISPCC_POLLTYPE_MASK)
	{
		case ISPCC_POLLTYPE_DATA:
			ProgCommand = PacketBytes[7];
			
			if (PacketBytes[0] == AICB_CMD_PROGRAM_FLASH_ISP) // Flash uses word addresses
			{
				ProgCommand  |= ((PollAddr & 0x01)? ISPCC_HIGH_BYTE_READ : ISPCC_LOW_BYTE_READ);
				PollAddr    >>= 1;
			}

			do
			{
				USI_SPITransmit(ProgCommand);
				USI_SPITransmitWord(PollAddr);
			}
			while (USI_SPITransmit(0x00) == PacketBytes[8]);
						
			break;
		case ISPCC_POLLTYPE_READY:
			do
			  USI_SPITransmitWord(0xF000);
			while (USI_SPITransmitWord(0x0000) & ISPCC_POLL_BUSYFLAG);

			break;
		default:                                      // Default is Wait polling
			MAIN_Delay1MS(PacketBytes[4]);	
	}
}
