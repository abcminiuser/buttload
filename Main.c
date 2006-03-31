/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
						

  Requires: AVR-GCC 3.4.3 or above, AVRLibC version 1.4.0 or above
*/

#include "ButtLoadTag.h"
BUTTLOADTAG(Name,   "BUTTLOAD AVRISP");
BUTTLOADTAG(Author, "BY DEAN CAMERA");

/*
    LICENCE:
      
      This program is free software; you can redistribute it and/or
      modify it under the terms of the GNU General Public License
      as published by the Free Software Foundation; either version 2
      of the License, or (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program; if not, write to the Free Software
      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
	  02110-1301, USA.
*/

/*
	FEATURES:
		
		1) Ability to emulate an AVRISP to allow the programming of AVRs with a computer
		   via the SPI interface
		
		2) Ability to store a program (including EEPROM, lockbit and fusebit data) in
		   non-volatile memory and be able to retrieve said program on user request and
		   thus program AVRs via ISP in the field
		
		3) Devices with large, 32-bit addresses (such as the MEGA2560) are supported.
		
		4) Ability to read and program external AVR dataflash memory chips directly
					
			      Status Colour | Description
			      --------------+------------
			       Green        | Ready
			       Orange       | Busy
			       Red          | Programming
*/

/*
	KNOWN ISSUES:

		1) The maximum speed of ButtLoad's USI system in SPI mode is 210,651Hz.

		2) A maximum of 10 fuse bytes and 10 lock bytes can be stored in memory at any one
		   time (writing the same fuse overwrites the existing value). If it is attempted to
		   write more than this maximum, the extra bytes will be ignored.
*/

/*
	CONNECTIONS:
	
	 * Because the on-board dataflash is connected to the SPI interface, the code uses
	 the USI system in three-wire mode to communicate with the slave AVR instead. This
	 means that the two systems can run at different data rates without switching, and
	 is also nessesary because the slave AVR does not have a /CS pin.
	
		USI Interface
			Pin 1  (SCK) - Slave AVR SCK
			Pin 2  (DI)  - Slave AVR MISO
			Pin 3  (DO)  - Slave AVR MOSI
			Pin 4  (GND) - Slave AVR GND
			
		PORTF (as viewed from the TOP of the board)
			Pin 1  (PF4) - Green (active-high) lead of a Bicolour LED (optional)
			Pin 5  (PF5) - Red (active-high) lead of a Bicolour LED (optional)
			Pin 3  (PF6) - /RESET line of slave AVR and /CS line of slave Dataflash
			Pin 10 (GND) - Status LED ground if used (optional)			
			
		USART Interface
			Pin 1        - Recieve relative to Butterfly
			Pin 2        - Transmit relative to Butterfly
			Pin 3        - Ground

	 * Level shifting circuitry must be employed that can translate the 3.3V Butterfly
	 signals to the target AVR's voltage and vice-versa AT SUFFICIENT CURRENT.
*/

/*
	AUTHOR CREDIT:

		FILE                                     |  AUTHOR
		-----------------------------------------+---------------------------------------------
		USART.c + Header file                    | By Atmel, ported to GCC by Martin Thomas
		EEPROM169.c + Header file                | By Atmel, ported to GCC by Martin Thomas
		DataflashCommandBytes.h                  | By Atmel, modified by Martin Thomas
		Dataflash.c + Header file                | By Dean Camera, re-coded from the generic dataflash
		                                         | code by Atmel (ported to GCC by Martin Thomas)
		USI.c + Header file                      | By Atmel, ported to GCC and modified by Dean Camera
		OSCCal.c + Header file                   | By Colin Oflynn, modified by Dean Camera
		LCD_Driver.c + Header file               | By Dean Camera
		Main.c + Header file                     | By Dean Camera
		SPI.c + Header file                      | By Dean Camera
		V2Protocol.c + Header file               | By Dean Camera
		AVRISPCommandBytes.h                     | By Dean Camera
		ISPChipComm.c + Header file              | By Dean Camera
		AVRISPCommandInterpreter.c + Header file | By Dean Camera
		ProgramManager.c + Header file           | By Dean Camera
		EEPROMVariables.c + Header file          | By Dean Camera		
		JoystickInterrupt.S                      | By Dean Camera
		USIInterrupt.S                           | By Dean Camera
		RingBuff.c + Header file                 | By Dean Camera
		ISRMacro.h                               | By Dean Camera
		Timeout.c + Header file                  | By Dean Camera
-------------------------------------------------+---------------------------------------------

   Special thanks to Barry (BPar) of AVRFreaks, for without his equipment and wisdom in debugging this
  monstrocity i'd still be working on it. Also thanks to the other members of AVRFreaks for their ideas.

--------------------------------------------------------------------------------------------------------

	Note: Almost all the 3rd party code has been modified in some way for this project. Please use the
	original code for each file if you intend to use any of the 3rd party code in you own project.
*/

