// This file has been prepared for Doxygen automatic documentation generation.
/*! \file ********************************************************************
*
* Atmel Corporation
*
* \li File:               spi_via_usi_driver.c
* \li Compiler:           IAR EWAAVR 3.10c
* \li Support mail:       avr@atmel.com
*
* \li Supported devices:  All devices with Universal Serial Interface (USI)
*                         capabilities can be used.
*                         The example is written for ATmega169.
*
* \li AppNote:            AVR319 - Using the USI module for SPI communication.
*
* \li Description:        Example on how to use the USI module for communicating
*                         with SPI compatible devices. The functions and variables
*                         prefixed 'USI_' can be renamed to be able to use several
*                         spi drivers (using different interfaces) with similar names.
*                         Some basic SPI knowledge is assumed.
*
*                         $Revision: 1.4 $
*                         $Date: Monday, September 13, 2004 12:08:54 UTC $
****************************************************************************/
#include "USI.h"

/*! \brief  Data input register buffer.
 *
 *  Incoming bytes are stored in this byte until the next transfer is complete.
 *  This byte can be used the same way as the SPI data register in the native
 *  SPI module, which means that the byte must be read before the next transfer
 *  completes and overwrites the current value.
 */
static uint8_t storedUSIDR;

/*! \brief  Driver status bit structure.
 *
 *  This struct contains status flags for the driver.
 *  The flags have the same meaning as the corresponding status flags
 *  for the native SPI module. The flags should not be changed by the user.
 *  The driver takes care of updating the flags when required.
 */
struct usidriverStatus_t {
	unsigned char transferComplete : 1; //!< True when transfer completed.
	unsigned char writeCollision : 1;   //!< True if put attempted during transfer.
};

volatile static struct usidriverStatus_t USI_status; //!< The driver status bits.


                                                          // SPI Clock Duration Value, Timer Prescale, Compare Value
const uint8_t USIPSValues[USI_PRESET_SPEEDS][3] PROGMEM = {{SPI_SPEED_921600Hz      , TC0_PS_1      , 64           },   // Actual speed = 113,427Hz
                                                            {SPI_SPEED_230400Hz      , TC0_PS_1      , 84           },   // Actual speed =  86,738Hz
                                                            {SPI_SPEED_57600Hz       , TC0_PS_1      , 128           },  // Actual speed =  57,600Hz
                                                            {SPI_SPEED_28800Hz       , TC0_PS_1      , 255           }}; // Actual speed =  28,912Hz
                                                          // USI Speed = (Fcpu/Prescale/(Compare + 1))Hz    Max Speed = (Fosc/64)

/*! \brief  Timer/Counter 0 Compare Match Interrupt handler.
 *
 *  This interrupt handler is only enabled when transferring data
 *  in master mode. It toggles the USI clock pin, i.e. two interrupts
 *  results in one clock period on the clock pin and for the USI counter.
 */
ISR(TIMER0_COMP_vect, ISR_BLOCK)
{
	USICR |= (1<<USITC);	// Toggle clock output pin.
}



/*! \brief  USI Timer Overflow Interrupt handler.
 *
 *  This handler disables the compare match interrupt if in master mode.
 *  When the USI counter overflows, a byte has been transferred, and we
 *  have to stop the timer tick.
 *  For all modes the USIDR contents are stored and flags are updated.
 */
ISR(USI_OVERFLOW_vect, ISR_BLOCK)
{
	// Master must now disable the compare match interrupt
	// to prevent more USI counter clocks.
	TIMSK0 &= ~(1<<OCIE0A);
	
	// Update flags and clear USI counter
	USISR = (1<<USIOIF);
	USI_status.transferComplete = 1;

	// Copy USIDR to buffer to prevent overwrite on next transfer.
	storedUSIDR = USIDR;
}



/*! \brief  Initialize USI as SPI master.
 *
 *  This function sets up all pin directions and module configurations.
 *  Use this function initially or when changing from slave to master mode.
 *  Note that the stored USIDR value is cleared.
 *
 *  \param spi_mode  Required SPI mode, must be 0 or 1.
 */
