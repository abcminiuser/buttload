#ifndef USI_H
#define USI_H

/* Includes */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "Main.h"
#include "AVRISPCommandBytes.h"

/* USI port and pin definitions. */
#define USI_OUT_REG	    PORTE	//!< USI port output register.
#define USI_IN_REG	    PINE	//!< USI port input register.
#define USI_DIR_REG	    DDRE	//!< USI port direction register.
#define USI_CLOCK_PIN	PE4		//!< USI clock I/O pin.
#define USI_DATAIN_PIN	PE5		//!< USI data input pin.
#define USI_DATAOUT_PIN	PE6		//!< USI data output pin.

/* SPI Mode Defines */
#define SPI_SAMPLE_LEADING 0	// Sample on leading _rising_ edge, setup on trailing _falling_ edge.
#define SPI_SAMPLE_FALLING 1	// Sample on leading _falling_ edge, setup on trailing _rising_ edge.

/*  Prescaler value converted to bit settings. */
#define TC0_PS_1    (1<<CS00)
#define TC0_PS_8    (1<<CS01)
#define TC0_PS_64   (1<<CS01)|(1<<CS00)
#define TC0_PS_256  (1<<CS02)
#define TC0_PS_1024 (1<<CS02)|(1<<CS00)

/* Other Defines */
#define USI_PRESET_SPEEDS 4

/* Macros */
#define USI_STOPUSITIMER() TCCR0A = 0;

/* Prototypes */
void    USI_SPIInitMaster( char Freq );
void    USI_SPIOff( void );
uint8_t USI_SPITransmit( unsigned char val );
uint8_t USI_SPITransmitWord( unsigned int val );
void    USI_SPIToggleClock(void);
void    USI_SPISetSpeed(uint8_t Freq);

#endif
