/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "AVRISPCommandInterpreter.h"

const char AVRISPModeMessage[] PROGMEM = "*ATAVRISP MODE*";

// ======================================================================================

void AICI_InterpretPacket(void)
{
	switch (PacketBytes[0])
	{
		case AICB_CMD_ENTER_PROGMODE_ISP:
			MessageSize = 2;
			
			USI_SPIInitMaster();
			ProgrammingFault = ISPCC_NO_FAULT;

			MAIN_ResetCSLine(MAIN_RESETCS_ACTIVE);     // Pull the slave AVR's RESET line to active
			ISPCC_EnterChipProgrammingMode();          // Run the Enter Programming Mode routine

			if (InProgrammingMode)
			  LCD_puts_f(AVRISPModeMessage);
			else
			  LCD_puts_f(SyncErrorMessage);

			break;
		case AICB_CMD_LEAVE_PROGMODE_ISP:
			MessageSize = 2;

			if (InProgrammingMode)
			  TG_PlayToneSeq((ProgrammingFault == ISPCC_NO_FAULT)? TONEGEN_SEQ_PROGDONE : TONEGEN_SEQ_PROGFAIL);

			MAIN_Delay1MS(PacketBytes[1]);             // Wait for the "PreDelay" amount specified in the packet
			InProgrammingMode = FALSE;
			MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE);   // Release the RESET line and allow the slave AVR to run
			MAIN_Delay1MS(PacketBytes[2]);             // Wait for the "PostDelay" amount specified in the packet
			
			USI_SPIOff();

			MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);     // Non programming mode = green status led
			PacketBytes[1] = AICB_STATUS_CMD_OK;       // Return OK

			break;
		case AICB_CMD_CHIP_ERASE_ISP:
			MessageSize = 2;
			
			for (uint8_t PacketB = 3; PacketB <= 6; PacketB++) // Send the erase commands to the slave AVR
			  USI_SPITransmit(PacketBytes[PacketB]);

			if (PacketBytes[2])                        // Poll mode, value of 1 indicates a busy flag wait
			{
				TCNT1  = 0;                            // Clear timer 1
				TCCR1B = ((1 << CS12) | (1 << CS10));  // Start timer 1 with a Fcpu/1024 clock

				do
				  USI_SPITransmitWord(0xF000);
				while ((USI_SPITransmitWord(0x0000) & 0x01) && (TCNT1 < ISPCC_COMM_TIMEOUT));

				TCCR1B = 0;                            // Stop timer 1

				PacketBytes[1] = ((TCNT1 < ISPCC_COMM_TIMEOUT)? AICB_STATUS_CMD_OK : AICB_STATUS_CMD_TOUT);
			}
			else                                       // Poll mode flag of 0 indicates a predefined delay
			{
				MAIN_Delay1MS(PacketBytes[1]);         // Wait the specified interval to ensure erase complete
				PacketBytes[1] = AICB_STATUS_CMD_OK;   // Always return OK
			}
						
			break;
		case AICB_CMD_SPI_MULTI:
			MessageSize = (3 + PacketBytes[2]);    // Number of recieved bytes, plus two OKs and the command byte
	
			uint8_t TxBytes      = PacketBytes[1]; // \. The packet data is overwritten during the transfer. Because
			uint8_t RxStartByte  = PacketBytes[2]; // |  of this each data byte must be stored into temp variables
			uint8_t RxBytes      = PacketBytes[3]; // /  so that their contents are not lost.
			uint8_t RxByteNum    = 0;
			uint8_t TxByteNum    = 0;
			uint8_t RecievedByte = 0;

			while (TxByteNum++ < TxBytes)          // Still bytes to transfer
			{
				RecievedByte = USI_SPITransmit(PacketBytes[3 + TxByteNum]); // Transmit the byte, store the answer

				if ((TxByteNum >= RxStartByte) && (RxByteNum < RxBytes))
				  PacketBytes[2 + RxByteNum++] = RecievedByte;
			}

			while (RxByteNum++ < RxBytes)                          // Still more bytes to recieve
			  PacketBytes[2 + RxByteNum] = USI_SPITransmit(0x00);  // its answer to be recorded (or more bytes than sent need responses), send dummy bytes to fetch the response(s)

			PacketBytes[1]             = AICB_STATUS_CMD_OK; // Data should be encompassed
			PacketBytes[3 + RxByteNum] = AICB_STATUS_CMD_OK; //  by STATS_CMD_OKs

			break;
		case AICB_CMD_READ_SIGNATURE_ISP:
		case AICB_CMD_READ_FUSE_ISP:
		case AICB_CMD_READ_LOCK_ISP:
		case AICB_CMD_READ_OSCCAL_ISP:
			MessageSize = 4;
	
			for (uint8_t ByteNum = 1; ByteNum <= 4; ByteNum++)
			{
				uint8_t Response = USI_SPITransmit(PacketBytes[1 + ByteNum]); // Transmit the four signature request bytes

				if (ByteNum == PacketBytes[1])         // If the current byte is the requested signature byte, save the reponse in the packet
				  PacketBytes[2] = Response;
			}

			PacketBytes[1] = AICB_STATUS_CMD_OK;       // Data byte is encased in CMD_OKs
			PacketBytes[3] = AICB_STATUS_CMD_OK;       // Data byte is encased in CMD_OKs

			break;
		case AICB_CMD_PROGRAM_FUSE_ISP:
		case AICB_CMD_PROGRAM_LOCK_ISP:
			MessageSize = 3;
			
			for (uint8_t PacketB = 1; PacketB <= 4; PacketB++) // Send the lock-byte values to the slave AVR
			  USI_SPITransmit(PacketBytes[PacketB]);

			PacketBytes[1] = AICB_STATUS_CMD_OK;       // Two CMD_OKs are always returned
			PacketBytes[2] = AICB_STATUS_CMD_OK;       // Two CMD_OKs are always returned

			break;
		case AICB_CMD_READ_FLASH_ISP:
		case AICB_CMD_READ_EEPROM_ISP:
			MessageSize = 0;                           // Here to prevent compiler from complaining if a var dec appears straight after a case

			uint8_t  ReadCommand = PacketBytes[3];
			uint16_t BytesToRead = ((uint16_t)PacketBytes[1] << 8) // Load in the number of bytes that is to
			                     | PacketBytes[2];                 // be read into a temp variable (MSB first)

			MessageSize = BytesToRead + 3;

			for (uint16_t ReadByte = 0; ReadByte < BytesToRead; ReadByte++)
			{
				if (PacketBytes[0] == AICB_CMD_READ_FLASH_ISP)  // Flash read mode - word addresses so MSB/LSB masking nessesary
				{
					USI_SPITransmit(ReadCommand | ((ReadByte & 0x01)? ISPCC_HIGH_BYTE_READ : ISPCC_LOW_BYTE_READ));
				}
				else                                   // EEPROM read mode, address is in bytes and so no masking nessesary
				{
					USI_SPITransmit(ReadCommand);
				}
				
				USI_SPITransmitWord(CurrAddress);      // Transmit the current address to the slave AVR

				PacketBytes[2 + ReadByte] = USI_SPITransmit(0x00); // Read in the byte stored at the requested location

				if ((ReadByte & 0x01) || (PacketBytes[0] == AICB_CMD_READ_EEPROM_ISP)) // Flash addresses are given in words; only increment on the odd byte if reading the flash.
				{
					V2P_IncrementCurrAddress();        // Increment the address counter
				}
				else
				{
					if ((CurrAddress & 0x00FF0000) && !(CurrAddress & 0x0000FFFF))
					{
						CurrAddress |= (1UL << 31);    // Set MSB set of the address, indicates a LOAD_EXTENDED_ADDRESS must be executed
						V2P_CheckForExtendedAddress();
					}
				}
			}
			
			PacketBytes[1]               = AICB_STATUS_CMD_OK; // Return data should be encompassed in STATUS_CMD_OKs
			PacketBytes[2 + BytesToRead] = AICB_STATUS_CMD_OK; // Return data should be encompassed in STATUS_CMD_OKs

			break;
		case AICB_CMD_PROGRAM_FLASH_ISP:
		case AICB_CMD_PROGRAM_EEPROM_ISP:
			ISPCC_ProgramChip();                       // Program the bytes into the chip
			
			MessageSize = 2;

			PacketBytes[1] = ((ProgrammingFault == ISPCC_NO_FAULT) ? AICB_STATUS_CMD_OK : AICB_STATUS_CMD_TOUT);
			
			break;
		default:                                       // Unknown command, return error
			MessageSize = 2;
			
			PacketBytes[1] = AICB_STATUS_CMD_UNKNOWN;
	}

	V2P_SendPacket();                                  // Send the response packet
}
