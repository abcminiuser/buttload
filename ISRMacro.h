/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
             dean [at] fourwalledcubicle [dot] com
                  www.fourwalledcubicle.com
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
   
   The new macro is backwards compatible with the original ISR macro.
   
   The avaliable attributes are:
      1) ISR_BLOCK         - ISR, interrupts disable until ISR completes.
      2) ISR_NOBLOCK       - ISR, interrupts enabled until ISR completes.
      3) ISR_NAKED         - ISR, no prologue or epilogue.
      4) ISR_ALIASOF(vect) - ISR, alias to another interrupt vector's ISR. GCC 4.2+ only.

   For GCC 3.x vector aliases, you can use the ISR_ALIAS_COMPAT macro (instead
   of ISR macro). Works with GCC 3.x as well as GCC 4.x, but compat aliased vector
   ISR will contain a JMP instruction that the non-compat aliased vector does not have.
*/

#ifndef ISRMACRO_H
#define ISRMACRO_H

   // If present, kill the current ISR macro:
   #if defined(ISR)
      #undef ISR
   #endif
   
   // The default vector is given a more descriptive alias here:
   #define BADISR_vect __vector_default
   
   // Return from interrupt command, defined for convenience in ISR_NAKED routines:
   #define reti() asm volatile ("RETI"::)
   
   // Internal macros:
   #define __replace_and_string(name) #name
   
   // Definition of the attributes here, GCC version specific:
   #if defined(__GNUC__) && (__GNUC__ > 3)
      #define ISR_NOBLOCK    __attribute__((interrupt, used, externally_visible))
      #define ISR_BLOCK      __attribute__((signal, used, externally_visible))
      #define ISR_NAKED      __attribute__((signal, naked, used, externally_visible))
      #define ISR_ALIASOF(v) __attribute__((alias(__replace_and_string(v)))) // GCC 4.2 and greater only!
   #else
      #define ISR_NOBLOCK   __attribute__((interrupt))
      #define ISR_BLOCK     __attribute__((signal))
      #define ISR_NAKED     __attribute__((signal, naked))
   #endif

   // GCC 3.x compatible alias macro. Works with GCC 4.1 also:
   #define ISR_ALIAS_COMPAT(vector, aliasof)      \
      void vector (void) ISR_NAKED;               \
      void vector (void) { asm volatile ("jmp " __replace_and_string(aliasof) ::); }

   // New ISR macro definition:
   #define ISR(vector, ...)                       \
      void vector (void) ISR_BLOCK __VA_ARGS__;   \
      void vector (void)
#endif
