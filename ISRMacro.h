/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

/* Must be included after avr/interrupt.h. This file re-defines the new
   ISR macro to extend it to allow custom attributes. When the old ISR
   macros SIGNAL and INTERRUPT were depricated, no suitable replacement
   was specifed for interruptable ISR routine (and no macro at all exists
   for naked ISRs). This file avoids the clumsyness of declaring the ISR
   routines manually with custom attributes and thus gives code uniformity.

   As a bonus, the default vector (called when an interrupt fires which does
   not have an associated ISR routine) is aliased here to a more descriptive
   name - use the new name as you would a standard signal name.
   
   The avaliable attributes are:
		1) ISR_BLOCK   - ISR, interrupts disable until ISR completes.
		1) ISR_NOBLOCK - ISR, interrupts enabled until ISR completes.
		2) ISR_NAKED   - ISR, no prologue or epilogue.                     */

#ifndef ISRMACRO_H
#define ISRMACRO_H

	// If present, kill the current ISR macro:
	#if defined(ISR)
	   #undef ISR
	#endif
	
	// The default vector is given a more descriptive alias here:
	#define BADISR_vect __vector_default
	
	// The three attributes are defined here:
	#if defined(__GNUC__) && (__GNUC__ > 3)
	   #define ISR_NOBLOCK __attribute__((interrupt, used, externally_visible))
	   #define ISR_BLOCK   __attribute__((signal, used, externally_visible))
	   #define ISR_NAKED   __attribute__((signal, naked, used, externally_visible))
	#else
	   #define ISR_NOBLOCK __attribute__((interrupt))
	   #define ISR_BLOCK   __attribute__((signal))
	   #define ISR_NAKED   __attribute__((signal, naked))
	#endif
	
	// Define the new ISR macro:
	#define ISR(vector, attributes)  \
	 void vector (void) attributes; \
	 void vector (void)
	 
#endif
