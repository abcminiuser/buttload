;
;             BUTTLOAD - Butterfly ISP Programmer
;
;              Copyright (C) Dean Camera, 2007.
;
;			  dean_camera@fourwalledcubicle.com
;                  www.fourwalledcubicle.com
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

; DEFINES:
#define  MEM_ADDR(sfr)         _SFR_MEM_ADDR(sfr)
#define  IO_ADDR(sfr)          _SFR_IO_ADDR(sfr)
#define  HIGH(val)             (val >> 8)
#define  LOW(val)              (val & 0xFF)

#define  XL                    r26
#define  XH                    r27

; REGISTER ALLOCATIONS (all call-saved):
#define  USI_SCRATCH_ASM_REG   r20
#define  USI_CONTROL_ASM_REG   r21
#define  USI_CONTROLCK_ASM_REG r22
#define  USI_BITSREM_ASM_REG   r23
#define  USI_PARAM_ASM_REG     r24
#define  USI_DELAYCNT_ASM_REG  r25
#define  USI_XL_ASM_REG        XL
#define  USI_XH_ASM_REG        XH
#define  USI_USICR_PTR_ASM_REG X

; NAME:      | USI_SPITransmit
; PURPOSE:   | Transmits and recieves a single byte via the USI subsystem (in SPI mode)
; ARGUMENTS: | None
; RETURNS:   | None
USI_SPITransmit:
	sts   MEM_ADDR(USIDR), USI_PARAM_ASM_REG                       ; Input uint8_t stored in R24, place in USI data register
	
	ldi	  USI_CONTROL_ASM_REG,   (USICONTROLREGS)                  ; Preload values to be set into a temp reg for speed
	ldi	  USI_CONTROLCK_ASM_REG, (USICONTROLREGS | (1 << USICLK))  ; Preload values to be set into a temp reg for speed	

	ldi   USI_XH_ASM_REG, HIGH(MEM_ADDR(USICR))                    ; XH = R27
	ldi   USI_XL_ASM_REG, LOW(MEM_ADDR(USICR))                     ; XL = R26

	ldi   USI_BITSREM_ASM_REG, 8

	in    USI_SCRATCH_ASM_REG, IO_ADDR(USIDelay)                   ; Grab the delay value from the GPIOR0 register

	cli                                                            ; Kill interrupts while transferring

; DELAY CHECKS - Convert delay index into the actual cycle delay
	cpi   USI_SCRATCH_ASM_REG, 0                                   ; Test for 1843200 Hz frequency
	breq  SuperFastTransfer

	cpi   USI_SCRATCH_ASM_REG, 1                                   ; Test for 921600 Hz frequency
	breq  FastTransfer

	ldi   USI_DELAYCNT_ASM_REG, ((32 - 6) / 3)
	cpi   USI_SCRATCH_ASM_REG, 2                                   ; Test for 230400 Hz frequency
	breq  SlowTransfer
	
	ldi   USI_DELAYCNT_ASM_REG, ((64 - 6) / 3)
	cpi   USI_SCRATCH_ASM_REG, 3                                   ; Test for 115200 Hz frequency
	breq  SlowTransfer

	ldi   USI_DELAYCNT_ASM_REG, ((128 - 6) / 3)
	cpi   USI_SCRATCH_ASM_REG, 4                                   ; Test for 57600 Hz frequency
	breq  SlowTransfer

	ldi   USI_DELAYCNT_ASM_REG, ((256 - 6) / 3)
	cpi   USI_SCRATCH_ASM_REG, 5                                   ; Test for 28800 Hz frequency
	breq  SlowTransfer

	rjmp  ClockingTransfer                                         ; Otherwise continue to the clocking loop
; END DELAY CHECKS
	
; SLOW LOOP
SlowTransfer:
	; Cycle-counted to ensure 50% duty cycle per clock; equates to (6 + (3 * USI_DELAYCNT_ASM_REG)) cycles between toggles

	st	  USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	mov   USI_SCRATCH_ASM_REG, USI_DELAYCNT_ASM_REG

	nop                                                            ; Maintain 50% duty cycle - 3 NOPs which
	nop                                                            ; use the same amount of clock cycles as
	nop                                                            ; the DEC and BRNE at the end of the loop

  TimeLoop:
	dec   USI_SCRATCH_ASM_REG
	brne  TimeLoop

	st	  USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	mov   USI_SCRATCH_ASM_REG, USI_DELAYCNT_ASM_REG

  TimeLoop2:
	dec   USI_SCRATCH_ASM_REG
	brne  TimeLoop2

	dec   USI_BITSREM_ASM_REG
	brne  SlowTransfer
	
	rjmp  Epilogue
; END SLOW LOOP

; FAST LOOP
FastTransfer:
	; Cycle-counted to ensure 50% duty cycle per clock; equates to 4 cycles between toggles

	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	nop                                                            ; NOP to maintain 50% duty cycle
	dec   USI_BITSREM_ASM_REG                                      ; DEC can be executed here since ST doesn't modify the Z flag
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	brne  FastTransfer

	rjmp  Epilogue
; END FAST LOOP

; SUPER FAST LOOP
SuperFastTransfer:
	; Cycle-counted to ensure 50% duty cycle per clock; equates to 2 cycles between toggles

	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG
		
	rjmp  Epilogue
; END SUPER FAST LOOP

; CLOCKING LOOP
ClockingTransfer:
	; Cycle-counted to ensure 50% duty cycle per clock; equates to 9 cycles between toggles

	ldi   USI_DELAYCNT_ASM_REG, 7

  ClockingLoop:
	sbi   IO_ADDR(USI_CLOCK_OUT_PORT), USI_CLOCK_BIT
	dec   USI_DELAYCNT_ASM_REG

	ldi   USI_SCRATCH_ASM_REG, 1
  ClockingDelayLoop1:
	dec   USI_SCRATCH_ASM_REG
	brne  ClockingDelayLoop1

	brne  ClockingLoop
	nop
	
	sbi   IO_ADDR(USI_CLOCK_OUT_PORT), USI_CLOCK_BIT
	st    USI_USICR_PTR_ASM_REG, USI_CONTROL_ASM_REG

	ldi   USI_SCRATCH_ASM_REG, 1
  ClockingDelayLoop2:
	dec   USI_SCRATCH_ASM_REG
	brne  ClockingDelayLoop2
	ldi   USI_DELAYCNT_ASM_REG, 7

  ClockingLoop2:
	sbi   IO_ADDR(USI_CLOCK_OUT_PORT), USI_CLOCK_BIT
	dec   USI_DELAYCNT_ASM_REG

	ldi   USI_SCRATCH_ASM_REG, 1
  ClockingDelayLoop3:
	dec   USI_SCRATCH_ASM_REG
	brne  ClockingDelayLoop3

	brne  ClockingLoop2
	nop

	sbi   IO_ADDR(USI_CLOCK_OUT_PORT), USI_CLOCK_BIT
	st    USI_USICR_PTR_ASM_REG, USI_CONTROLCK_ASM_REG

	dec   USI_BITSREM_ASM_REG
	brne  ClockingTransfer
	
	rjmp  Epilogue
; END CLOCKING LOOP

Epilogue:
	sei
	lds   USI_PARAM_ASM_REG, MEM_ADDR(USIDR)                       ; Output uint8_t stored in R24, get from USI data register
	ret