#include "Main.h"

// PROGMEM CONSTANTS:
const uint8_t    WaitText[]              PROGMEM = "*WAIT*";

const uint8_t    ProgrammerName[]        PROGMEM = "BUTTLOAD";
const uint8_t    VersionInfo[]           PROGMEM = {'V','0' + VERSION_MAJOR,'-','0' + VERSION_MINOR, '\0'};
const uint8_t    AuthorName[]            PROGMEM = "BY DEAN CAMERA";
const uint8_t    CopyRight[]             PROGMEM = "<C> 2006 - GPL";

const uint8_t*   AboutTextPtrs[]         PROGMEM = {ProgrammerName, VersionInfo, AuthorName, CopyRight};

const uint8_t    Func_ISPPRGM[]          PROGMEM = "AVRISP MODE";
const uint8_t    Func_STOREPRGM[]        PROGMEM = "STORE PRGM";
const uint8_t    Func_PRGMAVR[]          PROGMEM = "PROGRAM AVR";
const uint8_t    Func_PRGMDATAFLASH[]    PROGMEM = "DATAFLASH PRGM MODE";
const uint8_t    Func_PRGMSTOREINFO[]    PROGMEM = "DATASTORE INFO";
const uint8_t    Func_SETTINGS[]         PROGMEM = "SETTINGS";
const uint8_t    Func_SLEEP[]            PROGMEM = "SLEEP MODE";
	
const uint8_t*   MainFunctionNames[]     PROGMEM = {Func_ISPPRGM  , Func_STOREPRGM  , Func_PRGMAVR  , Func_PRGMDATAFLASH  , Func_PRGMSTOREINFO, Func_SETTINGS      , Func_SLEEP};
const FuncPtr    MainFunctionPtrs[]      PROGMEM = {FUNCAVRISPMode, FUNCStoreProgram, FUNCProgramAVR, FUNCProgramDataflash, FUNCStorageInfo   , FUNCChangeSettings , FUNCSleepMode};

const uint8_t    SFunc_SETCONTRAST[]     PROGMEM = "SET CONTRAST";
const uint8_t    SFunc_SETSPISPEED[]     PROGMEM = "SET SPI SPEED";
const uint8_t    SFunc_CLEARMEM[]        PROGMEM = "CLEAR MEMORY";
const uint8_t    SFunc_AUTOCALIB[]       PROGMEM = "AUTO CALIBRATE";
const uint8_t    SFunc_MANCALIB[]        PROGMEM = "MANUAL CALIBRATION";
const uint8_t    SFunc_GOBOOTLOADER[]    PROGMEM = "JUMP TO BOOTLOADER";

const uint8_t*   SettingFunctionNames[]  PROGMEM = {SFunc_SETCONTRAST, SFunc_SETSPISPEED, SFunc_CLEARMEM, SFunc_AUTOCALIB, SFunc_MANCALIB, SFunc_GOBOOTLOADER};
const FuncPtr    SettingFunctionPtrs[]   PROGMEM = {FUNCSetContrast  , FUNCSetISPSpeed  , FUNCClearMem  , FUNCAutoCalib  , FUNCManCalib, FUNCGoBootloader};

