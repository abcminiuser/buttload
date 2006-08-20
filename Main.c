/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
						

  Requires: AVR-GCC 3.4.3 or above, AVRLibC version 1.4.1 or above
*/

/*
	This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

/*
	FEATURES:
		
		1) Ability to emulate an AVRISP to allow the programming of AVRs with a computer
		   via the SPI interface
		
		2) Ability to store a program (including EEPROM, lockbit and fusebit data) in
		   non-volatile memory and be able to retrieve said program on user request and
		   thus program AVRs via ISP in the field
		
		3) Devices with larger than 16-bit flash addresses (such as the MEGA2560) are supported.
		
		4) Ability to read and program external AVR dataflash memory chips directly
					
			      Status Colour | Description
			      --------------+------------
			       Green        | Ready
			       Orange       | Busy
			       Red          | Programming
*/

/*
	KNOWN ISSUES:

		1) A maximum of 10 fuse bytes and 10 lock bytes can be stored in memory at any one
		   time (writing the same fuse overwrites the existing value). If it is attempted to
		   write more than this maximum, the extra bytes will be ignored.

		2) HEX files containing addresses which are non-consecutive (eg. use of .ORG command
		   in assembler other than .ORG 0) cannot be stored into ButtLoad's non-volatile memory.
		   Normal AVRISP mode is not affected by this issue.
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
		AVRISPCommandBytes.h                     | By Dean Camera
		AVRISPCommandInterpreter.c + Header file | By Dean Camera
		Dataflash.c + Header file                | By Dean Camera, re-coded from the generic dataflash
		                                         | code by Atmel (ported to GCC by Martin Thomas)
		DataflashCommandBytes.h                  | By Atmel, modified by Martin Thomas
		EEPROMVariables.h                        | By Dean Camera		
		ISPChipComm.c + Header file              | By Dean Camera
		ISRMacro.h                               | By Dean Camera
		JoystickInterrupt.S                      | By Dean Camera
		LCD_Driver.c + Header file               | By Dean Camera
		Main.c + Header file                     | By Dean Camera
		OSCCal.c + Header file                   | By Dean Camera, based on sample code by Colin Oflynn
		ProgramManager.c + Header file           | By Dean Camera
		RingBuff.c + Header file                 | By Dean Camera
		SPI.c + Header file                      | By Dean Camera
		Timeout.c + Header file                  | By Dean Camera
		USART.c + Header file                    | By Atmel, ported to GCC by Martin Thomas and
		                                         | modified by Dean Camera
		USI.c + Header file                      | By Dean Camera
		USITransfer.S                            | By Dean Camera with assistance from John Samperi 
		V2Protocol.c + Header file               | By Dean Camera
-------------------------------------------------+-----------------------------------------------------

   Special thanks to both Barry and Scott Coppersmith of AVRFreaks, for without their equipment and
   wisdom in debugging this monstrocity I'd still be working on it. Also thanks to the other members
   of AVRFreaks for their ideas.

--------------------------------------------------------------------------------------------------------

	Note: Almost all the 3rd party code has been modified in some way for this project. Please use the
	original code for each file if you intend to use any of the 3rd party code in you own project.
*/

#define INC_FROM_MAIN
#include "Main.h"

// PROGMEM CONSTANTS:
BUTTLOADTAG(Title,     "BUTTLOAD AVRISP");
BUTTLOADTAG(Version,   VERSION_VSTRING);
BUTTLOADTAG(Author,    "BY DEAN CAMERA");
BUTTLOADTAG(Copyright, "<C> 2005-2006");

#ifdef DEBUG_DBFUNCSON
  const uint8_t Func_ManualCalib[]       PROGMEM = "MAN OSCCAL SET";  
  
  #define DEBUGFUNCTIONS   , Func_ManualCalib
  #define DEBUGFUNCPTRS    , DFUNCManCalib
#else
  #define DEBUGFUNCTIONS
  #define DEBUGFUNCPTRS
#endif

const uint8_t*   AboutTextPtrs[]         PROGMEM = {BUTTTAG_Title.TagData, BUTTTAG_Version.TagData, BUTTTAG_Author.TagData, BUTTTAG_Copyright.TagData};