void USI_SPIInitMaster(char Freq)
{
	// Configure port directions.
 	USI_DIR_REG |= (1<<USI_DATAOUT_PIN) | (1<<USI_CLOCK_PIN);  // Outputs.
	USI_DIR_REG &= ~(1<<USI_DATAIN_PIN);                       // Inputs.
	USI_OUT_REG |= (1<<USI_DATAIN_PIN);                        // Pull-ups.
	USI_OUT_REG &= ~(1<<USI_DATAOUT_PIN) | (1<<USI_CLOCK_PIN); // Pull-ups.
	
	// Configure USI to 3-wire master mode with overflow interrupt.
	USICR = (1<<USIOIE) | (1<<USIWM0) |
	        (1<<USICS1) | (SPI_SAMPLE_LEADING<<USICS0) |
	        (1<<USICLK);

	// Set the compare and prescaler for the requested frequency:
	USI_SPISetSpeed(Freq);
	
	// Init driver status register.
	USI_status.transferComplete = 0;
	USI_status.writeCollision   = 0;
	
	storedUSIDR = 0;
}

void USI_SPIOff( void )
{
	USI_DIR_REG &= ~((1<<USI_DATAOUT_PIN) | (1<<USI_CLOCK_PIN));                        // Inputs.
	USI_OUT_REG &= ~((1<<USI_DATAIN_PIN)  | (1<<USI_DATAOUT_PIN) | (1<<USI_CLOCK_PIN)); // Pull-ups.
	DDRF  &= ~(1 << 6);
	PORTF &= ~(1 << 6);
	
	USI_STOPUSITIMER();
}

/*! \brief  Put one byte on bus.
 *
 *  Use this function like you would write to the SPDR register in the native SPI module.
 *  Calling this function in master mode starts a transfer, while in slave mode, a
 *  byte will be prepared for the next transfer initiated by the master device.
 *  If a transfer is in progress, this function will set the write collision flag
 *  and return without altering the data registers.
 *
 *  \returns  0 if a write collision occurred, 1 otherwise.
 */
uint8_t USI_SPITransmit( unsigned char val )
{
	// Check if transmission in progress,
	// i.e. USI counter unequal to zero.
	if( (USISR & 0x0F) != 0 ) {
		// Indicate write collision and return.
		USI_status.writeCollision = 1;
		return 0;
	}
	
	// Reinit flags.
	USI_status.transferComplete = 0;
	USI_status.writeCollision = 0;

	// Put data in USI data register.
	USIDR = val;
	
	TIFR0  |= (1<<OCF0A);  // Clear compare match flag.
	TIMSK0 |= (1<<OCIE0A); // Enable compare match interrupt.

	// Clear the timer 0 value
	TCNT0 = 0;

	do {} while( USI_status.transferComplete == 0 );

	return storedUSIDR;
}

uint8_t USI_SPITransmitWord( unsigned int val )
{
	USI_SPITransmit((uint8_t)(val >> 8));
	return USI_SPITransmit((uint8_t)val);
}

void USI_SPIToggleClock(void)
{
	MAIN_Delay1MS(1);
	USICR |= (1 << USITC); // Toggle the clock pin
	MAIN_Delay1MS(1);
	USICR |= (1 << USITC); // Toggle the clock pin
	MAIN_Delay1MS(1);	
}

void USI_SPISetSpeed(uint8_t Freq)
{
	if (Freq == 0xFF) // Blank EEPROM, default to fastest setting
	   Freq = SPI_SPEED_921600Hz;

	for (uint8_t MatchIndex = 0; MatchIndex < USI_PRESET_SPEEDS; MatchIndex++)
	{
		if ((pgm_read_byte(&USIPSValues[MatchIndex][0]) == Freq) || (MatchIndex == (USI_PRESET_SPEEDS - 1)))
		{
			// Init Output Compare Register.
			OCR0A = pgm_read_byte(&USIPSValues[MatchIndex][2]);

			// Enable 'Clear Timer on Compare match' and set prescaler, which starts the timer
			TCCR0A = ((1<<WGM01) | pgm_read_byte(&USIPSValues[MatchIndex][1]));
				
			return;
		}
	}
}

// end of file
