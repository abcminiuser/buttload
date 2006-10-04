/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef GLOBALMACROS_H
#define GLOBALMACROS_H

	// Version Macros:
	#define VERSION_MAJOR            2
	#define VERSION_MINOR            1
	#define VERSION_VSTRING          {'V','0' + VERSION_MAJOR,'-','0' + VERSION_MINOR, '\0'}
	
	#define MAGIC_NUM                0b01101010 // Magic number, used for first-run detection or upgrade incompatibility checks
	
	// Program Type Macros:	
	#define TYPE_EEPROM              0
	#define TYPE_FLASH               1
	#define TYPE_FUSE                2
	#define TYPE_LOCK                3
	
	// Code Macros:
	#define TRUE                     1
	#define FALSE                    0
	
	#define ARRAY_UPPERBOUND(array)  ((sizeof(array) / sizeof(array[0])) - 1)
	
	#define MACROS                   do
	#define MACROE                   while (0)

	// Joystick Macros:
	#define JOY_LEFT                 (1 << 2)
	#define JOY_RIGHT                (1 << 3)
	#define JOY_UP                   (1 << 6)
	#define JOY_DOWN                 (1 << 7)
	#define JOY_PRESS                (1 << 4)
	#define JOY_INVALID              (1 << 0)
	
	#define JOY_BMASK                ((1 << 4) | (1 << 6) | (1 << 7))
	#define JOY_EMASK                ((1 << 2) | (1 << 3))
		
	// ASM Macros:
	#define SLEEP()                  MACROS{ asm volatile ("sleep" ::); }MACROE

#endif
