;-----------------------------------------------------------------------------
;	H I D m o n  : メイン処理
;-----------------------------------------------------------------------------
	#include "p18fxxxx.inc"
	#include "boot.inc"
	#include "boot_if.inc"
	#include "usb_defs.inc"
	#include "hidcmd.inc"
;-----------------------------------------------------------------------------
; 公開API:
;	cmd_page_tx			// Flash書き込みを行う.
;-----------------------------------------------------------------------------
; 主要ワーク:
;	boot_cmd[64];		// ホストから送りつけられたHID_Reportパケット.
;	boot_rep[64];		// ホストに返送するHID_Reportパケット.
;-----------------------------------------------------------------------------


;-----------------------------------------------------------------------------
;	外部関数.
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
;	グローバル変数.
;-----------------------------------------------------------------------------
	extern	boot_cmd		; cmd受信用バッファ.
	extern	boot_rep		; rep送信用バッファ.
	extern	cmdsize
	extern	get_size_arena

#define	size_area	boot_cmd+1	;size(0..63)|area(bit7,6)
#define	addr_l		boot_cmd+2
#define	addr_h		boot_cmd+3
#define	data_0		boot_cmd+4	;書き込みデータ.
#define	data_1		boot_cmd+5	;書き込みマスク.

;-----------------------------------------------------------------------------
;	ローカル変数.
;-----------------------------------------------------------------------------
BOOT_DATA	UDATA

cntr		res	1
hold_r		res	1	; Current Holding Register for tblwt


BOOT_ASM_CODE	CODE

;-----------------------------------------------------------------------------
	global	cmd_page_erase
cmd_page_erase
	rcall	get_size_arena
;---------------------------------------------------------------------
;	boot_cmd.addr から64byte単位でboot_cmd.sizeブロックを消去する.
;---------------------------------------------------------------------
	rcall	load_address	; TBLPTR = addr
erase_code_loop
	; while( size_x64 )

	; Erase 64 bytes block
	bsf		EECON1, FREE	; Enable row Erase (not PRORGRAMMING)
	rcall	flash_write	; Erase block. EECON1.FREE will be cleared by HW

	; TBLPTR += 64
	movlw	0x40
	addwf	TBLPTRL
	movlw	0x00
	addwfc	TBLPTRH

	decfsz	cmdsize
	bra		erase_code_loop
	return





;-----------------------------------------------------------------------------
	global	cmd_page_tx
cmd_page_tx
	rcall	get_size_arena
;----------------------------------------------------------------------
;	boot_cmd.code[] から boot_cmd.size バイトだけ、Flashに書き込む
;----------------------------------------------------------------------
;引数:
;	boot_cmd.code[] = 書き込みするデータ(最大64byte-5)
;	boot_cmd.addr   = 書き込み先CODE ROMのアドレス
;	boot_cmd.size   = 書き込みbyte数 (最大64byteまで) 4の倍数.
;---------------------------------------------------------------------
write_code
	; TBLPTR = addr
	rcall	load_address
	lfsr	FSR0,data_0
	tblrd*-					; TBLPTR--

	; while( cntr-- )
write_code_loop
	movff	POSTINC0, TABLAT
	tblwt+*			; *(++Holding_Register) = *data++
	incf	hold_r		; hold_r++
#ifdef __18F14K50
	btfsc	hold_r, 4	; if( hold_r == 0x10 )  End of Holding Area:18F14K50
#else
	btfsc	hold_r, 5	; if( hold_r == 0x20 )  End of Holding Area:18F2550/4550
#endif
	rcall	flash_write	;     write_flash       Dump Holding Area to Flash

	decfsz	cmdsize
	bra		write_code_loop

	tstfsz	hold_r		; if( hold_r != 0 )     Holding Area not dumped
	rcall	flash_write	;       write_flash     Dump Holding Area to Flash
;			↑ 最後の端数dataも(必要なら)書き込む.
	return

;-----------------------------------------------
;	TBLPTR に boot_cmd.addr の値をセットする.
;	ついでにhold_r に TBLPTRLの下位5bitをセットする.
;-----------------------------------------------

;-----------------------------------------------------------------------------
;	buf[1] = HIDASP_POKE;
;	buf[2] = size | area;
;	buf[3] = addr;
;	buf[4] =(addr>>8);
;	buf[5～] : flash data;


; TBLPTR = boot_rep.addr = boot_cmd.addr; hold_r = boot_cmd.addr_lo & 0x1F
load_address
	movff	addr_h	, TBLPTRH
	movf	addr_l	, W	
	movwf	TBLPTRL
	andlw	0x1F
	movwf	hold_r
	return

;----------------------------------------------------------------------

; write flash (if EECON1.FREE is set will perform block erase)
flash_write
	bsf		EECON1, EEPGD	; Access code memory (not EEPROM)
; write eeprom EEADR,EEDATA must be preset, EEPGD must be cleared
	global	eeprom_write
eeprom_write
	bcf		EECON1, CFGS	; Access code memory (not Config)
	bsf		EECON1, WREN	; Enable write
	movlf	0x55,EECON2
	movlf	0xAA,EECON2
	bsf		EECON1, WR	; Start flash/eeprom writing
	clrf	hold_r		; hold_r=0
	return
;----------------------------------------------------------------------
	END
