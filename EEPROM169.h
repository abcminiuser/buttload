#ifndef _eeprom169_h_
#define _eeprom169_h_

// INCLUDES:
#include <inttypes.h>

// PROTOTYPES:
uint8_t eeprom_read_byte_169(const uint16_t *addr);
void    eeprom_write_byte_169(const uint16_t *addr, uint8_t val);

#endif
