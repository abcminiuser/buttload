/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#ifndef TAGMGR_H
#define TAGMGR_H

	// INCLUDES:
	#include <avr/io.h>
	
	#include "GlobalMacros.h"
	#include "Main.h"
	#include "ButtLoadTag.h"
	#include "Dataflash.h"
	
	// PROTOTYPES:
	void   TM_ShowTags(void);
	void   TM_FindNextTag(void);
	
#endif