const uint8_t    PRG_D[]                 PROGMEM = "DATA ONLY";
const uint8_t    PRG_E[]                 PROGMEM = "EEPROM ONLY";
const uint8_t    PRG_DE[]                PROGMEM = "DATA AND EEPROM";
const uint8_t    PRG_F[]                 PROGMEM = "FUSE BYTES ONLY";
const uint8_t    PRG_L[]                 PROGMEM = "LOCK BYTES ONLY";
const uint8_t    PRG_FL[]                PROGMEM = "FUSE AND LOCK BYTES";
const uint8_t    PRG_C[]                 PROGMEM = "ERASE ONLY";

const uint8_t*   ProgOptions[]           PROGMEM = {PRG_D, PRG_E, PRG_DE, PRG_F, PRG_L, PRG_FL, PRG_C};

const uint8_t    USI_Speed0[]            PROGMEM = " 57153 HZ";
const uint8_t    USI_Speed1[]            PROGMEM = " 86738 HZ";
const uint8_t    USI_Speed2[]            PROGMEM = "113427 HZ";
const uint8_t    USI_Speed3[]            PROGMEM = "210651 HZ";

const uint8_t*   USIPSNamePtrs[USI_PRESET_SPEEDS] PROGMEM = {USI_Speed0, USI_Speed1, USI_Speed2, USI_Speed3};

const uint8_t    SIFO_Size[]             PROGMEM = "STORAGE SIZES";
const uint8_t    SIFO_Tags[]             PROGMEM = "VIEW DATA TAGS";

const uint8_t*   SIFOOptionPtrs[]        PROGMEM = {SIFO_Size, SIFO_Tags};

// ======================================================================================

int main(void)
{
	uint8_t CurrFunc = 0;

	// TODO: REENABLE JTAG
	//MCUCR   = (1 << JTD);                        // Turn off JTAG via code
	//MCUCR   = (1 << JTD);                        // Twice as specified in datasheet
	
	ACSR    = (1 << ACD);                        // Disable the unused Analogue Comparitor to save power
	PRR     = ((1 << PRADC) | (1 << PRSPI));     // Disable the ADC (unused) and SPI (for now) to save power
	
	DDRF    = ((1 << 4) | (1 << 5)); // Set status LEDs as outputs
	DDRB    = ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 5)); // On-board dataflash /CS, ISP MOSI/SCK and beeper as outputs
	PORTB   = ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)   // Set SPI pins to high/pullups, and disable dataflash (send /CS high)
	        | JOY_BMASK);                        // \ Turn joystick
	PORTE   = JOY_EMASK;                         // /  pullups on
	
	PCMSK0  = JOY_EMASK;                         // \.
	PCMSK1  = JOY_BMASK;                         // | Turn on joystick
	EIMSK   = ((1<<PCIE0) | (1<<PCIE1));         // |  interrupts
	EIFR    = ((1<<PCIF0) | (1<<PCIF1));         // /

	MAIN_SETSTATUSLED(MAIN_STATLED_RED);	     // Set status LEDs to red (busy)

	if (eeprom_read_byte_169(&Sys_MagicNumber) != MAGIC_NUM) // Check if first ButtLoad run
	{
		for (uint16_t EAddr = 0; EAddr < Sys_MagicNumber; EAddr++) // Clear the EEPROM if first run
		   eeprom_write_byte_169(&EAddr, 0xFF);

		eeprom_write_byte_169(&Sys_MagicNumber, MAGIC_NUM);
	}

	LCD_Init();
	LCD_CONTRAST_LEVEL(eeprom_read_byte_169(&Sys_LCDContrast));

	sei();
	
	LCD_puts_f(WaitText);

	DF_EnableDataflash(FALSE);                   // Pull internal Dataflash /CS high to disable it and thus save power

	MAIN_SETSTATUSLED(MAIN_STATLED_ORANGE);      // Set status LEDs to orange (busy)

	OSCCAL_Calibrate();                          // Calibrate the internal occilator correction value
	USART_Init(USART_BAUDVALUE);                 // UART at 115200 baud (7.3MHz clock, double USART speed)
	
	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);	     // Set status LEDs to green (ready)
	
	JoyStatus = 1;                               // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	
	while (1)
	{
		if (JoyStatus)                           // Joystick is in the non-center position
		{
			if (JoyStatus & JOY_UP)              // Previous function
			  (CurrFunc == 0)? CurrFunc = (MAIN_TOTALMAINMENUITEMS - 1): CurrFunc--;
			else if (JoyStatus & JOY_DOWN)      // Next function
			  (CurrFunc == (MAIN_TOTALMAINMENUITEMS - 1))? CurrFunc = 0 : CurrFunc++;
			else if (JoyStatus & JOY_PRESS)     // Select current function
			  ((FuncPtr)pgm_read_word(&MainFunctionPtrs[CurrFunc]))(); // Run associated function
			else if (JoyStatus & JOY_RIGHT)
			  FUNCShowAbout();
		
			// Show current setting function onto the LCD:
			LCD_puts_f((uint8_t*)pgm_read_word(&MainFunctionNames[CurrFunc]));

			MAIN_WaitForJoyRelease();
		}
	}

	return 0;
}

