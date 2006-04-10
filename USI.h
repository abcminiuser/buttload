#if (!defined(USI_H))
 /* Assember and C Code Defines. */
 #define USI_CONTROL_REG_FLAGS ((1<<USIOIE) | (1<<USIWM0) | (1<<USICS1) | (1<<USICLK))
#endif

#if (!defined(USI_H) && !(defined(__ASSEMBLER__)))
#define USI_H

/* Includes */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "Main.h"
#include "AVRISPCommandBytes.h"

/* USI port and pin definitions. */
#define USI_OUT_REG	           PORTE   //!< USI port output register.
#define USI_IN_REG	           PINE    //!< USI port input register.
#define USI_DIR_REG	           DDRE    //!< USI port direction register.
#define USI_CLOCK_PIN	       PE4     //!< USI clock I/O pin.
#define USI_DATAIN_PIN	       PE5     //!< USI data input pin.
#define USI_DATAOUT_PIN	       PE6     //!< USI data output pin.

/*  Prescaler value converted to bit settings. */
#define TC0_PS_1               (1<<CS00)
#define TC0_PS_8               (1<<CS01)
#define TC0_PS_64              (1<<CS01)|(1<<CS00)
#define TC0_PS_256             (1<<CS02)
#define TC0_PS_1024            (1<<CS02)|(1<<CS00)

/* Other Defines */
#define USI_PRESET_SPEEDS       4

#define USI_SPI_SPEED_921600Hz  0
#define USI_SPI_SPEED_230400Hz  1
#define USI_SPI_SPEED_57600Hz   2
#define USI_SPI_SPEED_28800Hz   3

/* Macros */
#define USI_STOPUSITIMER()      MACROS{ TCCR0A = 0; }MACROE

/* Prototypes */
void    USI_SPIInitMaster( char Freq );
void    USI_SPIOff( void );
uint8_t USI_SPITransmit( unsigned char val );
uint8_t USI_SPITransmitWord( unsigned int val );
void    USI_SPIToggleClock(void);
void    USI_SPISetSpeed(uint8_t Freq);

#endif
