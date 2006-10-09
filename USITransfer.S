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

; REGISTER ALLOCATIONS (all call-saved):
#define  USI_SCRATCH_ASM_REG   r20
#define  USI_CONTROL_ASM_REG   r21
#define  USI_CONTROLCK_ASM_REG r22
#define  USI_BITSREM_ASM_REG   r23
#define  USI_PARAM_ASM_REG     r24
#define  USI_DELAYCNT_ASM_REG  r25
#define  USI_XL_ASM_REG        r26
#define  USI_XH_ASM_REG        r27
#define  USI_USICR_PTR_ASM_REG X

USI_SPITransmit:
	sts  _SFR_MEM_ADDR(USIDR), USI_PARAM_ASM_REG                  ; Input uint8_t stored in R24, place in USI data register
	
	ldi	 USI_CONTROL_ASM_REG,   (USICONTROLREGS)                  ; Preload values to be set into a temp reg for speed
	ldi	 USI_CONTROLCK_ASM_REG, (USICONTROLREGS | (1 << USICLK))  ; Preload values to be set into a temp reg for speed	

	ldi  USI_XH_ASM_REG, (_SFR_MEM_ADDR(USICR) >> 8)              ; XH = R27
	ldi  USI_XL_ASM_REG, (_SFR_MEM_ADDR(USICR) & 0x00FF)          ; XL = R26

	ldi  USI_BITSREM_ASM_REG, 8

	in   USI_SCRATCH_ASM_REG, _SFR_IO_ADDR(USIDelay)              ; Grab the delay value from the GPIOR0 register

	cli                                                           ; Kill interrupts while transferring

; DELAY CHECKS - Convert delay index into the actual cycle delay
	cpi  USI_SCRATCH_ASM_REG, 0                                   ; Test for 1843200 Hz frequency
	breq SuperFastTransfer

	cpi  USI_SCRATCH_ASM_REG, 1                                   ; Test for 921600 Hz frequency
	breq FastTransfer

	ldi  USI_DELAYCNT_ASM_REG, (((32 - 4) / 4) / 3)
	cpi  USI_SCRATCH_ASM_REG, 2                                   ; Test for 230400 Hz frequency
	breq SlowTransfer
	
	ldi  USI_DELAYCNT_ASM_REG, (((64 - 4) / 4) / 3)
	cpi  USI_SCRATCH_ASM_REG, 3                                   ; Test for 115200 Hz frequency
	breq SlowTransfer

	ldi  USI_DELAYCNT_ASM_REG, (((128 - 4) / 4) / 3)
	cpi  USI_SCRATCH_ASM_REG, 4                                   ; Test for 57600 Hz frequency
	breq SlowTransfer

	ldi  USI_DELAYCNT_ASM_REG, (((256 - 4) / 4) / 3)              ; Assume 28800 Hz frequency
; END DELAY CHECKS	
	
	
; SLOW LOOP
SlowTransfer:
	st	 USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	mov  USI_SCRATCH_ASM_REG, USI_DELAYCNT_ASM_REG

	nop                                                           ; Maintain 50% duty cycle - 3 NOPs which
	nop                                                           ; use the same amount of clock cycles as
	nop                                                           ; the DEC and BRNE at the end of the loop

TimeLoop:
	dec  USI_SCRATCH_ASM_REG
	brne TimeLoop

	st	 USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	mov  USI_SCRATCH_ASM_REG, USI_DELAYCNT_ASM_REG

TimeLoop2:
	dec  USI_SCRATCH_ASM_REG
	brne TimeLoop2

	dec  USI_BITSREM_ASM_REG
	brne SlowTransfer
	
	rjmp Epilogue
; END SLOW LOOP

; FAST LOOP
FastTransfer:
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	nop                                                           ; NOP to maintain 50% duty cycle
	dec  USI_BITSREM_ASM_REG                                      ; DEC can be executed here since ST doesn't modify the Z flag
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	brne FastTransfer

	rjmp Epilogue
; END FAST LOOP

; SUPER FAST LOOP
SuperFastTransfer:
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st   USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
; END SUPER FAST LOOP

Epilogue:
	sei
	lds  USI_PARAM_ASM_REG, _SFR_MEM_ADDR(USIDR)                  ; Output uint8_t stored in R24, get from USI data register
	ret