// ======================================================================================

void MAIN_Delay10MS(uint8_t loops)
{
  /* Prevents the use of floating point libraries. Delaying in groups of
     10ms increases accuracy by reducing the time overhead for each loop
     interation of the while.                                            */

	while (loops--)
	  _delay_ms(10);
}

void MAIN_Delay1MS(uint8_t loops)
{
  /* Prevents the use of floating point libraries. Less accurate than the
     Delay10MS routine, but nessesary for many commands. The overhead required
     to call the routine is substantially less than the overhead required to
     calculate the float at compile time, so this actually saves execution time. */

	while (loops--)
	  _delay_ms(1);
}

void MAIN_ResetCSLine(uint8_t ActiveInactive)
{
  /* ActiveInactive controls the /Reset line to an AVR device or external dataflash
     /CS line. If the reset polarity parameter is a 0 then interfacing with AT89
	 devices which has an active high reset. Pins are tristated when inactive.      */
	
	switch (ActiveInactive)
	{
		case MAIN_RESETCS_ACTIVE:   // The target RESET line may be either active high or low.
			DDRF |= (1 << 6);
		
			if (!(eeprom_read_byte_169(&Param_ResetPolarity))) // Translate to correct logic level for target device type
			  PORTF |=  (1 << 6);
			else
			  PORTF &= ~(1 << 6);
		
			break;
		case MAIN_RESETCS_EXTDFACTIVE: // Dataflashes are always active low.
			DDRF  |=  (1 << 6);
			PORTF &= ~(1 << 6);
			
			break;
		case MAIN_RESETCS_INACTIVE: // Both modes tristate the pins when inactive.
			DDRF  &= ~(1 << 6);
			PORTF &= ~(1 << 6);
	}
}

void MAIN_WaitForJoyRelease(void)
{
	while (1)
	{
		while (JoyStatus) {};                   // Wait until joystick released

		MAIN_Delay10MS(1);

		if (!(JoyStatus))                       // Joystick still released (not bouncing), return
		  return;
	}
}

void MAIN_IntToStr(uint16_t IntV, uint8_t* Buff)
{
	// Shows leading zeros, unlike itoa.

	uint8_t Temp = 0;
	
	while (IntV >= 100)
	{
		Temp++;
		IntV -= 100;
	}

	*(Buff++) = '0' + Temp;
	
	Temp = 0;
	
	while (IntV >= 10)
	{
		Temp++;
		IntV -= 10;
	}
		
	*(Buff++) = '0' + Temp;
	*(Buff++) = '0' + IntV;
	*(Buff)   = '\0';
}

void MAIN_ShowProgType(uint8_t Letter)
{
	uint8_t ProgTypeBuffer[7];

	strcpy_P(ProgTypeBuffer, PSTR("PRG> "));
	ProgTypeBuffer[5] = Letter;
	ProgTypeBuffer[6] = '\0';
	
	LCD_puts(ProgTypeBuffer);
}

