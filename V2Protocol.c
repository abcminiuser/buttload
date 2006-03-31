/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#include "V2Protocol.h"

// PROGMEM CONSTANTS:
const uint8_t VersionData[]        PROGMEM   = {HW_VERSION, SW_VERSION_MAJOR, SW_VERSION_MINOR};
const uint8_t SignonResponse[11]   PROGMEM   = {CMD_SIGN_ON, STATUS_CMD_OK, 8, 'A', 'V', 'R', 'I', 'S', 'P', '_', '2'};

// GLOBAL VARIABLES:
FuncPtr  InterpretPacketRoutine         = AICI_InterpretPacket;

uint8_t  PacketBytes[V2P_MAXBUFFSIZE]   = {};
uint16_t SequenceNum                    = 0;
uint16_t MessageSize                    = 0;
uint8_t  InProgrammingMode              = FALSE;
uint32_t CurrAddress                    = 0;

uint8_t  Param_ControllerInit           = 0; // This is set to zero on reset, and can be written to or read by the computer

// ======================================================================================

void V2P_RunStateMachine(void)
{
	uint8_t  V2PState            = V2P_STATE_IDLE;
	uint16_t CurrentMessageByte  = 0;

	USART_ENABLE(USART_TX_ON, USART_RX_ON);
	BUFF_InitialiseBuffer();

	TIMEOUT_SetupTimeoutTimer();

	InProgrammingMode = FALSE;
	CurrAddress       = 0;

	while (1)
	{
		if (TimeOut == TRUE)        // Packet has timed out waiting for data
		   V2PState = V2P_STATE_PACKERR;
		else if (V2PState != V2P_STATE_IDLE)
		   TIMEOUT_TIMER_ON();      // Reset the timer on each loop if not in idle mode
		
		switch (V2PState)
		{
			case V2P_STATE_PACKERR:
				MessageSize    = 2;
				PacketBytes[1] = STATUS_CMD_FAILED;
				TimeOut = FALSE;
				V2P_SendPacket();
				// Note - PACKERR falls through to PACKOK in order to clear the buffer
			case V2P_STATE_PACKOK:
				BUFF_InitialiseBuffer();          // Flush the ringbuffer
				TIMEOUT_TIMER_OFF();
				V2PState  = V2P_STATE_IDLE;
				
				break;
			case V2P_STATE_IDLE:	
				if (BuffElements)                 // Serial data recieved in FIFO buffer
				   V2PState  = V2P_STATE_START;
				
				if ((JoyStatus & JOY_LEFT) && !(InProgrammingMode))
				{
					USART_ENABLE(USART_TX_OFF, USART_RX_OFF);
					return;
				}
								
				break;
			case V2P_STATE_START:
				if (USART_Rx() == MESSAGE_START)  // Start bit is always 0x1B
					V2PState  = V2P_STATE_GETSEQUENCENUM;
				else
					V2PState  = V2P_STATE_PACKERR;
				
				break;
			case V2P_STATE_GETSEQUENCENUM:
				SequenceNum = USART_Rx();

				V2PState  = V2P_STATE_GETMESSAGESIZE1;

				break;
			case V2P_STATE_GETMESSAGESIZE1:
				MessageSize  = ((uint16_t)USART_Rx() << 8);  // Message size is MSB first
				V2PState     = V2P_STATE_GETMESSAGESIZE2;
				
				break;
			case V2P_STATE_GETMESSAGESIZE2:
				MessageSize |= USART_Rx();         // Get the second byte of the message size				

				if (MessageSize < V2P_MAXBUFFSIZE) // Safety; only continue if packet size is less than buffer size
				{	
					V2PState = V2P_STATE_GETTOKEN;
					CurrentMessageByte = 0;
				}
				else
				{
					V2PState = V2P_STATE_PACKERR;
				}
	
				break;
			case V2P_STATE_GETTOKEN:
				if (USART_Rx() == TOKEN)           // Token bit is always 0x0E
					V2PState  = V2P_STATE_GETDATA;
				else                               // Incorrect token bit
					V2PState  = V2P_STATE_PACKERR;

				break;
			case V2P_STATE_GETDATA:
				if (CurrentMessageByte == MessageSize) // Packet reception complete
					V2PState  = V2P_STATE_GETCHECKSUM;
				else
					PacketBytes[CurrentMessageByte++] = USART_Rx(); // Store the byte
	
				break;
			case V2P_STATE_GETCHECKSUM:
				if (V2P_GetChecksum() == USART_Rx())   // If checksum is ok, process the packet
				{
					switch (PacketBytes[0])            // \/ Look for generic commands which can be interpreted, 
					{                                   //  \ otherwise run the custom interpret routine
						case CMD_SIGN_ON:
							MessageSize = 11;

							for (uint8_t SOByte = 0; SOByte < 11; SOByte++) // Load the sign-on sequence from program memory
							   PacketBytes[SOByte] = pgm_read_byte(&SignonResponse[SOByte]);

							V2P_SendPacket();
							break;
						case CMD_FIRMWARE_UPGRADE:
							MessageSize = 2;
			
							PacketBytes[1] = STATUS_CMD_FAILED;  // Return failed (no automatic firmware upgrades)
			
							V2P_SendPacket();
							break;				
						case CMD_LOAD_ADDRESS:
							MessageSize  = 2;
			
							V2P_CheckForExtendedAddress();

							CurrAddress  = ((uint32_t)PacketBytes[1] << 24)
							             | ((uint32_t)PacketBytes[2] << 16)
							             | ((uint32_t)PacketBytes[3] << 8)
							             | PacketBytes[4];
										 
							if (InPMMode)
							{
								DF_CopyBufferToFlashPage(CurrPageAddress);  // Save currently written buffer data

								// TODO: BELOW LINE BREAKS STORAGE MODE
								PM_SetupDFAddressCounters(MemoryType);      // Get partial data saved in page

								DF_CopyFlashPageToBuffer(CurrPageAddress);  // Set up the counters to continue from the new address
								DF_BufferWriteEnable(CurrBuffByte);         // Enable writing from the new location
							}
			 							
							PacketBytes[1] = STATUS_CMD_OK;

							V2P_SendPacket();
							break;			
						case CMD_GET_PARAMETER:
						case CMD_SET_PARAMETER:						
							V2P_GetSetParamater();
							break;
						default:
							((FuncPtr)InterpretPacketRoutine)();            // Run the interpret packet routine as set by the pointer
					}

					V2PState       = V2P_STATE_PACKOK;
				}
				else
				{					
					MessageSize    = 2;
					PacketBytes[1] = STATUS_CKSUM_ERROR;
					V2P_SendPacket();
			
					V2PState       = V2P_STATE_PACKOK;
				}				
		}
	}
}

