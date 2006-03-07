;
;             BUTTLOAD - Butterfly ISP Programmer
;				
;              Copyright (C) Dean Camera, 2005.
;                  dean_camera@hotmail.com
;

#include <avr/io.h>

; Bonded ISRs. Here PCINT0_vect and PCINT1_vect ISRs are both linked to the same routine
; via a simple ASM jump instruction. In the C code, when the PCINT1_vect is reached, the
; PCINT0_vect vector will also execute that code when fired after the jump.

.global PCINT0_vect             ; Pin change 0 (joystick)
.global PCINT1_vect             ; Pin change 1 (joystick)

PCINT0_vect:
  jmp PCINT1_vect
