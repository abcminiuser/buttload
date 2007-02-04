/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#define  INC_FROM_V2P
#include "V2Protocol.h"

// PROGMEM CONSTANTS:
const uint8_t SignonResponse[]  PROGMEM = {AICB_CMD_SIGN_ON, AICB_STATUS_CMD_OK, 8, 'A', 'V', 'R', 'I', 'S', 'P', '_', '2', 0x00};

// GLOBAL VARIABLES:
uint8_t  PacketBytes[V2P_MAXBUFFSIZE]   = {};
uint8_t  SequenceNum                    = 0;
uint16_t MessageSize                    = 0;
uint8_t  InProgrammingMode              = FALSE;
uint32_t CurrAddress                    = 0;

uint8_t  Param_ControllerInit           = 0; // This is set to zero on reset, and can be written to or read by the computer

// ======================================================================================

void V2P_RunStateMachine(FuncPtr PacketDecodeFunction)
{
	uint8_t  V2PState           = V2P_STATE_IDLE;
	uint16_t CurrentMessageByte = 0;

	BUFF_InitialiseBuffer();	
	TIMEOUT_SLEEP_TIMER_OFF();
	
	InProgrammingMode = FALSE;
	CurrAddress       = 0;

	for (;;)
	{
		if (PacketTimeOut == TRUE)                // Packet has timed out waiting for data
		  V2PState = V2P_STATE_TIMEOUT;
		else if (V2PState != V2P_STATE_IDLE)
		  TIMEOUT_PACKET_TIMER_ON();              // Reset the timer on each loop if not in idle mode
		
		switch (V2PState)
		{
			case V2P_STATE_IDLE:
				cli();
				
				if (BuffElements)                 // Serial data recieved in FIFO buffer
				  V2PState = V2P_STATE_START;

				sei();
				
				if ((JoyStatus & JOY_LEFT) && !(InProgrammingMode))
				{
					USART_OFF();
					TOUT_SetupSleepTimer();       // Re-setup and start the auto-sleep timer
					return;
				}
								
				IDLECPU();                        // Can idle here, since USART or Joystick interrupt will cause execution to continue

				break;
			case V2P_STATE_START:			
				if (USART_Rx() == AICB_MESSAGE_START)  // Start bit is always 0x1B
				  V2PState = V2P_STATE_GETSEQUENCENUM;
				else
				  V2PState = V2P_STATE_PACKERR;
				
				break;
			case V2P_STATE_GETSEQUENCENUM:
				SequenceNum = USART_Rx();
				V2PState    = V2P_STATE_GETMESSAGESIZE1;

				break;
			case V2P_STATE_GETMESSAGESIZE1:
				MessageSize = ((uint16_t)USART_Rx() << 8);  // Message size is MSB first				
				V2PState    = V2P_STATE_GETMESSAGESIZE2;
				
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
				if (USART_Rx() == AICB_TOKEN)      // Token bit is always 0x0E
				  V2PState = V2P_STATE_GETDATA;
				else                               // Incorrect token bit
				  V2PState = V2P_STATE_PACKERR;

				break;
			case V2P_STATE_GETDATA:
				if (CurrentMessageByte == MessageSize) // Packet reception complete
				  V2PState = V2P_STATE_GETCHECKSUM;
				else
				  PacketBytes[CurrentMessageByte++] = USART_Rx(); // Store the byte
	
				break;
			case V2P_STATE_GETCHECKSUM:
				if  (!(PacketTimeOut))             // Only try to process the packet if there is no timeout
				{
					if (V2P_GetChecksum() == USART_Rx()) // If checksum is ok, process the packet
					{
						switch (PacketBytes[0])    // \/ Look for generic commands which can be interpreted here, 
						{                          //  \ otherwise run the custom interpret routine
							case AICB_CMD_SIGN_ON:
								MessageSize = 11;

								for (uint8_t SOByte = 0; SOByte < 11; SOByte++) // Load the sign-on sequence from program memory
								  PacketBytes[SOByte] = pgm_read_byte(&SignonResponse[SOByte]);

								V2P_SendPacket();
								break;
							case AICB_CMD_FIRMWARE_UPGRADE:
								MessageSize = 2;
			
								PacketBytes[1] = AICB_STATUS_CMD_FAILED;  // Return failed (no automatic firmware upgrades)
			
								V2P_SendPacket();
								break;				
							case AICB_CMD_LOAD_ADDRESS:
								MessageSize = 2;

								CurrAddress = ((uint32_t)PacketBytes[1] << 24)
								            | ((uint32_t)PacketBytes[2] << 16)
											| ((uint16_t)PacketBytes[3] << 8)
											|            PacketBytes[4];

								if (PacketDecodeFunction == AICI_InterpretPacket)
								  V2P_CheckForExtendedAddress();
								else
								  VAMM_SetAddress();
								
								PacketBytes[1] = AICB_STATUS_CMD_OK;

								V2P_SendPacket();
								break;			
							case AICB_CMD_GET_PARAMETER:
							case AICB_CMD_SET_PARAMETER:						
								V2P_GetSetParamater();
								break;
							default:
								((FuncPtr)PacketDecodeFunction)();        // Run the interpret packet routine as set by the pointer
						}

						V2PState = V2P_STATE_PACKOK;
					}
					else
					{					
						V2PState = V2P_STATE_BADCHKSUM;
					}
				}

				break;
			case V2P_STATE_BADCHKSUM:
			case V2P_STATE_TIMEOUT:
			case V2P_STATE_PACKERR:
				if (V2PState == V2P_STATE_BADCHKSUM)
				  PacketBytes[1] = AICB_STATUS_CKSUM_ERROR;
				else if (V2PState == V2P_STATE_TIMEOUT)
				  PacketBytes[1] = AICB_STATUS_CMD_TOUT;
				else
				  PacketBytes[1] = AICB_STATUS_CMD_FAILED;

				MessageSize = 2;
				V2P_SendPacket();

				// Fall through to V2P_STATE_PACKOK
			case V2P_STATE_PACKOK:
				PacketTimeOut = FALSE;
				BUFF_InitialiseBuffer();           // Flush the ringbuffer
				TIMEOUT_PACKET_TIMER_OFF();

				V2PState = V2P_STATE_IDLE;
		}
	}	
}