const uint8_t    WaitText[]              PROGMEM = "*WAIT*";

const uint8_t    Func_ISPPRGM[]          PROGMEM = "AVRISP MODE";
const uint8_t    Func_STOREPRGM[]        PROGMEM = "STORE PRGM";
const uint8_t    Func_PRGMAVR[]          PROGMEM = "PROGRAM AVR";
const uint8_t    Func_PRGMDATAFLASH[]    PROGMEM = "DATAFLASH PRGM";
const uint8_t    Func_PRGMSTOREINFO[]    PROGMEM = "DATASTORE INFO";
const uint8_t    Func_SETTINGS[]         PROGMEM = "SETTINGS";
const uint8_t    Func_SLEEP[]            PROGMEM = "SLEEP MODE";
	
const uint8_t*   MainFunctionNames[]     PROGMEM = {Func_ISPPRGM  , Func_STOREPRGM  , Func_PRGMAVR  , Func_PRGMDATAFLASH  , Func_PRGMSTOREINFO, Func_SETTINGS      , Func_SLEEP
                                                    DEBUGFUNCTIONS};
const FuncPtr    MainFunctionPtrs[]      PROGMEM = {FUNCAVRISPMode, FUNCStoreProgram, FUNCProgramAVR, FUNCProgramDataflash, FUNCStorageInfo   , FUNCChangeSettings , FUNCSleepMode
                                                    DEBUGFUNCPTRS};

const uint8_t    SFunc_SETCONTRAST[]     PROGMEM = "SET CONTRAST";
const uint8_t    SFunc_SETSPISPEED[]     PROGMEM = "SET ISP SPEED";
const uint8_t    SFunc_SETRESETMODE[]    PROGMEM = "SET RESET MODE";
const uint8_t    SFunc_SETFIRMMINOR[]    PROGMEM = "SET FIRM VERSION";
const uint8_t    SFunc_SETAUTOSLEEPTO[]  PROGMEM = "SET SLEEP TIMEOUT";
const uint8_t    SFunc_CLEARMEM[]        PROGMEM = "CLEAR SETTINGS";
const uint8_t    SFunc_GOBOOTLOADER[]    PROGMEM = "JUMP TO BOOTLOADER";

const uint8_t*   SettingFunctionNames[]  PROGMEM = {SFunc_SETCONTRAST, SFunc_SETSPISPEED, SFunc_SETRESETMODE, SFunc_SETFIRMMINOR , SFunc_SETAUTOSLEEPTO   , SFunc_CLEARMEM, SFunc_GOBOOTLOADER};
const FuncPtr    SettingFunctionPtrs[]   PROGMEM = {FUNCSetContrast  , FUNCSetISPSpeed  , FUNCSetResetMode  , FUNCSetFirmMinorVer, FUNCSetAutoSleepTimeOut, FUNCClearMem  , FUNCGoBootloader};

const uint8_t    PRG_A[]                 PROGMEM = "PRGM ALL";
const uint8_t    PRG_D[]                 PROGMEM = "DATA ONLY";
const uint8_t    PRG_E[]                 PROGMEM = "EEPROM ONLY";
const uint8_t    PRG_DE[]                PROGMEM = "DATA AND EEPROM";
const uint8_t    PRG_F[]                 PROGMEM = "FUSE BYTES ONLY";
const uint8_t    PRG_L[]                 PROGMEM = "LOCK BYTES ONLY";
const uint8_t    PRG_FL[]                PROGMEM = "FUSE AND LOCK BYTES";
const uint8_t    PRG_C[]                 PROGMEM = "ERASE ONLY";

const uint8_t*   ProgOptions[]           PROGMEM = {PRG_A, PRG_D, PRG_E, PRG_DE, PRG_F, PRG_L, PRG_FL, PRG_C};

const uint8_t    USISpeeds[USI_PRESET_SPEEDS][10]  PROGMEM = {"921600 HZ", "230400 HZ", " 57600 HZ", " 28800 HZ"};
const uint8_t    SPIResetModes[2][6]               PROGMEM = {"LOGIC", "FLOAT"};
const uint8_t    SIFONames[2][15]                  PROGMEM = {"STORAGE SIZES", "VIEW DATA TAGS"};

