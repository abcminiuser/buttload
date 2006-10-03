/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
						

  Requires: AVR-GCC 3.4.3 or above, AVRLibC version 1.4.1 or above
*/

/*
	This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	General Public License for more details.
*/

/*
	FEATURES:
		
		1) Ability to emulate an AVRISP to allow the programming of AVRs with a computer
		   via the SPI interface
		
		2) Ability to store a program (including EEPROM, lockbit and fusebit data) in
		   non-volatile memory and be able to retrieve said program on user request and
		   thus program AVRs via ISP in the field
		
		3) Devices with larger than 16-bit flash addresses (such as the MEGA2560) are supported.
					
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
			Pin 3  (PF6) - /RESET line of slave AVR
			Pin 9  (PF7) - Butterfly battery voltage detection (optional)
			Pin 10 (GND) - Status LED ground if used (optional)			
			
		USART Interface
			Pin 1        - Recieve relative to Butterfly
			Pin 2        - Transmit relative to Butterfly
			Pin 3        - Ground

		External Vin Interface
			Pin 1        - External sleep input (active high)
			Pin 2        - Ground

	 * Level shifting circuitry must be employed that can translate the 3.3V Butterfly
	 signals to the target AVR's voltage and vice-versa AT SUFFICIENT CURRENT.
*/