void V2P_SendPacket(void)
{
	USART_Tx(AICB_MESSAGE_START);
	USART_Tx(SequenceNum);
	USART_Tx(MessageSize >> 8);
	USART_Tx(MessageSize & 0xFF);
	USART_Tx(AICB_TOKEN);

	for (uint16_t SentBytes = 0; SentBytes < MessageSize; SentBytes++)
		USART_Tx(PacketBytes[SentBytes]);

	USART_Tx(V2P_GetChecksum());

	SequenceNum++;
}


void V2P_IncrementCurrAddress(void)
{
	// Incrementing a 32-bit unsigned variable takes a lot of code. Because much of the code is
	// not very time critical (much of it is waiting for the hardware), I've chosen to waste a
	// few extra cycles per increment and save a good 60 bytes or so of code space by putting the
	// increment inside its own function.

	CurrAddress++;
}

void V2P_CheckForExtendedAddress(void)
{
	if (CurrAddress & V2P_LOAD_EXTENDED_ADDR_FLAG)     // MSB set of the address, indicates a LOAD_EXTENDED_ADDRESS must be executed
	{
		USI_SPITransmit(V2P_LOAD_EXTENDED_ADDR_CMD);   // Load extended address command
		USI_SPITransmit(0x00);
		USI_SPITransmit((CurrAddress & V2P_LOAD_EXTENDED_ADDR_MASK) >> V2P_LOAD_EXTENDED_ADDR_SHIFT);
		USI_SPITransmit(0x00);
		
		CurrAddress &= ~(V2P_LOAD_EXTENDED_ADDR_FLAG); // Clear the flag
	}
}

static uint8_t V2P_GetChecksum(void)
{
	uint8_t CheckSumByte;
	
	/* Checksum for the V2 protocol is comprised of an XOR of all the packet 
      bytes, including the start, sequence number, size and token bytes.    */
	
	CheckSumByte  = AICB_MESSAGE_START;
	CheckSumByte ^= SequenceNum;
	CheckSumByte ^= (uint8_t)(MessageSize >> 8);
	CheckSumByte ^= (uint8_t)(MessageSize & 0xFF);
	CheckSumByte ^= AICB_TOKEN;
	
	for (uint16_t CByteIndex = 0; CByteIndex < MessageSize; CByteIndex++)
	   CheckSumByte ^= PacketBytes[CByteIndex];

	return CheckSumByte;
}