// GLOBAL VARIABLES:
volatile uint8_t JoyStatus;

// GLOBAL EEPROM VARIABLE STRUCT:
EEPROMVarsType EEPROMVars EEMEM;

// ======================================================================================

int main(void)
{	
	uint8_t CurrFunc = 0;

	#ifndef DEBUG_JTAGON
	    MCUCR   = (1 << JTD);                    // Turn off JTAG via code
	    MCUCR   = (1 << JTD);                    // Twice as specified in datasheet
	#endif

	ACSR    = (1 << ACD);                        // Disable the unused Analogue Comparitor to save power
	PRR     = ((1 << PRADC) | (1 << PRSPI) | (1 << PRUSART0)); // Disable subsystems (for now) to save power
	
	DDRF    = ((1 << 4) | (1 << 5));             // Set status LEDs as outputs
	DDRB    = ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 5)); // On-board dataflash /CS, ISP MOSI/SCK and beeper as outputs
	PORTB   = ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)   // Set SPI pins to high/pullups, and disable dataflash (send /CS high)
	        | JOY_BMASK);                        // \ Turn joystick
	PORTE   = JOY_EMASK;                         // /  pullups on
			  
	PCMSK0  = JOY_EMASK;                         // \.
	PCMSK1  = JOY_BMASK;                         // | Turn on joystick
	EIMSK   = ((1 << PCIE0) | (1 << PCIE1));     // |  interrupts
	EIFR    = ((1 << PCIF0) | (1 << PCIF1));     // /

	MAIN_SETSTATUSLED(MAIN_STATLED_ORANGE);      // Set status LEDs to orange (busy)
	MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE);     // Set target reset line to inactive

	sei();                                       // Enable interrupts

	LCD_Init();
	LCD_CONTRAST_LEVEL(0x0F);
	LCD_puts_f(WaitText);
	
	if (eeprom_read_byte(&EEPROMVars.MagicNumber) != MAGIC_NUM) // Check if first ButtLoad run
	{
		for (uint16_t EAddr = 0; EAddr < sizeof(EEPROMVars); EAddr++) // Clear the EEPROM if first run
		  eeprom_write_byte((uint8_t*)EAddr, 0xFF);

		eeprom_write_byte(&EEPROMVars.MagicNumber, MAGIC_NUM);
	}
	
	LCD_CONTRAST_LEVEL(eeprom_read_byte(&EEPROMVars.LCDContrast));
	DF_EnableDataflash(FALSE);                   // Pull internal Dataflash /CS high to disable it and thus save power
	OSCCAL_Calibrate();                          // Calibrate the internal RC occilator
	TOUT_SetupSleepTimer();                      // Set up and start the auto-sleep timer
	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);	     // Set status LEDs to green (ready)	
	
	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)                           // Joystick is in the non-center position
		{
			if (JoyStatus & JOY_UP)              // Previous function
			  (CurrFunc == 0)? CurrFunc = ARRAY_UPPERBOUND(MainFunctionPtrs): CurrFunc--;
			else if (JoyStatus & JOY_DOWN)       // Next function
			  (CurrFunc == ARRAY_UPPERBOUND(MainFunctionPtrs))? CurrFunc = 0 : CurrFunc++;
			else if (JoyStatus & JOY_PRESS)      // Select current function
			  ((FuncPtr)pgm_read_word(&MainFunctionPtrs[CurrFunc]))(); // Run associated function
			else if (JoyStatus & JOY_RIGHT)      // About ButtLoad list
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

void MAIN_ResetCSLine(const uint8_t ActiveInactive)
{
	/* ActiveInactive controls the /Reset line to an AVR device or external dataflash
	   /CS line. If the reset polarity parameter is a 0 then interfacing with AT89
	   devices which has an active high reset. Pins are tristated when inactive unless
	   the SPIResetMode option is changed in the OPTIONS mode.                         */
	
	switch (ActiveInactive)
	{
		case MAIN_RESETCS_ACTIVE:                // The target RESET line may be either active high or low.
			DDRF |= (1 << 6);
		
			if (eeprom_read_byte(&EEPROMVars.ResetPolarity)) // Translate to correct active logic level for target device type
			  PORTF &= ~(1 << 6);
			else
			  PORTF |=  (1 << 6);
		
			break;
		case MAIN_RESETCS_EXTDFACTIVE:           // Dataflash chips are always active low.
			DDRF  |=  (1 << 6);
			PORTF &= ~(1 << 6);
			
			break;
		case MAIN_RESETCS_INACTIVE:              // Must determine what to do for inactive RESET.
			if (eeprom_read_byte(&EEPROMVars.SPIResetMode)) // FLOAT mode reset
			{
				DDRF  &= ~(1 << 6);
				PORTF &= ~(1 << 6);
			}
			else                                 // ACTIVE mode reset
			{
				if (eeprom_read_byte(&EEPROMVars.ResetPolarity)) // Translate to correct inactive logic level for target device type
				  PORTF |=  (1 << 6);
				else
				  PORTF &= ~(1 << 6);			
			}
	}
}

void MAIN_WaitForJoyRelease(void)
{
	if (JoyStatus == JOY_INVALID)                // If invalid value used to force menu drawing, reset value and exit
	{
		JoyStatus = 0;
		return;
	}

	for (;;)
	{
		while (JoyStatus) {};                    // Wait until joystick released

		MAIN_Delay10MS(2);

		if (!(JoyStatus))                        // Joystick still released (not bouncing), return
		{
			MAIN_Delay10MS(15);
			return;
		}
	}
}

void MAIN_IntToStr(uint16_t IntV, uint8_t* Buff)
{
	// Shows leading zeros, unlike itoa.
	// Maximum value which can be converted is 999.

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

void MAIN_ShowProgType(const uint8_t Letter)
{
	uint8_t ProgTypeBuffer[7];

	strcpy_P(ProgTypeBuffer, PSTR("PRG>  "));
	ProgTypeBuffer[5] = Letter;
	
	LCD_puts(ProgTypeBuffer);
}

void MAIN_ShowError(const uint8_t *pFlashStr)
{
	uint8_t ErrorBuff[LCD_TEXTBUFFER_SIZE];      // New buffer, LCD text buffer size
	
	ErrorBuff[0] = 'E';
	ErrorBuff[1] = '>';

	strcpy_P(&ErrorBuff[2], pFlashStr);          // WARNING: If flash error text is larger than (TEXTBUFFER_SIZE - 2),
	                                             // this will overflow the buffer and crash the program!
	LCD_puts(ErrorBuff);
	
	MAIN_WaitForJoyRelease();
	while (!(JoyStatus & JOY_PRESS)) {};
	MAIN_WaitForJoyRelease();
}

// ======================================================================================

ISR(PCINT1_vect, ISR_NOBLOCK)                    // Joystick routine; PCINT0_vect is bound to this also via JoystickInterrupt.S
{
	JoyStatus = (~PINB & JOY_BMASK)
	          | (~PINE & JOY_EMASK);
			  
	TIMEOUT_SLEEP_TIMEOUT_RESET();
}

ISR(BADISR_vect, ISR_NAKED)                      // Bad ISR routine; should never be called, here for safety
{
	SPI_SPIOFF();
	USI_SPIOff();
	USART_OFF();
	TIMEOUT_PACKET_TIMER_OFF();
	TIMEOUT_SLEEP_TIMER_OFF();

	MAIN_SETSTATUSLED(MAIN_STATLED_RED);

	MAIN_ShowError(PSTR("BADISR"));
		
	for (;;) {};
}

// ======================================================================================

void FUNCChangeSettings(void)
{
	uint8_t CurrSFunc = 0;
	
	MAIN_WaitForJoyRelease();

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)                           // Joystick is in the non-center position
		{
			if (JoyStatus & JOY_UP)              // Previous function
			  (CurrSFunc == 0)? CurrSFunc = ARRAY_UPPERBOUND(SettingFunctionPtrs) : CurrSFunc--;
			else if (JoyStatus & JOY_DOWN)       // Next function
			  (CurrSFunc == ARRAY_UPPERBOUND(SettingFunctionPtrs))? CurrSFunc = 0 : CurrSFunc++;
			else if (JoyStatus & JOY_PRESS)      // Select current function
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
	
	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD			
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			  (InfoNum == 0)? InfoNum = ARRAY_UPPERBOUND(AboutTextPtrs) : InfoNum--;
			else if (JoyStatus & JOY_DOWN)
			  (InfoNum == ARRAY_UPPERBOUND(AboutTextPtrs))? InfoNum = 0 : InfoNum++;
			else if (JoyStatus & JOY_LEFT)
			  return;

			LCD_puts_f((uint8_t*)pgm_read_word(&AboutTextPtrs[InfoNum]));

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCAVRISPMode(void)
{
	USART_Init();
	LCD_puts_f(AVRISPModeMessage);
	
	V2P_RunStateMachine(AICI_InterpretPacket);
}

void FUNCProgramDataflash(void)
{
	USI_SPIInitMaster();
	DataflashInfo.UseExernalDF = TRUE;
	DataflashInfo.DFSPIRoutinePointer = USI_SPITransmit;
	
	USART_Init();
	LCD_puts_f(DataFlashProgMode);

	V2P_RunStateMachine(PD_InterpretAVRISPPacket);
	   
	DF_EnableDataflash(FALSE);
	SPI_SPIOFF();
}

void FUNCProgramAVR(void)
{
	uint8_t  DoneFailMessageBuff[12];
	uint8_t  Fault    = ISPCC_NO_FAULT;
	uint8_t  ProgMode = 0;
	uint8_t  StoredLocksFuses;

	MAIN_WaitForJoyRelease();
	
	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_LEFT)
			  return;
			else if (JoyStatus & JOY_PRESS)
			  break;
			else if (JoyStatus & JOY_UP)
			  (ProgMode == 0)? ProgMode = ARRAY_UPPERBOUND(ProgOptions) : ProgMode--;
			else if (JoyStatus & JOY_DOWN)
			  (ProgMode == ARRAY_UPPERBOUND(ProgOptions))? ProgMode = 0 : ProgMode++;

			LCD_puts_f((uint8_t*)pgm_read_word(&ProgOptions[ProgMode])); // Show current function onto the LCD

			MAIN_WaitForJoyRelease();
		}
	}

	LCD_puts_f(WaitText);
	SPI_SPIInit();
	DataflashInfo.UseExernalDF = FALSE;
	DataflashInfo.DFSPIRoutinePointer = SPI_SPITransmit;
	
	if (!(DF_CheckCorrectOnboardChip()))
	  return;

	TIMEOUT_SLEEP_TIMER_OFF();

	USI_SPIInitMaster();
	MAIN_ResetCSLine(MAIN_RESETCS_ACTIVE);       // Capture the RESET line of the slave AVR
			
	for (uint8_t PacketB = 0; PacketB < 12; PacketB++) // Read the enter programming mode command bytes
	  PacketBytes[PacketB] = eeprom_read_byte(&EEPROMVars.EnterProgMode[PacketB]);
	
	ISPCC_EnterChipProgrammingMode();            // Try to sync with the slave AVR

	CurrAddress = 0;

	if (PacketBytes[1] == AICB_STATUS_CMD_OK)    // ISPCC_EnterChipProgrammingMode alters the PacketBytes buffer rather than returning a value
	{						
		if ((ProgMode == 0) || (ProgMode == 7) || (ProgMode == 1) || (ProgMode == 3)) // All, erase chip, flash and eeprom, or program flash mode
		{
			MAIN_ShowProgType('C');
			
			if (!(eeprom_read_byte(&EEPROMVars.EraseCmdStored) == TRUE))
			{
				Fault = ISPCC_FAULT_NOERASE;
				MAIN_ShowError(PSTR("NO ERASE CMD"));
			}
			else
			{
				PM_SendEraseCommand();
			}
		}

		if (((ProgMode == 0) || (ProgMode == 1) || (ProgMode == 3)) && (Fault != ISPCC_FAULT_NOERASE)) // All, flash and EEPROM, or program flash mode
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
	
		if ((ProgMode == 0) || (ProgMode == 2) || (ProgMode == 3)) // All, flash and EEPROM, or program EEPROM mode
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

		if ((ProgMode == 0) || (ProgMode == 4) || (ProgMode == 6)) // All, fuse and lock bytes, or program fuse bytes mode
		{
			MAIN_ShowProgType('F');
			
			StoredLocksFuses = eeprom_read_byte(&EEPROMVars.TotalFuseBytes);
			if (!(StoredLocksFuses) || (StoredLocksFuses != 0xFF))
			{
				Fault = ISPCC_FAULT_NODATATYPE;					
				MAIN_ShowError(PSTR("NO FUSE BYTES"));
			}
			else
			{
				PM_SendFuseLockBytes(TYPE_FUSE);
			}
		}

		if ((ProgMode == 0) || (ProgMode == 5) || (ProgMode == 6)) // All, fuse and lock bytes, or program lock bytes mode
		{
			if (ProgMode == 6)                           // If fusebytes have already been written, we need to re-enter programming mode to latch them
			{
				MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE); // Release the RESET line of the slave AVR
				MAIN_Delay10MS(1);
				MAIN_ResetCSLine(MAIN_RESETCS_ACTIVE);   // Capture the RESET line of the slave AVR
				ISPCC_EnterChipProgrammingMode();        // Try to sync with the slave AVR
			}

			MAIN_ShowProgType('L');
		
			StoredLocksFuses = eeprom_read_byte(&EEPROMVars.TotalLockBytes);
			if (!(StoredLocksFuses) || (StoredLocksFuses != 0xFF))
			{
				Fault = ISPCC_FAULT_NODATATYPE;
				MAIN_ShowError(PSTR("NO LOCK BYTES"));
			}
			else
			{
				PM_SendFuseLockBytes(TYPE_LOCK);
			}
		}

		strcpy_P(DoneFailMessageBuff, PSTR("PROG DONE"));

		if (Fault != ISPCC_NO_FAULT)              // Takes less code to just overwrite part of the string on fail
		  strcpy_P(&DoneFailMessageBuff[5], PSTR("FAILED"));

		LCD_puts(DoneFailMessageBuff);

		MAIN_Delay10MS(225);
	}
	else
	{
		MAIN_ShowError(SyncErrorMessage);
	}
	
	TOUT_SetupSleepTimer();
	MAIN_ResetCSLine(MAIN_RESETCS_INACTIVE);     // Release the RESET line and allow the slave AVR to run	
	USI_SPIOff();
	DF_EnableDataflash(FALSE);
	SPI_SPIOFF();
	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);       // Set status LEDs to green (ready)
}

