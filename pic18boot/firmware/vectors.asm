; Tabsize: 4
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  BootLoader.                                                             ;;
;;  Copyright (C) 2007 Diolan ( http://www.diolan.com )                     ;;
;;                                                                          ;;
;;  This program is free software: you can redistribute it and/or modify    ;;
;;  it under the terms of the GNU General Public License as published by    ;;
;;  the Free Software Foundation, either version 3 of the License, or       ;;
;;  (at your option) any later version.                                     ;;
;;                                                                          ;;
;;  This program is distributed in the hope that it will be useful,         ;;
;;  but WITHOUT ANY WARRANTY; without even the implied warranty of          ;;
;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           ;;
;;  GNU General Public License for more details.                            ;;
;;                                                                          ;;
;;  You should have received a copy of the GNU General Public License       ;;
;;  along with this program.  If not, see <http://www.gnu.org/licenses/>    ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Device Reset Vectors
;-----------------------------------------------------------------------------
	#include "p18fxxxx.inc"

	#include "boot.inc"
	#include "io_cfg.inc"
	#include "usb_desc.inc"
;-----------------------------------------------------------------------------
; Externals
;-----------------------------------------------------------------------------
	extern	main  
	extern	bootloader_soft_reset
;;	extern	bootloader_loop
	extern	bios_task
;;	extern	bios_getc
;;	extern	bios_putc
;;	extern	bios_setmode
;;	extern	bios_gethid
;;	extern	bios_puthid
;-----------------------------------------------------------------------------
; START
;-----------------------------------------------------------------------------
VECTORS		CODE	VECT	;;0x0000
	;--- RESET Vector
	org		VECT+0x0000
	clrf	TBLPTRH
	clrf	TBLPTRU
	movlb	GPR0
	bra		main
;;	bra		bios_puthid
;-----------------------------------------------------------------------------
;--- HIGH Interrupt Vector
	org		VECT+0x0008
	goto	APP_HIGH_INTERRUPT_VECTOR
;-----------------------------------------------------------------------------
;	0x000c Å` 0x0018:	6 Entries.
	bra		bootloader_soft_reset
	bra		bios_task
;;	bra		bootloader_loop
;;	bra		bios_gethid
;;	bra		bios_getc
;;	bra		bios_putc
;;	bra		bios_setmode

;--------------------------------------------------------------------------
;	ÉAÉvÉäãNìÆ
;--------------------------------------------------------------------------
	global	appl_reset_vector
appl_reset_vector
	movlb	0
;;	goto	APP_RESET_VECTOR	; Run Application FW
	bra		APP_RESET_VECTOR	; Run Application FW



	global	USB_LANG_DESC
;-----------------------------------------------------------------------------
; String descriptors language ID Descriptor
USB_LANG_DESC			dw	0x0403
;USB_LANG_DESC_bLength	db	(USB_LANG_DESC_end - USB_LANG_DESC)	; Size
;USB_LANG_DESC_bDscType	db	DSC_STR	; Descriptor type = string
USB_LANG_DESC_string	dw	0x0409	; Language ID = EN_US
USB_LANG_DESC_end

;-----------------------------------------------------------------------------
; Here is 4 bytes (2 program words) free
; in address range from 0x012 to 0x015 inclusive.
; Can be used for something short for size optimization.
;-----------------------------------------------------------------------------
;--- BOOTLOADER External Entry Point                         
;	org	VECT+0x0016
; if USE_EEPROM_MARK 
;	bra	bootloader_soft_reset
; endif        
;        
        ;--- HIGH Interrupt Vector
	org		VECT+0x0018
	goto	APP_LOW_INTERRUPT_VECTOR
;-----------------------------------------------------------------------------
; APPLICATION STUB
;-----------------------------------------------------------------------------
APPSTRT CODE APP_RESET_VECTOR
	goto    bootloader_soft_reset
;;	bra	$
;-----------------------------------------------------------------------------
	END
