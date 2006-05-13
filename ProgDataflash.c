/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "ProgDataflash.h"

const uint8_t DataFlashProgMode[] PROGMEM = "*DATAFLASH MODE*";

// ======================================================================================

void PD_InterpretAVRISPPacket(void)
{
	uint8_t EraseDataflash = FALSE;

	switch (PacketBytes[0])
	{
		case AICB_CMD_ENTER_PROGMODE_ISP:
			MessageSize = 2;
			
			DF_EnableDataflash(TRUE);
			DF_GetChipCharacteristics();

			if (DataflashInfo.PageBits)
			{
				LCD_puts_f(DataFlashProgMode);
			
				InProgrammingMode = TRUE;                  // Set the flag, prevent the user from exiting the V2P state machine			
				MAIN_SETSTATUSLED(MAIN_STATLED_RED);
				PacketBytes[1] = AICB_STATUS_CMD_OK;
			}
			else
			{
				LCD_puts_f(DataFlashError);
			
				DF_EnableDataflash(FALSE);
				PacketBytes[1] = AICB_STATUS_CMD_FAILED;
			}
			
			break;			
		case AICB_CMD_LEAVE_PROGMODE_ISP:
			MessageSize = 2;

			InProgrammingMode = FALSE;                     // Clear the flag, allow the user to exit the V2P state machine

			DF_EnableDataflash(FALSE);
			MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);
			PacketBytes[1] = AICB_STATUS_CMD_OK;

			break;
		case AICB_CMD_READ_SIGNATURE_ISP:
			MessageSize = 4;

			PacketBytes[1] = AICB_STATUS_CMD_OK;           // Data byte is encased in CMD_OKs
			PacketBytes[2] = 0x02;                         // Signature bytes all return "02" in program dataflash mode
			PacketBytes[3] = AICB_STATUS_CMD_OK;           // Data byte is encased in CMD_OKs

			break;
		case AICB_CMD_CHIP_ERASE_ISP:
			MessageSize = 2;

			EraseDataflash = TRUE;
			PacketBytes[1] = AICB_STATUS_CMD_OK;
			
			break;
		case AICB_CMD_READ_OSCCAL_ISP:
		case AICB_CMD_READ_FUSE_ISP:
		case AICB_CMD_READ_LOCK_ISP:
			MessageSize = 4;

			PacketBytes[1] = AICB_STATUS_CMD_OK;          // Data byte is encased in CMD_OKs
			PacketBytes[2] = 0xFF;                        // Return 0xFFs for the bytes since they're not applicable
			PacketBytes[3] = AICB_STATUS_CMD_OK;          // Data byte is encased in CMD_OKs

			break;
		case AICB_CMD_PROGRAM_FUSE_ISP:
		case AICB_CMD_PROGRAM_LOCK_ISP:
			MessageSize = 3;
			
			PacketBytes[1] = AICB_STATUS_CMD_OK;          // Two CMD_OKs are always returned
			PacketBytes[2] = AICB_STATUS_CMD_OK;          // Two CMD_OKs are always returned

			break;
		case AICB_CMD_PROGRAM_FLASH_ISP:
			MessageSize = 2;

			PacketBytes[1] = AICB_STATUS_CMD_OK;
		
			break;
		case AICB_CMD_READ_FLASH_ISP:
			MessageSize = (((uint16_t)PacketBytes[1] << 8) | PacketBytes[2]) + 3;

			for (uint16_t DB = 1; DB < (MessageSize - 2); DB++)
			   PacketBytes[DB] = 0xFF;

			PacketBytes[1]               = AICB_STATUS_CMD_OK; // Return data should be encompassed in STATUS_CMD_OKs
			PacketBytes[MessageSize - 1] = AICB_STATUS_CMD_OK; // Return data should be encompassed in STATUS_CMD_OKs
				
			break;
		case AICB_CMD_PROGRAM_EEPROM_ISP:
			PD_SetupDFAddressCounters();
			
			DF_CopyFlashPageToBuffer(CurrPageAddress);
			DF_BufferWriteEnable(CurrBuffByte);
			
			uint16_t BytesToWrite = ((uint16_t)PacketBytes[1] << 8)
			                      | PacketBytes[2];

			for (uint16_t WriteByte = 0; WriteByte < BytesToWrite; WriteByte++)
			{
				PD_StoreDataflashByte(PacketBytes[10 + WriteByte]);
				CurrBuffByte++;
				V2P_IncrementCurrAddress();
			}

			PacketBytes[1] = AICB_STATUS_CMD_OK;
		
			break;
		case AICB_CMD_READ_EEPROM_ISP:
			PD_SetupDFAddressCounters();
			DF_CopyFlashPageToBuffer(CurrPageAddress);
			
			uint16_t BytesToRead = ((uint16_t)PacketBytes[1] << 8)
			                     | PacketBytes[2];
			
			for (uint16_t ReadByte = 0; ReadByte < BytesToRead; ReadByte++)
			{
				if (CurrBuffByte == DataflashInfo.PageSize)
				{
					PD_SetupDFAddressCounters();
					DF_CopyFlashPageToBuffer(CurrPageAddress);
				}
				
				PacketBytes[2 + ReadByte] = DF_ReadBufferByte(CurrBuffByte++); // Read in the next dataflash byte if present
				V2P_IncrementCurrAddress();
			}
			
			MessageSize = BytesToRead + 3;

			PacketBytes[1]               = AICB_STATUS_CMD_OK; // Return data should be encompassed in STATUS_CMD_OKs
			PacketBytes[2 + BytesToRead] = AICB_STATUS_CMD_OK; // Return data should be encompassed in STATUS_CMD_OKs
			
			break;
		default:
			MessageSize = 1;
			
			PacketBytes[1] = AICB_STATUS_CMD_UNKNOWN;
	}

	V2P_SendPacket();                                   // Send the response packet

	if (EraseDataflash)                                 // Very slow dataflash erasing must be done after replying to the message
	{
		LCD_puts_f(WaitText);		

		for (uint16_t BlockToErase = 0; BlockToErase < (DataflashInfo.TotalPages >> 3); BlockToErase++)
		   DF_EraseBlock(BlockToErase);
	
		LCD_puts_f(DataFlashProgMode);
	}
}

void PD_SetupDFAddressCounters(void)
{
	uint32_t StartAddress = CurrAddress;

	CurrPageAddress = 0;

	while (StartAddress > DataflashInfo.PageSize)      // This loop is the equivalent of a DIV and a MOD
	{
		StartAddress -= DataflashInfo.PageSize;         // Subtract one page's worth of bytes from the desired address
		CurrPageAddress++;
	}
	
	CurrBuffByte = (uint16_t)StartAddress;              // The buffer byte is the remainder
}

void PD_StoreDataflashByte(const uint8_t Data)
{
	if (CurrBuffByte == DataflashInfo.PageSize)
	{
		DF_CopyBufferToFlashPage(CurrPageAddress++);
		DF_BufferWriteEnable(0);
		CurrBuffByte = 0;
	}
	
	USI_SPITransmit(Data);                                 // Store the byte, dataflash is in write mode due to DF_BufferWriteEnable
	CurrBuffByte++;
}