void FUNCStoreProgram(void)
{
	SPI_SPIInit();
	DataflashInfo.UseExernalDF = FALSE;
	DataflashInfo.DFSPIRoutinePointer = SPI_SPITransmit;
	DF_EnableDataflash(TRUE);

	if (!(DF_CheckCorrectOnboardChip()))
	  return;
			
	USART_Init();
	LCD_puts_f(PSTR("*STORAGE MODE*"));

	V2P_RunStateMachine(PM_InterpretAVRISPPacket);
	
	DF_EnableDataflash(FALSE);
	SPI_SPIOFF();
}

void FUNCClearMem(void)
{
	LCD_puts_f(PSTR("CONFIRM"));
	MAIN_Delay10MS(180);

	LCD_puts_f(PSTR("<N Y>"));

	for (;;)
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
	MAIN_SETSTATUSLED(MAIN_STATLED_ORANGE);      // Set status LEDs to orange (busy)

	for (uint16_t EAddr = 0; EAddr < sizeof(EEPROMVars); EAddr++)
	  eeprom_write_byte((uint8_t*)EAddr, 0xFF);
	
	eeprom_write_byte(&EEPROMVars.MagicNumber, MAGIC_NUM);

	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);       // Set status LEDs to green (ready)
	LCD_puts_f(PSTR("MEM CLEARED"));
	MAIN_Delay10MS(250);
}