/*
	AUTHOR CREDIT:

		FILE                                     |  AUTHOR
		-----------------------------------------+---------------------------------------------
		Analogue.c + Header file                 | By Dean Camera
		AVRISPCommandBytes.h                     | By Dean Camera
		AVRISPCommandInterpreter.c + Header file | By Dean Camera
		ButtLoadTag.h                            | By Dean Camera
		Dataflash.c + Header file                | By Dean Camera, re-coded from the generic dataflash
		                                         | code by Atmel (ported to GCC by Martin Thomas)
		DataflashCommandBytes.h                  | By Atmel, modified by Martin Thomas
		EEPROMVariables.h                        | By Dean Camera
		GlobalMacros.h                           | By Dean Camera
		ISPChipComm.c + Header file              | By Dean Camera
		ISRMacro.h                               | By Dean Camera
		JoystickInterrupt.S                      | By Dean Camera
		LCD_Driver.c + Header file               | By Dean Camera
		Main.c + Header file                     | By Dean Camera
		OSCCal.c + Header file                   | By Dean Camera, based on sample code by Colin Oflynn
		ProgramManager.c + Header file           | By Dean Camera
		RingBuff.c + Header file                 | By Dean Camera
		SPI.c + Header file                      | By Dean Camera
		StorageManager.c + Header file           | By Dean Camera
		TagManager.c + Header file               | By Dean Camera
		Timeout.c + Header file                  | By Dean Camera
		ToneGeneration.c + Header file           | By Dean Camera
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

const char*   AboutTextPtrs[]                   PROGMEM = {BUTTTAG_Title.TagData, BUTTTAG_Version.TagData, BUTTTAG_Author.TagData, BUTTTAG_Copyright.TagData};

const char    WaitText[]                        PROGMEM = "*WAIT*";
const char    OffText[]                         PROGMEM = "OFF";

const char    Func_ISPPRGM[]                    PROGMEM = "AVRISP MODE";
const char    Func_STOREPRGM[]                  PROGMEM = "STORE PRGM";
const char    Func_PRGMAVR[]                    PROGMEM = "PROGRAM AVR";
const char    Func_PRGMSTOREINFO[]              PROGMEM = "DATAFLASH STATS";
const char    Func_SETTINGS[]                   PROGMEM = "SETTINGS";
const char    Func_SLEEP[]                      PROGMEM = "SLEEP MODE";
	
const char*   MainFunctionNames[]               PROGMEM = {Func_ISPPRGM  , Func_STOREPRGM  , Func_PRGMAVR  , Func_PRGMSTOREINFO, Func_SETTINGS      , Func_SLEEP};
const FuncPtr MainFunctionPtrs[]                PROGMEM = {FUNCAVRISPMode, FUNCStoreProgram, FUNCProgramAVR, FUNCStorageInfo   , FUNCChangeSettings , FUNCSleepMode};

const char    SFunc_SETCONTRAST[]               PROGMEM = "SET CONTRAST";
const char    SFunc_SETSPISPEED[]               PROGMEM = "SET ISP SPEED";
const char    SFunc_SETRESETMODE[]              PROGMEM = "SET RESET MODE";
const char    SFunc_SETFIRMMINOR[]              PROGMEM = "SET FIRM VERSION";
const char    SFunc_SETAUTOSLEEPTO[]            PROGMEM = "SET SLEEP TIMEOUT";
const char    SFunc_SETTONEVOL[]                PROGMEM = "SET TONE VOLUME";
const char    SFunc_SETSTARTUP[]                PROGMEM = "SET STARTUP MODE";
const char    SFunc_CLEARMEM[]                  PROGMEM = "CLEAR SETTINGS";
const char    SFunc_GOBOOTLOADER[]              PROGMEM = "JUMP TO BOOTLOADER";

const char*   SettingFunctionNames[]            PROGMEM = {SFunc_SETCONTRAST, SFunc_SETSPISPEED, SFunc_SETRESETMODE, SFunc_SETFIRMMINOR , SFunc_SETAUTOSLEEPTO   , SFunc_SETTONEVOL, SFunc_SETSTARTUP  , SFunc_CLEARMEM, SFunc_GOBOOTLOADER};
const FuncPtr SettingFunctionPtrs[]             PROGMEM = {FUNCSetContrast  , FUNCSetISPSpeed  , FUNCSetResetMode  , FUNCSetFirmMinorVer, FUNCSetAutoSleepTimeOut, FUNCSetToneVol  , FUNCSetStartupMode, FUNCClearMem  , FUNCGoBootloader};

const char    USISpeeds[USI_PRESET_SPEEDS][10]  PROGMEM = {"921600 HZ", "230400 HZ", " 57600 HZ", " 28800 HZ"};
const char    SPIResetModes[2][6]               PROGMEM = {"LOGIC", "FLOAT"};
const char    SIFONames[2][15]                  PROGMEM = {"STORAGE SIZES", "VIEW DATA TAGS"};
const char    ProgramAVROptions[2][8]           PROGMEM = {"START", "OPTIONS"};
const char    StartupModes[3][11]               PROGMEM = {"NORMAL", "PRODUCTION", "AVRISP"};

// GLOBAL EEPROM VARIABLE STRUCT:
EEPROMVarsType EEPROMVars EEMEM;

// ======================================================================================

int main(void)
{	
	uint8_t CurrFunc    = 0;
	uint8_t StartupMode = eeprom_read_byte(&EEPROMVars.StartupMode);

	#ifndef DEBUG_JTAGON
	    MCUCR   = (1 << JTD);                    // Turn off JTAG via code
	    MCUCR   = (1 << JTD);                    // Twice as specified in datasheet
	#endif

	ACSR    = (1 << ACD);                        // Disable the unused Analogue Comparitor to save power
	PRR     = ((1 << PRADC) | (1 << PRSPI) | (1 << PRUSART0)); // Disable subsystems (for now) to save power
	
	DDRF    = ((1 << 4) | (1 << 5) | (1 << 3));  // Set status LEDs and VCP as outputs, rest as inputs
	DDRB    = ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 5)); // On-board dataflash /CS, ISP MOSI/SCK and beeper as outputs
	PORTB   = ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)   // Set SPI pins to high/pullups, and disable dataflash (send /CS high)
	        | JOY_BMASK);                        // \ Turn joystick
	PORTE   = JOY_EMASK;                         // /  pullups on
	PORTF   = (1 << 7);                          // Enable PORTF pullup on Bat Detect pin
		  
	PCMSK0  = JOY_EMASK;                         // \.
	PCMSK1  = JOY_BMASK;                         // | Turn on joystick
	EIMSK   = ((1 << PCIE0) | (1 << PCIE1));     // | interrupts
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
	
	TONEGEN_GET_TONE_VOL();
	TG_PlayToneSeq(TONEGEN_SEQ_STARTUP);         // Play startup tone

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD

	if (StartupMode == 1)                        // Check if production startup mode
	  FUNCProgramAVR();
	else if (StartupMode == 2)                   // Check if AVRISP startup mode
	  FUNCAVRISPMode();

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
			LCD_puts_f((char*)pgm_read_word(&MainFunctionNames[CurrFunc]));

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
	/* ActiveInactive controls the /Reset line to the target AVR device. If the reset polarity parameter
	   is a 0 then Buttload is interfacing with AT89x devices (which have an active high rather than an
	   active low reset) and this is managed automaticaly. The reset pin is tristated when inactive unless
	   the SPIResetMode option is changed in the OPTIONS mode.                                             */
	
	switch (ActiveInactive)
	{
		case MAIN_RESETCS_ACTIVE:                // The target RESET line may be either active high or low.
			DDRF |= (1 << 6);
		
			if (eeprom_read_byte(&EEPROMVars.ResetPolarity)) // Translate to correct active logic level for target device type
			  PORTF &= ~(1 << 6);
			else
			  PORTF |=  (1 << 6);
		
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

void MAIN_IntToStr(uint16_t IntV, char* Buff)
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
	char ProgTypeBuffer[6];

	strcpy_P(ProgTypeBuffer, PSTR("PRG>  "));
	ProgTypeBuffer[5] = Letter;
	
	LCD_puts(ProgTypeBuffer);
}

