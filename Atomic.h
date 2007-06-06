/*
             BUTTLOAD - Butterfly ISP Programmer

              Copyright (C) Dean Camera, 2007.
              
             dean [at] fourwalledcubicle [dot] com
                  www.fourwalledcubicle.com
*/

#ifndef ATOMIC_H
#define ATOMIC_H

	#include <avr/io.h>
	#include <avr/interrupt.h>

	static __inline__ uint8_t __iSeiRetVal(void)               { sei(); return 1; }   
	static __inline__ uint8_t __iCliRetVal(void)               { cli(); return 1; }   
	static __inline__ void    __iSeiParam(const uint8_t *__s)  { sei(); __asm__ volatile ("" ::: "memory"); (void)__s; }
	static __inline__ void    __iCliParam(const uint8_t *__s)  { cli(); __asm__ volatile ("" ::: "memory"); (void)__s; }
	static __inline__ void    __iRestore(const  uint8_t *__s)  { SREG = *__s; __asm__ volatile ("" ::: "memory"); }

	#define ATOMIC_BLOCK(exitmode)     for ( exitmode, __ToDo = __iCliRetVal(); __ToDo ; __ToDo = 0 )

	#define ATOMIC_RESTORESTATE        uint8_t sreg_save __attribute__((__cleanup__(__iRestore)))  = SREG
	#define ATOMIC_FORCEON             uint8_t sreg_save __attribute__((__cleanup__(__iSeiParam))) = 0
	
	#define NON_ATOMIC_BLOCK(type)     for ( type, __ToDo = __iSeiRetVal(); __ToDo ; __ToDo = 0 )
	
	#define NONATOMIC_RESTORESTATE     uint8_t sreg_save __attribute__((__cleanup__(__iRestore)))  = SREG
	#define NONATOMIC_FORCEOFF         uint8_t sreg_save __attribute__((__cleanup__(__iCliParam))) = 0

#endif