void FUNCSetContrast(void)
{
	uint8_t Buffer[6];
	uint8_t Contrast = (eeprom_read_byte(&EEPROMVars.LCDContrast) & 0x0F); // Ranges from 0-15 so mask retuns 15 on blank EEPROM (0xFF)
	
	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
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
				if (Contrast > 1)                // Zero is non-visible, so 1 is the minimum
				  Contrast--;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte(&EEPROMVars.LCDContrast, Contrast);
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
	uint8_t CurrSpeed = eeprom_read_byte(&EEPROMVars.SCKDuration);

	if (CurrSpeed > ARRAY_UPPERBOUND(USISpeeds))
	  CurrSpeed = 0;                             // Protection against blank EEPROM

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			{
				(CurrSpeed == 0)? CurrSpeed = ARRAY_UPPERBOUND(USISpeeds) : CurrSpeed--;
			}
			else if (JoyStatus & JOY_DOWN)
			{
				(CurrSpeed == ARRAY_UPPERBOUND(USISpeeds))? CurrSpeed = 0 : CurrSpeed++;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte(&EEPROMVars.SCKDuration, CurrSpeed);
				return;
			}
			
			// Show selected USI speed value onto the LCD:
			LCD_puts_f(USISpeeds[CurrSpeed]);

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCSetResetMode(void)
{
	uint8_t CurrMode = (eeprom_read_byte(&EEPROMVars.SPIResetMode) & 0x01);

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & (JOY_UP | JOY_DOWN))
			{
				CurrMode ^= 1;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte(&EEPROMVars.SPIResetMode, CurrMode);
				return;
			}
			
			// Show selected USI speed value onto the LCD:
			LCD_puts_f(SPIResetModes[CurrMode]);

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCSetFirmMinorVer(void)
{
	uint8_t VerBuffer[5];
	uint8_t VerMinor = eeprom_read_byte(&EEPROMVars.FirmVerMinor);

	if (VerMinor > 9)
	  VerMinor = V2P_SW_VERSION_MINOR_DEFAULT;
	
	strcpy_P(VerBuffer, PSTR("V2- "));

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			{
				if (VerMinor < 9)
				  VerMinor++;
			}
			if (JoyStatus & JOY_DOWN)
			{
				if (VerMinor)
				  VerMinor--;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte(&EEPROMVars.FirmVerMinor, VerMinor);
				return;
			}
			
			VerBuffer[3] = '0' + VerMinor;
			LCD_puts(VerBuffer);

			MAIN_WaitForJoyRelease();
		}
	}	
}