void MAIN_ShowError(const uint8_t *pFlashStr)
{
	uint8_t ErrorBuff[LCD_TEXTBUFFER_SIZE];       // New buffer, LCD text buffer size
	
	ErrorBuff[0] = 'E';
	ErrorBuff[1] = '>';

	strcpy_P(&ErrorBuff[2], pFlashStr);       // WARNING: If error text is larger than (TEXTBUFFER_SIZE - 2),
	                                          // this will overflow the buffer and probably crash the micro!
	LCD_puts(ErrorBuff);
	
	MAIN_WaitForJoyRelease();
	while (!(JoyStatus & JOY_PRESS)) {};
	MAIN_WaitForJoyRelease();
}

// ======================================================================================

ISR(PCINT1_vect, ISR_NOBLOCK)                 // Joystick routine; PCINT0_vect is bound to this also via JoystickInterrupt.S
{
	JoyStatus = (~PINB & JOY_BMASK)
	          | (~PINE & JOY_EMASK);
}

ISR(BADISR_vect, ISR_NAKED)                   // Bad ISR routine; should never be called, here for safety
{
	MAIN_ShowError(PSTR("BADISR"));
	while (1) {};
}

// ======================================================================================

void FUNCChangeSettings(void)
{
	uint8_t CurrSFunc = 0;
	
	JoyStatus = 1;

	while (1)
	{
		if (JoyStatus)                         // Joystick is in the non-center position
		{
			if (JoyStatus & JOY_UP)            // Previous function
			  (CurrSFunc == 0)? CurrSFunc = 5 : CurrSFunc--;
			else if (JoyStatus & JOY_DOWN)     // Next function
			  (CurrSFunc == 5)? CurrSFunc = 0 : CurrSFunc++;
			else if (JoyStatus & JOY_PRESS)    // Select current function
			  ((FuncPtr)pgm_read_word(&SettingFunctionPtrs[CurrSFunc]))(); // Run associated function
			else if (JoyStatus & JOY_LEFT)
			  return;
		
			// Show current function onto the LCD:
			LCD_puts_f((uint8_t*)pgm_read_word(&SettingFunctionNames[CurrSFunc]));

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCShowAbout(void)
{
	uint8_t InfoNum = 0;
	
	JoyStatus = 1;
			
	while (1)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			  (InfoNum == 0)? InfoNum = 3 : InfoNum--;
			else if (JoyStatus & JOY_DOWN)
			  (InfoNum == 3)? InfoNum = 0 : InfoNum++;
			else if (JoyStatus & JOY_LEFT)
			  return;

			LCD_puts_f((uint8_t*)pgm_read_word(&AboutTextPtrs[InfoNum]));

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCAVRISPMode(void)
{
	LCD_puts_f(AVRISPModeMessage);
	
	InterpretPacketRoutine = (FuncPtr)AICI_InterpretPacket;
	V2P_RunStateMachine();
}

void FUNCProgramDataflash(void)
{
	USI_SPIInitMaster(eeprom_read_byte_169(&Param_SCKDuration));
	UseExernalDF = TRUE;
	DFSPIRoutinePointer = USI_SPITransmit;
	
	LCD_puts_f(DataFlashProgMode);

	InterpretPacketRoutine = PD_InterpretAVRISPPacket;
	V2P_RunStateMachine();
	   
	DF_EnableDataflash(FALSE);
	SPI_SPIOFF();
}

void FUNCProgramAVR(void)
{
	uint8_t  DoneFailMessageBuff[19];
	uint16_t EEPROMAddress;
	uint8_t  Fault = ISPCC_NO_FAULT;
	uint8_t  ProgMode = 0;

	SPI_SPIInit();
	UseExernalDF = FALSE;
	DFSPIRoutinePointer = SPI_SPITransmit;
	
	if (!(DF_CheckCorrectOnboardChip()))
	  return;

	MAIN_WaitForJoyRelease();
	
	JoyStatus = 1;                            // Use an invalid joystick value to force the program to write the
	                                          // name of the default command onto the LCD
	while (1)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_LEFT)
			  return;
			else if (JoyStatus & JOY_PRESS)
			  break;
			else if (JoyStatus & JOY_UP)
			  (ProgMode == 0)? ProgMode = 6 : ProgMode--;
			else if (JoyStatus & JOY_DOWN)
			  (ProgMode == 6)? ProgMode = 0 : ProgMode++;

			LCD_puts_f((uint8_t*)pgm_read_word(&ProgOptions[ProgMode])); // Show current function onto the LCD

			MAIN_WaitForJoyRelease();
		}
	}

	MAIN_SETSTATUSLED(MAIN_STATLED_ORANGE);                // Orange = busy
	LCD_puts_f(WaitText);

	USI_SPIInitMaster(eeprom_read_byte_169(&Param_SCKDuration));
	MAIN_ResetCSLine(MAIN_RESETCS_ACTIVE); // Capture the RESET line of the slave AVR

	EEPROMAddress = Prog_EnterProgMode;
			
	for (uint8_t PacketB = 0; PacketB <= 11; PacketB++) // Read the enter programming mode command bytes
	{
		PacketBytes[PacketB] = eeprom_read_byte_169(&EEPROMAddress);
		EEPROMAddress++;
	}
	
	ISPCC_EnterChipProgrammingMode();    // Try to sync with the slave AVR

	CurrAddress = 0;

	if (PacketBytes[1] == STATUS_CMD_OK) // ISPCC_EnterChipProgrammingMode alters the PacketBytes buffer rather than returning a value
	{						
		if ((ProgMode == 6) || (ProgMode == 0) || (ProgMode == 2)) // Erase chip, or program flash mode
		{
			MAIN_ShowProgType('C');
			
			if (!(eeprom_read_byte_169(&Prog_EraseCmdStored) == TRUE))
			{
				Fault = ISPCC_FAULT_NOERASE;
				MAIN_ShowError(PSTR("NO ERASE CMD"));
			}
			else
			{
				PM_SendEraseCommand();
			}
		}

		if (((ProgMode == 0) || (ProgMode == 2)) && (Fault == ISPCC_NO_FAULT)) // Program flash
		{
			MAIN_ShowProgType('D');

			if (!(PM_GetStoredDataSize(TYPE_FLASH))) // Check to make sure a program is present in memory
			{
				Fault = ISPCC_FAULT_NODATATYPE;					
				MAIN_ShowError(PSTR("NO DATA"));
			}
			else
			{
				PM_CreateProgrammingPackets(TYPE_FLASH);
			}
		}
	
		if ((ProgMode == 1) || (ProgMode == 2)) // Program EEPROM
		{
			MAIN_ShowProgType('E');
				
			if (!(PM_GetStoredDataSize(TYPE_EEPROM))) // Check to make sure EEPROM data is present in memory
			{
				Fault = ISPCC_FAULT_NODATATYPE;
				MAIN_ShowError(PSTR("NO EEPROM"));
			}
			else
			{
				PM_CreateProgrammingPackets(TYPE_EEPROM);
			}
		}

		if ((ProgMode == 3) || (ProgMode == 5)) // Program Fuse bytes
		{
			MAIN_ShowProgType('F');
			
			if (!(eeprom_read_byte_169(&Prog_TotalFuseBytes)))
			{
				Fault = ISPCC_FAULT_NODATATYPE;					
				MAIN_ShowError(PSTR("NO FUSE BYTES"));
			}
			else
			{
				PM_SendFuseLockBytes(TYPE_FUSE);
			}
		}

		if ((ProgMode == 4) || (ProgMode == 5)) // Program Lock bytes
		{
			if (ProgMode == 5)                    // If fusebytes have already been written, we need to reenter programming mode to latch them
			{
				MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE); // Release the RESET line of the slave AVR
				MAIN_Delay10MS(1);
				MAIN_ResetCSLine(MAIN_RESETCS_ACTIVE); // Capture the RESET line of the slave AVR
				ISPCC_EnterChipProgrammingMode(); // Try to sync with the slave AVR
			}

			MAIN_ShowProgType('L');
		
			if (!(eeprom_read_byte_169(&Prog_TotalLockBytes)))
			{
				Fault = ISPCC_FAULT_NODATATYPE;
				MAIN_ShowError(PSTR("NO LOCK BYTES"));
			}
			else
			{
				PM_SendFuseLockBytes(TYPE_LOCK);
			}
		}

		strcpy_P(DoneFailMessageBuff, PSTR("PROGRAMMING DONE"));

		if (Fault != ISPCC_NO_FAULT)         // Takes less code to just overwrite part of the string on fail
		  strcpy_P(&DoneFailMessageBuff[12], PSTR("FAILED"));

		LCD_puts(DoneFailMessageBuff);

		MAIN_Delay10MS(255);
		MAIN_Delay10MS(100);
	}
	else
	{
		MAIN_ShowError(SyncErrorMessage);
	}
	
	MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE); // Release the RESET line and allow the slave AVR to run	
	USI_SPIOff();
	DF_EnableDataflash(FALSE);
	SPI_SPIOFF();
	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);   // Green = ready
}

