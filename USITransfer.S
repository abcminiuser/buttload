;
;             BUTTLOAD - Butterfly ISP Programmer
;				
;              Copyright (C) Dean Camera, 2006.
;                  dean_camera@hotmail.com
;

; Routine sends a byte in the SPI format via the USI subsystem. Routine sets and returns the
; USI data according to the AVR-GCC ABI. The clock speed is configurable via a value preloaded
; into the GPIOR0 register. Interrupts are disabled during the transfer.
;
; No push/pops required; only the "Call-Saved" registers are used, which improves speed. For
; information on how GCC handles register management, see the "FAQ" section of the AVRLIBC manual.

#include "USI.h"

.global  USI_SPITransmit                                ; \ Link the USI_SPITransmit label to the function
.func    USI_SPITransmit                                ; / prototype declared in USI.h

USI_SPITransmit:
	sts  _SFR_MEM_ADDR(USIDR), r24                      ; Input uint8_t stored in R24, place in USI data register
	
	ldi	 r24, (USICONTROLREGS)                          ; Preload values to be set into a temp reg for speed
	ldi	 r25, (USICONTROLREGS | (1 << USICLK))          ; Preload values to be set into a temp reg for speed
	
	in   r27, _SFR_IO_ADDR(USIDelay)                    ; Grab the delay value from the GPIOR0 register

	cli                                                 ; Kill interrupts while transferring

; DELAY CHECKS - Convert delay index into the actual cycle delay
	cpi  r27, 0                                         ; Test for 1843200 Hz frequency
	breq GoSuperFastLoop

	cpi  r27, 1                                         ; Test for 921600 Hz frequency
	breq GoFastLoop

	ldi  r23, (((32 - 4) / 4) / 3)
	cpi  r27, 2                                         ; Test for 230400 Hz frequency
	breq GoSlowLoop
	
	ldi  r23, (((64 - 4) / 4) / 3)
	cpi  r27, 3                                         ; Test for 115200 Hz frequency
	breq GoSlowLoop

	ldi  r23, (((128 - 4) / 4) / 3)
	cpi  r27, 4                                         ; Test for 57600 Hz frequency
	breq GoSlowLoop

	ldi  r23, (((256 - 4) / 4) / 3)                     ; Assume 28800 Hz frequency
; END DELAY CHECKS	
	
	
; SLOW LOOP
GoSlowLoop:
	ldi  r26, 8                                         ; Transferred bits counter register
	
SlowLoopStart:
	sts	 _SFR_MEM_ADDR(USICR), r24
	mov  r27, r23

	nop                                                 ; Maintain 50% duty cycle - 3 NOPs which
	nop                                                 ; use the same amount of clock cycles as
	nop                                                 ; the DEC and BRNE at the end of the loop

TimeLoop:
	dec  r27
	brne TimeLoop

	sts	 _SFR_MEM_ADDR(USICR), r25
	mov  r27, r23

TimeLoop2:
	dec  r27
	brne TimeLoop2

	dec  r26
	brne SlowLoopStart
	
	rjmp Epilogue
; END SLOW LOOP

; FAST LOOP
GoFastLoop:
	ldi  r27, (_SFR_MEM_ADDR(USICR) >> 8)     ; XH = R27
	ldi  r26, (_SFR_MEM_ADDR(USICR) & 0x00FF) ; XL = R26

	ldi  r23, 8

FastLoopStart:
	st   X, r24
	nop
	nop
	nop
	st   X, r25
	dec  r23
	brne FastLoopStart

	rjmp Epilogue
; END FAST LOOP

; SUPER FAST LOOP
GoSuperFastLoop:
	sts  _SFR_MEM_ADDR(USICR), r24
	sts  _SFR_MEM_ADDR(USICR), r25
	sts  _SFR_MEM_ADDR(USICR), r24
	sts  _SFR_MEM_ADDR(USICR), r25
	sts  _SFR_MEM_ADDR(USICR), r24
	sts  _SFR_MEM_ADDR(USICR), r25
	sts  _SFR_MEM_ADDR(USICR), r24
	sts  _SFR_MEM_ADDR(USICR), r25
	sts  _SFR_MEM_ADDR(USICR), r24
	sts  _SFR_MEM_ADDR(USICR), r25
	sts  _SFR_MEM_ADDR(USICR), r24
	sts  _SFR_MEM_ADDR(USICR), r25
	sts  _SFR_MEM_ADDR(USICR), r24
	sts  _SFR_MEM_ADDR(USICR), r25
	sts  _SFR_MEM_ADDR(USICR), r24
	sts  _SFR_MEM_ADDR(USICR), r25
; END SUPER FAST LOOP

Epilogue:
	sei
	lds  r24, _SFR_MEM_ADDR(USIDR)                      ; Output uint8_t stored in R24, get from USI data register
	ret