void FUNCSetAutoSleepTimeOut(void)
{
	uint8_t SleepTxtBuffer[8];
	uint8_t SleepVal = eeprom_read_byte(&EEPROMVars.AutoSleepValIndex);

	if (SleepVal > ARRAY_UPPERBOUND(AutoSleepTOValues))
	  SleepVal = ARRAY_UPPERBOUND(AutoSleepTOValues);

	strcpy_P(SleepTxtBuffer, PSTR("    SEC"));
	
	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			{
				(SleepVal == 0)? SleepVal = ARRAY_UPPERBOUND(AutoSleepTOValues) : SleepVal--;
			}
			if (JoyStatus & JOY_DOWN)
			{
				(SleepVal == ARRAY_UPPERBOUND(AutoSleepTOValues))? SleepVal = 0 : SleepVal++;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte(&EEPROMVars.AutoSleepValIndex, SleepVal);
				TOUT_SetupSleepTimer();
				return;
			}

			if (!(SleepVal))
			{
				LCD_puts_f(PSTR("OFF"));
			}
			else
			{
				MAIN_IntToStr(pgm_read_byte(&AutoSleepTOValues[SleepVal]), &SleepTxtBuffer[0]);
				SleepTxtBuffer[3] = ' ';         // Remove the auto-string termination from the buffer
				LCD_puts(SleepTxtBuffer);
			}

			MAIN_WaitForJoyRelease();
		}
	}	
}

