;; TAB SIZE = 4
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

	radix	dec

; Main definitions
;-----------------------------------------------------------------------------
; Bootloader Version
#define FW_VER_MAJOR		0
#define FW_VER_MINOR		2
#define FW_VER_SUB_MINOR	2
#define FW_VER_CODE		((FW_VER_MAJOR << 8) | (FW_VER_MINOR << 4) | (FW_VER_SUB_MINOR))
;-----------------------------------------------------------------------------
; BUILD Switches:
#define	TEST_MODE		1		; 1 のときは 0x800番地以降に配置される.
#define	DEBUG_CODE		0		; 1 のときは write_code_debug関数が使用可能.
#define	HAVE_ENDPOINT	2		; ENDPOINT 個数
#define	CLEAR_RAM		0		; 1 のときは 起動時にRAMをゼロクリアする.


#define	UNKNOWN_PACKET_DUMP	0

;-----------------------------------------------------------------------------
; Application FW RESET and INTERRUPT VECTORS
; VECT は Makefileで定義されます。

#define	APP_RESET_VECTOR			(VECT+0x800)
#define	APP_HIGH_INTERRUPT_VECTOR	(VECT+0x808)
#define	APP_LOW_INTERRUPT_VECTOR	(VECT+0x818)


;#if	TEST_MODE
;#define	VECT						0x000800
;#define	APP_RESET_VECTOR       		0x001000
;#define	APP_HIGH_INTERRUPT_VECTOR	0x001008
;#define	APP_LOW_INTERRUPT_VECTOR	0x001018
;#else
;#define	VECT						0x000000
;#define	APP_RESET_VECTOR       		0x000800
;#define	APP_HIGH_INTERRUPT_VECTOR	0x000808
;#define	APP_LOW_INTERRUPT_VECTOR	0x000818
;#endif

#ifdef __18F14K50
#define		GPR0	0x2		;// 汎用ワークのアドレスBSR( 0 番地からこちらに移動 )
#define		DPRAM	0x200	;// USB DualPort RAM
#else
#define		GPR0	0x4		;// 汎用ワークのアドレスBSR( 0 番地からこちらに移動 )
#define		DPRAM	0x400	;// USB DualPort RAM
#endif

#define		GPRF	0xf		;// PIC 18F14K50 限定: 0xf5X アドレス参照のため.

;-----------------------------------------------------------------------------
; CPU Power & Speed
; If CPU_5V_HS is 1 then CPU run from 48 MHz PLL clock. REQUIRE 5V Power Supply
; If CPU_5V_HS is 0 then CPU run from 16 MHz OSC directly. Possible 3.3V Power Supply
#define CPU_5V_HS		1
;-----------------------------------------------------------------------------
; EEPROM Mark to run bootloader
#define USE_EEPROM_MARK		0		; Enable EEPROM MARK. To disable = 0
#define EEPROM_MARK_ADDR	0x00	; EEPROM Address for MARK
#define EEPROM_MARK			0x5A	; MARK value
;-----------------------------------------------------------------------------
; EEPROM and ID encoding by XTEA support
#define ENCODE_EEPROM		0	; Disable EEPROM encoding. To enable = 1
#define ENCODE_ID			0	; Disable ID encoding. To enable = 1

;-----------------------------------------------------------------------------
; SRAM Mark to run bootloader
#define USE_SRAM_MARK		0		; Enable SRAM MARK. To disable = 0

;-----------------------------------------------------------------------------
;	8bit変数を WREG にロードしたい.
ld	macro	MEMORY_
	movf	MEMORY_ , W
	endm

;-----------------------------------------------------------------------------
;	WREG をメモリーにストアしたい.
st	macro	MEMORY_
	movwf	MEMORY_
	endm

;-----------------------------------------------------------------------------
;	8bit定数をメモリーにセットしたい.
movlf	macro	LITERAL_,MEMORY_
	movlw	LITERAL_
	movwf	MEMORY_
	endm

;-----------------------------------------------------------------------------
;	アドレス定数を2byteワークにセットしたい.
lea		macro	MEMORY_,LITERAL_
	movlw	LOW(LITERAL_)
	movwf	MEMORY_
	movlw	HIGH(LITERAL_)
	movwf	(MEMORY_ +	1)
	endm

;-----------------------------------------------------------------------------
;	変数と定数を比較して、一致したら分岐.
if_eq	macro	MEMORY_,LITERAL_,MNE_,LABEL_
	movf	MEMORY_,W
	xorlw	LITERAL_
	bz		LABEL_
	endm

;-----------------------------------------------------------------------------
;	変数と定数を比較して、異なっていたら分岐.
if_ne	macro	MEMORY_,LITERAL_,MNE_,LABEL_
	movlw	LITERAL_
	cpfseq	MEMORY_
	MNE_	LABEL_
	endm

;-----------------------------------------------------------------------------
;	変数と定数を比較して、大きいか等しいなら分岐.
if_le	macro	MEMORY_,LITERAL_,MNE_,LABEL_
	movlw	LITERAL_
	cpfsgt	MEMORY_			; skip if (f) > (W)
	MNE_	LABEL_			; if (f) <= LITERAL_ goto LABEL_
	endm

;-----------------------------------------------------------------------------
;	変数と定数を比較して、小さいか等しいなら分岐.
if_ge	macro	MEMORY_,LITERAL_,MNE_,LABEL_
	movlw	LITERAL_
	cpfslt	MEMORY_			; skip if (f) < (W)
	MNE_	LABEL_			; if (f) >= LITERAL_ goto LABEL_
	endm

;	注意:	MNE_ が２ワードに跨る命令( goto , call など )の場合、ハングする.
;			基本的に bra , rcall しか使えない.
;			しかし、このアセンブラは警告しない.
;-----------------------------------------------------------------------------
;	gpasmでは dw がうまく機能しないのでdb２個で代替.
dw2	macro	ADDRESS_
	dw		ADDRESS_
;;	db		LOW(ADDRESS_)
;;	db		HIGH(ADDRESS_)
	endm

;-----------------------------------------------------------------------------
; Debug messages over UART
#define UART_DEBUG	0
#if UART_DEBUG
UD_INIT	macro
	clrf	TXSTA
	clrf	RCSTA
	clrf	BAUDCON
	bsf	TXSTA, BRGH
	movlw	((.48000000 / .16 / .115200) - 1)	; .25 = 115200
	movwf	SPBRG
	bsf	RCSTA, SPEN
	bsf	TXSTA, TXEN
	endm
UD_TX	macro	BYTE
	btfss	PIR1, TXIF
	bra	$ - 2
	movlw	BYTE
	movwf	TXREG
	bcf	PIR1, TXIF
	endm
UD_TXB	macro	BYTE
	btfss	PIR1, TXIF
	bra	$ - 2
	movf	BYTE, W
	addlw	0x30
	movwf	TXREG
	bcf	PIR1, TXIF
	endm
#else
UD_INIT	macro
	endm
UD_TX	macro	BYTE
	endm
UD_TXB	macro	BYTE
	endm
#endif ; UART_DEBUG
;-----------------------------------------------------------------------------