static void V2P_GetSetParamater(void)
{
	uint8_t Param_Name = PacketBytes[1];    // Save the parameter number

	MessageSize = 3;                        // Set the default response message size to 3 bytes     
	PacketBytes[1] = AICB_STATUS_CMD_OK;    // Set the default response to OK

	switch (Param_Name)                     // Switch based on the recieved parameter byte
	{
		case AICB_PARAM_BUILD_NUMBER_LOW:
			PacketBytes[2] = VERSION_MINOR;

			break;
		case AICB_PARAM_BUILD_NUMBER_HIGH:
			PacketBytes[2] = VERSION_MAJOR;

			break;
		case AICB_PARAM_HARDWARE_VERSION:
			PacketBytes[2] = V2P_HW_VERSION;

			break;
		case AICB_PARAM_SW_MAJOR:
			PacketBytes[2] = V2P_SW_VERSION_MAJOR;

			break;
		case AICB_PARAM_SW_MINOR:
			PacketBytes[2] = ((eeprom_read_byte(&EEPROMVars.FirmVerMinor) == 0xFF)? V2P_SW_VERSION_MINOR_DEFAULT : eeprom_read_byte(&EEPROMVars.FirmVerMinor));

			break;
		case AICB_PARAM_CONTROLLER_INIT:
			if (PacketBytes[0] == AICB_CMD_GET_PARAMETER)
			{
				PacketBytes[2] = Param_ControllerInit;
			}
			else
			{
				MessageSize = 2;
				Param_ControllerInit = PacketBytes[2];
			}
			
			break;
		case AICB_PARAM_SCK_DURATION:
			if (PacketBytes[0] == AICB_CMD_GET_PARAMETER)
			{
				PacketBytes[2] = pgm_read_byte(USISpeedVals[eeprom_read_byte(&EEPROMVars.SCKDuration)]);
				if (PacketBytes[2] > USI_PRESET_SPEEDS)
				  PacketBytes[2] = 0;
			}
			else
			{
				MessageSize = 2;
				
				if (PacketBytes[2] >= USI_STUDIO_SPEEDS) // Bounds check
				  PacketBytes[2] = USI_STUDIO_SPEEDS - 1;

				eeprom_write_byte(&EEPROMVars.SCKDuration, pgm_read_byte(USISpeedIndex[PacketBytes[2]]));

				USI_SPIInitMaster();          // Re-Initialise the USI system with the new frequency
			}
					
			break;
		case AICB_PARAM_RESET_POLARITY:
			if (PacketBytes[0] == AICB_CMD_GET_PARAMETER)
			{
				PacketBytes[2] = eeprom_read_byte(&EEPROMVars.ResetPolarity);		
			}
			else
			{
				MessageSize = 2;
				eeprom_write_byte(&EEPROMVars.ResetPolarity, PacketBytes[2]);
				MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE); // Change takes effect immediatly
			}
			
			break;
		case AICB_PARAM_STATUS:
		case AICB_PARAM_OSC_PSCALE:
		case AICB_PARAM_OSC_CMATCH:
		case AICB_PARAM_TOPCARD_DETECT:
		case AICB_PARAM_VTARGET:
		case AICB_PARAM_VADJUST:
		case AICB_PARAM_DATA:
		
			/* Despite not supporting these parameters (STK500 only), the AVR Studio programmer
			   seems to send them occasionally. A OK must be returned or the sequence will fail. */
		
			if (PacketBytes[0] == AICB_CMD_GET_PARAMETER)
			  PacketBytes[2] = 0;             // If the command is a read, return a 0 for both parameters
			else
			  MessageSize = 2;                // Otherwise just send back an OK if the command is a set		
			
			break;
		default:                              // Unrecognised parameter
			MessageSize = 2;
			PacketBytes[1] = AICB_STATUS_CMD_FAILED;			
	}
	
	V2P_SendPacket();
}