void V2P_SendPacket(void)
{
	USART_Tx(MESSAGE_START);
	USART_Tx(SequenceNum);
	USART_Tx(MessageSize >> 8);
	USART_Tx(MessageSize & 0xFF);
	USART_Tx(TOKEN);

	for(uint16_t SentBytes = 0; SentBytes < MessageSize; SentBytes++)
		USART_Tx(PacketBytes[SentBytes]);

	USART_Tx(V2P_GetChecksum());

	SequenceNum++;
}

uint8_t V2P_GetChecksum()
{
	uint8_t CheckSumByte;
	
	/* Checksum for the V2 protocol is comprised of an XOR of all the packet 
      bytes, including the start, sequence number, size and token bytes.    */
	
	CheckSumByte  = MESSAGE_START;
	CheckSumByte ^= SequenceNum;
	CheckSumByte ^= (uint8_t)(MessageSize >> 8);
	CheckSumByte ^= (uint8_t)(MessageSize);
	CheckSumByte ^= TOKEN;
	
	for(uint16_t CByteIndex = 0; CByteIndex < MessageSize; CByteIndex++)
		CheckSumByte ^= PacketBytes[CByteIndex];

	return CheckSumByte;
}

void V2P_GetSetParamater(void)
{
	uint8_t Param_Name = PacketBytes[1];    // Save the parameter number

	MessageSize = 3;                        // Set the default response message size to 3 bytes     
	PacketBytes[1] = STATUS_CMD_OK;         // Set the default response to OK

	switch (Param_Name)                    // Switch based on the recieved parameter byte
	{
		case PARAM_BUILD_NUMBER_LOW:
			PacketBytes[2] = VERSION_MINOR;

			break;
		case PARAM_BUILD_NUMBER_HIGH:
			PacketBytes[2] = VERSION_MAJOR;

			break;
		case PARAM_HARDWARE_VERSION:
		case PARAM_SW_MAJOR:
		case PARAM_SW_MINOR:
			PacketBytes[2] = pgm_read_byte(&VersionData[Param_Name - PARAM_HARDWARE_VERSION]);

			break;
		case PARAM_CONTROLLER_INIT:
			if (PacketBytes[0] == CMD_GET_PARAMETER)
			{
				PacketBytes[2] = Param_ControllerInit;
			}
			else
			{
				MessageSize = 2;
				Param_ControllerInit = PacketBytes[2];
			}
			
			break;
		case PARAM_SCK_DURATION:
			if (PacketBytes[0] == CMD_GET_PARAMETER)
			{
				PacketBytes[2] = eeprom_read_byte_169(&Param_SCKDuration);
			}
			else
			{
				MessageSize = 2;
				eeprom_write_byte_169(&Param_SCKDuration, PacketBytes[2]);
				USI_SPISetSpeed(PacketBytes[2]); // Re-Initialise the USI system with the new frequency
			}
					
			break;
		case PARAM_RESET_POLARITY:
			if (PacketBytes[0] == CMD_GET_PARAMETER)
			{
				PacketBytes[2] = eeprom_read_byte_169(&Param_ResetPolarity);		
			}
			else
			{
				MessageSize = 2;
				eeprom_write_byte_169(&Param_ResetPolarity, PacketBytes[2]);
				MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE); // Change takes effect immediatly
			}
			
			break;
		case PARAM_OSC_PSCALE:
		case PARAM_OSC_CMATCH:
			/* Despite not supporting these parameters (STK500 only), the AVR Studio programmer
			   sends them along with the SCK duration. A OK must be returned or the sequence will fail
			   and the SCK duration byte will not be sent.                                             */
		
			if (PacketBytes[0] == CMD_GET_PARAMETER)
			{
			   PacketBytes[2] = 0;            // If the command is a read, return a 0 for both parameters
			}
			else
			{
				MessageSize = 2;              // Otherwise just send back an OK if the command is a set		
			}
			
			break;
		default:                             // Unrecognised parameter
			MessageSize = 2;
			PacketBytes[1] = STATUS_CMD_FAILED;			
	}
	
	V2P_SendPacket();
}

void V2P_IncrementCurrAddress(void)
{
	// Incrementing a 32-bit unsigned variable takes a lot of code. Because much of the code is
	// not very time critical (much of it is waiting for the hardware), i've chosen to waste
	// a few extra cycles per increment and save a good 60 bytes or so of code space by putting
	// the incrmement inside a function.

	CurrAddress++;
}

void V2P_CheckForExtendedAddress(void)
{
	if (CurrAddress & (1UL << 31))                     // MSB set of the address, indicates a LOAD_EXTENDED_ADDRESS must be executed
	{
		USI_SPITransmit(V2P_LOAD_EXTENDED_ADDR_CMD);   // Load extended address command
		USI_SPITransmit(0x00);
		USI_SPITransmit((CurrAddress & 0x00FF0000) >> 16); // The 3rd byte of the long holds the extended address
		USI_SPITransmit(0x00);
		
		CurrAddress &= ~(1UL << 31);                   // Clear the flag
	}
}
