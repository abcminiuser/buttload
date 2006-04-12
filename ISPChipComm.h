/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef ISPCHIPCOMM_H
#define ISPCHIPCOMM_H

// INCLUDES:
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "Main.h"
#include "LCD_Driver.h"
#include "SPI.h"
#include "USI.h"
#include "AVRISPCommandBytes.h"

// MACROS AND DEFINES:
#define ISPCC_PROG_MODE_PAGE              0x01
#define ISPCC_PROG_MODE_PAGEDONE          0x80
#define ISPCC_POLL_MODE_AVR               0x03

#define ISPCC_POLL_BUSYFLAG               0x01

#define ISPCC_PAGE_POLLTYPE_MASK          0x70
#define ISPCC_PAGE_POLLTYPE_MASKSHIFT     4
#define ISPCC_PAGE_POLLTYPE_WAIT          (1 << 4)

#define ISPCC_WORD_POLLTYPE_MASK          0x0E
#define ISPCC_WORD_POLLTYPE_MASKSHIFT     1
#define ISPCC_WORD_POLLTYPE_WAIT          (1 << 1)

#define ISPCC_POLLTYPE_MASK               0x07
#define ISPCC_POLLTYPE_WAIT               (1 << 0)
#define ISPCC_POLLTYPE_DATA               (1 << 1)
#define ISPCC_POLLTYPE_READY              (1 << 2)

#define ISPCC_HIGH_BYTE_READ              (1 << 3)
#define ISPCC_LOW_BYTE_READ               (0 << 3)
#define ISPCC_HIGH_BYTE_WRITE             (1 << 3)
#define ISPCC_LOW_BYTE_WRITE              (0 << 3)

#define ISPCC_NO_FAULT                    0
#define ISPCC_FAULT_NOERASE               2
#define ISPCC_FAULT_NODATATYPE            3

#define ISPCC_USIMASK                     ((1<<USI_DATAIN_PIN) | (1<<USI_DATAOUT_PIN) | (1<<USI_CLOCK_PIN))

// EXTERNAL VARIABLES:
extern const uint8_t SyncErrorMessage[] PROGMEM;

// PROTOTYPES:
void   ISPCC_EnterChipProgrammingMode(void);
void   ISPCC_ProgramChip(void);
void   ISPCC_PollForProgComplete(uint8_t PollData, uint16_t PollAddr);

#endif