void MAIN_ShowError(const char *pFlashStr)
{
	char ErrorBuff[LCD_TEXTBUFFER_SIZE + 2];     // New buffer, LCD text buffer size plus space for the "E>" prefix
	
	ErrorBuff[0] = 'E';
	ErrorBuff[1] = '>';

	strcpy_P(&ErrorBuff[2], pFlashStr);          // WARNING: If flash error text is larger than TEXTBUFFER_SIZE,
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
			LCD_puts_f((char*)pgm_read_word(&SettingFunctionNames[CurrSFunc]));

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

			LCD_puts_f((char*)pgm_read_word(&AboutTextPtrs[InfoNum]));

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

void FUNCProgramAVR(void)
{
	uint8_t ProgMode = 0;
	
	MAIN_WaitForJoyRelease();

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_LEFT)
			{
				return;
			}
			else if (JoyStatus & JOY_PRESS)
			{
				if (ProgMode == 1)
				  PM_ChooseProgAVROpts();
				else
				  PM_StartProgAVR();
			}
			else if (JoyStatus & (JOY_UP | JOY_DOWN))
			{
				ProgMode ^= 1;
			}

			LCD_puts_f(ProgramAVROptions[ProgMode]);

			MAIN_WaitForJoyRelease();
		}
	}
}

void FUNCStoreProgram(void)
{
	SPI_SPIInit();
	DF_EnableDataflash(TRUE);

	if (!(DF_CheckCorrectOnboardChip()))
	  return;
			
	USART_Init();
	LCD_puts_f(PSTR("*STORAGE MODE*"));

	V2P_RunStateMachine(SM_InterpretAVRISPPacket);
	
	DF_EnableDataflash(FALSE);
	SPI_SPIOFF();
}

void FUNCClearMem(void)
{
	LCD_puts_f(PSTR("CONFIRM"));
	MAIN_Delay10MS(180);

	LCD_puts_f(PSTR("<N Y>"));

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD	
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
	uint8_t Contrast = (eeprom_read_byte(&EEPROMVars.LCDContrast) & 0x0F); // Ranges from 0-15 so mask retuns 15 on blank EEPROM (0xFF)
	char Buffer[6];
	
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
	uint8_t VerMinor = eeprom_read_byte(&EEPROMVars.FirmVerMinor);
	char    VerBuffer[5];

	if (VerMinor > 9)
	  VerMinor = V2P_SW_VERSION_MINOR_DEFAULT;
	
	VerBuffer[0] = 'V';
	VerBuffer[1] = '2';

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			{
				if (VerMinor < 20)
				  VerMinor++;
			}
			else if (JoyStatus & JOY_DOWN)
			{
				if (VerMinor)
				  VerMinor--;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte(&EEPROMVars.FirmVerMinor, VerMinor);
				return;
			}
			
			MAIN_IntToStr(VerMinor, &VerBuffer[2]);
			VerBuffer[2] = '-';
			LCD_puts(VerBuffer);

			MAIN_WaitForJoyRelease();
		}
	}	
}

