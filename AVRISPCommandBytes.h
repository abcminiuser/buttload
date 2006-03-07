/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

// DEFINES:
#define MESSAGE_START             0x1B
#define TOKEN                     0x0E

#define CMD_SIGN_ON               0x01
#define CMD_SET_PARAMETER         0x02
#define CMD_GET_PARAMETER         0x03
#define CMD_LOAD_ADDRESS          0x06
#define CMD_FIRMWARE_UPGRADE      0x07
#define CMD_ENTER_PROGMODE_ISP    0x10
#define CMD_LEAVE_PROGMODE_ISP    0x11
#define CMD_CHIP_ERASE_ISP        0x12
#define CMD_PROGRAM_FLASH_ISP     0x13
#define CMD_READ_FLASH_ISP        0x14
#define CMD_PROGRAM_EEPROM_ISP    0x15
#define CMD_READ_EEPROM_ISP       0x16
#define CMD_PROGRAM_FUSE_ISP      0x17
#define CMD_READ_FUSE_ISP         0x18
#define CMD_PROGRAM_LOCK_ISP      0x19
#define CMD_READ_LOCK_ISP         0x1A
#define CMD_READ_SIGNATURE_ISP    0x1B
#define CMD_READ_OSCCAL_ISP       0x1C
#define CMD_SPI_MULTI             0x1D

#define PARAM_BUILD_NUMBER_LOW    0x80
#define PARAM_BUILD_NUMBER_HIGH   0x81
#define PARAM_HARDWARE_VERSION    0x90
#define PARAM_SW_MAJOR            0x91
#define PARAM_SW_MINOR            0x92
#define PARAM_OSC_PSCALE          0x96
#define PARAM_OSC_CMATCH          0x97
#define PARAM_SCK_DURATION        0x98
#define PARAM_RESET_POLARITY      0x9E
#define PARAM_CONTROLLER_INIT     0x9F

#define STATUS_CMD_OK             0x00
#define STATUS_CMD_FAILED         0xC0
#define STATUS_CKSUM_ERROR        0xC1
#define STATUS_CMD_UNKNOWN        0xC9

#define SPI_SPEED_921600Hz        0
#define SPI_SPEED_230400Hz        1
#define SPI_SPEED_57600Hz         2
#define SPI_SPEED_28800Hz         3
