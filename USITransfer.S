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
; As you can see, assembly is not one of my strong points.

#include <avr/io.h>
#include "USI.h"

.global  USI_SPITransmit
.func    USI_SPITransmit

USI_SPITransmit:
	sts _SFR_MEM_ADDR(USIDR), r24                    ; Input uint8_t stored in R24, place in USI data register
	
	in  r24, _SFR_IO_ADDR(USIDelay)                  ; Grab the delay value from the GPIOR0 register
	ldi	r25, (USI_CONTROL_REG_FLAGS | (1 << USICLK)) ; Preload value to be set into a temp reg for speed
	ldi	r26, (USI_CONTROL_REG_FLAGS)                 ; Preload value to be set into a temp reg for speed
	ldi r27, 0x08                                    ; Transferred bits counter register
	
	cli
	
LoopStart:
	sts	_SFR_MEM_ADDR(USICR), r26
	sts	_SFR_MEM_ADDR(USICR), r25
	
	cpi	 r24, 0x00
	breq LoopEnd
	cpi	 r24, 0x01
	breq Delay32
	cpi	 r24, 0x02
	breq Delay96
	
Delay128:
	ldi r23, ((128-12) / 4)                         ; Delay counter register
D128L:
	nop
	dec r23
	brne D128L

Delay96:
	ldi r23, ((96-13) / 4)                          ; Delay counter register
D96L:
	nop
	dec r23
	brne D96L
	
Delay32:
	ldi r23, ((32-11) / 3)                          ; Delay counter register
D32L:
	dec r23
	brne D32L

LoopEnd:
	dec  r27
	brne LoopStart
	
	sei
	
	lds  r24, _SFR_MEM_ADDR(USIDR)                  ; Output uint8_t stored in R24, get from USI data register

	ret
