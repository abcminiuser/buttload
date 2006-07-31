;
;             BUTTLOAD - Butterfly ISP Programmer
;				
;              Copyright (C) Dean Camera, 2006.
;                  dean_camera@hotmail.com
;

; Routine sends a byte in the SPI format via the USI subsystem. Routine sets and returns the
; USI data according to the AVR-GCC ABI. The clock speed is configurable via a value preloaded
; into the GPIOR0 register. Interrupts are disabled during the transfer.

#include "USI.h"

.global  USI_SPITransmit
.func    USI_SPITransmit

USI_SPITransmit:
	sts  _SFR_MEM_ADDR(USIDR), r24                      ; Input uint8_t stored in R24, place in USI data register
	
	in   r23, _SFR_IO_ADDR(USIDelay)                    ; Grab the delay value from the GPIOR0 register
	ldi	 r24, USICONTROLREGS                            ; Preload value to be set into a temp reg for speed
	ldi	 r25, (USICONTROLREGS | (1 << USICLK))          ; Preload value to be set into a temp reg for speed
	ldi  r26, 8                                         ; Transferred bits counter register
	
; DELAY CHECKS - convert delay index into the actual cycle delay
CheckDelay0:
	cpi  r23, 0
	breq GoFastLoop

CheckDelay1:
	cpi  r23, 1	
	brne CheckDelay2
	ldi  r23, (32 - 9)
	rjmp GoSlowLoop
	
CheckDelay2:
	cpi  r23, 2
	brne CheckDelay3
	ldi  r23, (128 - 9)
	rjmp GoSlowLoop

CheckDelay3:
	ldi  r23, (256 - 9)
; END DELAY CHECKS	
	
; SLOW LOOP - (9 + r24) cycles per iteration
GoSlowLoop:
	cli
	
SlowLoopStart:
	sts	 _SFR_MEM_ADDR(USICR), r24
	sts	 _SFR_MEM_ADDR(USICR), r25
	
	mov  r27, r23

TimeLoop:
	dec  r27
	brne TimeLoop

LoopEnd:
	dec  r26
	brne SlowLoopStart
	rjmp Epilogue
; END SLOW LOOP

; FAST LOOP - 8 cycles per iteration
GoFastLoop:
	cli
	
FastLoopStart: 
	sts	 _SFR_MEM_ADDR(USICR), r24
	sts	 _SFR_MEM_ADDR(USICR), r25
	nop
	dec  r26
	brne FastLoopStart	
; END FAST LOOP	

Epilogue:
	sei
	lds  r24, _SFR_MEM_ADDR(USIDR)                      ; Output uint8_t stored in R24, get from USI data register
	ret
