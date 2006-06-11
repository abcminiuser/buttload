;
;             BUTTLOAD - Butterfly ISP Programmer
;				
;              Copyright (C) Dean Camera, 2006.
;                  dean_camera@hotmail.com
;

#include <avr/io.h>
#include "USI.h"

; This interrupt handler is my ASM version of the Atmel USI C handler,
; rewritten for speed. It clocks the USI shift register on each
; compare match interrupt of the Timer0 timer.

; To make the routine as fast as possible, the USICR register value
; (including the new USITC flag) is written directly to the register
; rather than a LDS/ORI/STS sequence. Neither the LDI nor the STS
; instructions use the SREG flags, so saving the SREG is not required.

.global TIMER0_COMP_vect             ; Timer 0 compare

TIMER0_COMP_vect:
  push r24
  ldi r24, (USI_CONTROL_REG_FLAGS | (1<<USITC))
  sts _SFR_MEM_ADDR(USICR), r24
  pop r24
  reti