void FUNCStoreProgram(void)
{
	DFSPIRoutinePointer = (SPIFuncPtr)SPI_SPITransmit;
	SPI_SPIInit();
	UseExernalDF = FALSE;
	DF_EnableDataflash(TRUE);

	if (!(DF_CheckCorrectOnboardChip()))
	  return;
			
	LCD_puts_f(PSTR("*STORAGE MODE*"));

	InterpretPacketRoutine = (FuncPtr)PM_InterpretAVRISPPacket;
	InPMMode = TRUE;
	V2P_RunStateMachine();
	InPMMode = FALSE;
	DF_EnableDataflash(FALSE);
	SPI_SPIOFF();
}

void FUNCClearMem(void)
{
	LCD_puts_f(PSTR("CONFIRM"));
	MAIN_Delay10MS(180);

	LCD_puts_f(PSTR("<N Y>"));

	while (1)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_LEFT)
			  return;
			else if (JoyStatus & JOY_RIGHT)
			  break;
		}
	}

	MAIN_WaitForJoyRelease();

	LCD_puts_f(WaitText);

	for (uint16_t EAddr = 0; EAddr < Sys_MagicNumber; EAddr++)
	  eeprom_write_byte_169(&EAddr, 0xFF);

	LCD_puts_f(PSTR("MEM CLEARED"));
	MAIN_Delay10MS(255);
}