void FUNCSleepMode(void)
{
	LCDCRA &= ~(1 << LCDEN);                     // Turn off LCD driver while sleeping
	LCDCRA |=  (1 << LCDBL);                     // Blank LCD to discharge all segments
	PRR    |=  (1 << PRLCD);                     // Enable LCD power reduction bit

	MAIN_SETSTATUSLED(MAIN_STATLED_OFF);         // Save battery power - turn off status LED

	SMCR    = ((1 << SM1) | (1 << SE));          // Power down sleep mode
	while (!(JoyStatus & JOY_UP))                // Joystick interrupt wakes the micro
	  SLEEP();
	   
	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);       // Turn status LED back on

	PRR    &= ~(1 << PRLCD);                     // Disable LCD power reduction bit
	LCDCRA &= ~(1 << LCDBL);                     // Un-blank LCD to enable all segments
	LCDCRA |=  (1 << LCDEN);                     // Re-enable LCD driver
	
	MAIN_WaitForJoyRelease();
}

void FUNCStorageInfo(void)
{
	uint8_t SelectedItem = 0;

	MAIN_WaitForJoyRelease();

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
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
				if (SelectedItem == 1)           // View storage tags
				{
					SPI_SPIInit();
					DataflashInfo.UseExernalDF = FALSE;
					DataflashInfo.DFSPIRoutinePointer = (SPIFuncPtr)SPI_SPITransmit;
					DF_EnableDataflash(TRUE);

					if (!(PM_GetStoredDataSize(TYPE_FLASH)))
					  MAIN_ShowError(PSTR("NO STORED PRGM"));
					else if (DF_CheckCorrectOnboardChip())
					  TM_ShowTags();

					DF_EnableDataflash(FALSE);
					SPI_SPIOFF();
				}
				else                             // View stored data sizes
				{
					PM_ShowStoredItemSizes();
				}
			}
			
			LCD_puts_f(SIFONames[SelectedItem]);

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCGoBootloader(void)
{
	volatile uint8_t MD = (MCUCR & ~(1 << JTD)); // Forces compiler to use IN, AND plus two OUTs rather than two lots of IN/AND/OUTs
	MCUCR = MD;                                  // Turn on JTAG via code
	MCUCR = MD; 
	
	TIMEOUT_SLEEP_TIMER_OFF();
	
	LCD_puts_f(PSTR("*JTAG ON*"));
	
	MAIN_WaitForJoyRelease();
	
	WDTCR = ((1<<WDCE) | (1<<WDE));              // Enable Watchdog Timer to give reset after minimum timeout
	for (;;) {};                                 // Eternal loop - when watchdog resets the AVR it will enter the bootloader,
	                                             // assuming the BOOTRST fuse is programmed (ptherwise app will just restart)
}

#ifdef DEBUG_DBFUNCSON
void DFUNCManCalib(void)
{
	uint8_t Buffer[16];

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	USART_Init();

	for (;;)
	{
		if (BuffElements)                        // Routine will also echo send chars (directly accesses the ringbuffer count var)
		  USART_Tx(BUFF_GetBuffByte());
	
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			  OSCCAL++;
			else if (JoyStatus & JOY_DOWN)
			  OSCCAL--;
			else if (JoyStatus & JOY_LEFT)
			  break;
					
			// Copy the programmer name out of memory and transmit it several times via the USART:
			strcpy_P(Buffer, BUTTTAG_Title.TagData);
			
			for (uint8_t SendLoop = 0; SendLoop < 18; SendLoop++)
			{
				for (uint8_t BByte = 0; BByte < 15; BByte++)
				  USART_Tx(Buffer[BByte]);
				
				USART_Tx(' ');
				USART_Tx(' ');
			}

			Buffer[0] = 'C';
			Buffer[1] = 'V';
			Buffer[2] = ' ';

			MAIN_IntToStr(OSCCAL, &Buffer[3]);
			LCD_puts(Buffer);

			MAIN_WaitForJoyRelease();
		}
	}
	
	USART_OFF();
}
#endif