void FUNCSetAutoSleepTimeOut(void)
{
	uint8_t SleepVal = eeprom_read_byte(&EEPROMVars.AutoSleepValIndex);
	char    SleepTxtBuffer[8];

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
				LCD_puts_f(OffText);
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

void FUNCSetToneVol(void)
{
	char VolBuffer[5];

	VolBuffer[0] = 'V';
	
	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			uint8_t ToneVolLcl = ToneVol;        // Copy the global to a local variable to save space

			if (JoyStatus & JOY_UP)
			{
				if (ToneVolLcl < 80)
				  ToneVolLcl += 8;
				else
				  ToneVolLcl  = 0;				
			}
			else if (JoyStatus & JOY_DOWN)
			{
				if (ToneVolLcl)
				  ToneVolLcl -= 8;
				else
				  ToneVolLcl  = 80;
			}
			else if (JoyStatus & JOY_LEFT)
			{
				eeprom_write_byte(&EEPROMVars.ToneVolume, ToneVolLcl);
				return;
			}

			ToneVol = ToneVolLcl;            // Copy the local value back into the global

			if (!(ToneVolLcl))
			{
				LCD_puts_f(OffText);
			}
			else
			{
				TG_PlayToneSeq(TONEGEN_SEQ_VOLTEST);
				MAIN_IntToStr((ToneVol >> 3), &VolBuffer[1]);
				LCD_puts(VolBuffer);				
			}

			MAIN_WaitForJoyRelease();
		}
	}	
}

void FUNCSetStartupMode(void)
{
	uint8_t StartupMode = eeprom_read_byte(&EEPROMVars.StartupMode);

	if (StartupMode > ARRAY_UPPERBOUND(StartupModes))
	  StartupMode = 0;

	JoyStatus = JOY_INVALID;                     // Use an invalid joystick value to force the program to write the
	                                             // name of the default command onto the LCD
	for (;;)
	{
		if (JoyStatus)
		{
			if (JoyStatus & JOY_UP)
			  (StartupMode == 0)? StartupMode = ARRAY_UPPERBOUND(StartupModes) : StartupMode--;
			else if (JoyStatus & JOY_DOWN)
			  (StartupMode == ARRAY_UPPERBOUND(StartupModes))? StartupMode = 0 : StartupMode++;
			else if (JoyStatus & JOY_LEFT)
			  break;

			LCD_puts_f(StartupModes[StartupMode]);			

			MAIN_WaitForJoyRelease();
		}
	}	

	eeprom_write_byte(&EEPROMVars.StartupMode, StartupMode);
}

void FUNCSleepMode(void)
{
	LCDCRA &= ~(1 << LCDEN);                     // Turn off LCD driver while sleeping
	LCDCRA |=  (1 << LCDBL);                     // Blank LCD to discharge all segments
	PRR    |=  (1 << PRLCD);                     // Enable LCD power reduction bit

	TG_PlayToneSeq(TONEGEN_SEQ_SLEEP);

	MAIN_SETSTATUSLED(MAIN_STATLED_OFF);         // Save battery power - turn off status LED
	TIMEOUT_SLEEP_TIMER_OFF();

	while (ASSR & ((1 << TCN2UB) | (1 << TCR2UB) | (1 << OCR2UB))); // Wait for sleep timer to disengage

	SMCR    = ((1 << SM1) | (1 << SE));          // Power down sleep mode
	while (!(JoyStatus & JOY_UP))                // Joystick interrupt wakes the micro
	  SLEEP();
	   
	MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);       // Turn status LED back on
	TOUT_SetupSleepTimer();
		
	TG_PlayToneSeq(TONEGEN_SEQ_RESUME);

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
					DF_EnableDataflash(TRUE);

					if (!(SM_GetStoredDataSize(TYPE_FLASH)))
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
	
	SecsBeforeAutoSleep = 0;
	TIMEOUT_SLEEP_TIMER_OFF();
	
	LCD_puts_f(PSTR("*JTAG ON*"));
	
	MAIN_WaitForJoyRelease();
	
	WDTCR = ((1<<WDCE) | (1<<WDE));              // Enable Watchdog Timer to give reset after minimum timeout
	for (;;) {};                                 // Eternal loop - when watchdog resets the AVR it will enter the bootloader,
	                                             // assuming the BOOTRST fuse is programmed (otherwise app will just restart)
}