void FUNCAutoCalib(void)
{
	LCD_puts_f(WaitText);
	OSCCAL_Calibrate();
}

void FUNCManCalib(void)
{
	uint8_t Buffer[9];

	JoyStatus = 1;                           // Invalid value to force the LCD to update
	
	USART_ENABLE(USART_TX_ON, USART_RX_OFF);

	while (1)
	{
		if (BuffElements)                    // Routine will also echo send chars (directly accesses the ringbuffer count var)
		   USART_Tx(BUFF_GetBuffByte());
	
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			  OSCCAL++;
			else if (JoyStatus & JOY_DOWN)
			  OSCCAL--;
			else if (JoyStatus & JOY_LEFT)
			  break;
					
			// Copy the programmer name out of memory and transmit it via the USART:
			strcpy_P(Buffer, ProgrammerName);
			USART_TxString(Buffer);

			Buffer[0] = 'C';
			Buffer[1] = 'V';
			Buffer[2] = ' ';

			MAIN_IntToStr(OSCCAL, &Buffer[3]);
			LCD_puts(Buffer);

			MAIN_WaitForJoyRelease();
		}
	}
	
	USART_ENABLE(USART_TX_OFF, USART_RX_OFF);
}

void FUNCSetContrast(void)
{
	uint8_t Buffer[6];
	uint8_t Contrast = (eeprom_read_byte_169(&Sys_LCDContrast) & 0x0F); // Ranges from 0-15 so mask retuns 15 on blank EEPROM (0xFF)
	
	JoyStatus = 1;                          // Invalid value to force the LCD to update
	
	while (1)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			{
				if (Contrast < 15)
				  Contrast++;
			}
			else if (JoyStatus & JOY_DOWN)
			{
				if (Contrast > 1)          // Zero is non-visible, so 1 is the minimum
				  Contrast--;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte_169(&Sys_LCDContrast, Contrast);
				return;
			}
					
			Buffer[0] = 'C';
			Buffer[1] = 'T';
			Buffer[2] = ' ';

			MAIN_IntToStr((uint16_t)Contrast, &Buffer[3]);
			LCD_puts(Buffer);

			LCD_CONTRAST_LEVEL(Contrast);

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCSetISPSpeed(void)
{
	JoyStatus = 1;                         // Invalid value to force the LCD to update

	uint8_t CurrSpeed = eeprom_read_byte_169(&Param_SCKDuration);

	if (CurrSpeed > (USI_PRESET_SPEEDS - 1)) CurrSpeed = 0; // Protection against blank EEPROM

	while (1)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			{
				(CurrSpeed == 0)? CurrSpeed = (USI_PRESET_SPEEDS - 1) : CurrSpeed--;
			}
			else if (JoyStatus & JOY_DOWN)
			{
				(CurrSpeed == (USI_PRESET_SPEEDS - 1))? CurrSpeed = 0 : CurrSpeed++;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte_169(&Param_SCKDuration, CurrSpeed);
				return;
			}
			
			// Show selected USI speed value onto the LCD:
			LCD_puts_f((uint8_t*)pgm_read_word(&USIPSNamePtrs[CurrSpeed]));

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCSleepMode(void)
{
	SMCR    = ((1 << SM1) | (1 << SE));   // Power down sleep mode
	LCDCRA &= ~(1 << LCDEN); 
	
	while (!(JoyStatus & JOY_UP))        // Joystick interrupt wakes the micro
	  SLEEP();
	   
	LCDCRA |= (1 << LCDEN);

	LCD_puts_f(WaitText);
	OSCCAL_Calibrate();	
	
	MAIN_WaitForJoyRelease();
}

void FUNCStorageInfo(void)
{
	uint8_t SelectedItem = 0;

	MAIN_WaitForJoyRelease();

	JoyStatus = 1;                         // Invalid value to force the LCD to update

	while (1)
	{
		if (JoyStatus)
		{
			if (JoyStatus & (JOY_UP | JOY_DOWN))
			{
				SelectedItem ^= 1;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				return;
			}
			else if (JoyStatus & JOY_PRESS)
			{
				if (SelectedItem == 1)    // View storage tags
				{
					DFSPIRoutinePointer = (SPIFuncPtr)SPI_SPITransmit;
					SPI_SPIInit();
					UseExernalDF = FALSE;
					DF_EnableDataflash(TRUE);

					if (DF_CheckCorrectOnboardChip())
					{
						TM_ShowTags();
						SPI_SPIOFF();
					}
					else if (!(PM_GetStoredDataSize(TYPE_FLASH)))
					{
						DF_EnableDataflash(FALSE);
						SPI_SPIOFF();

						MAIN_ShowError(PSTR("NO STORED PRGM"));
					}	
				}
				else                  // View stored data sizes
				{
					PM_ShowStoredItemSizes();
				}
			}
			
			LCD_puts_f((uint8_t*)pgm_read_word(&SIFOOptionPtrs[SelectedItem]));

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCGoBootloader(void)
{
	uint8_t MD = (MCUCR & ~(1 << JTD)); // Forces compiler to use IN, AND plus two OUTs rather than two lots of IN/AND/OUTs
	MCUCR = MD;  // Turn on JTAG via code
	MCUCR = MD;  // Twice as specified in datasheet        
	
	LCD_puts_f(PSTR("*JTAG ON*"));
	
	MAIN_WaitForJoyRelease();
	
	WDTCR = ((1<<WDCE) | (1<<WDE)); // Enable Watchdog Timer to give reset after minimum timeout
	while (1) {};                  // Eternal loop - when watchdog resets the AVR it will enter the bootloader
	                                // assuming the BOOTRST fuse is programmed
}
