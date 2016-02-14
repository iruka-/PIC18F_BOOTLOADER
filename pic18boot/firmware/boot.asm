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
; BootLoader Main code
;-----------------------------------------------------------------------------
	#include "p18fxxxx.inc"
	#include "boot.inc"
	#include "io_cfg.inc"
	#include "usb_defs.inc"
	#include "usb_desc.inc"
	#include "usb.inc"
	#include "boot_if.inc"
;--------------------------------------------------------------------------

#ifdef __18F14K50
	#include "fuse14k.inc"
#else
	#include "fuse.inc"
#endif
;--------------------------------------------------------------------------
; External declarations
	extern	usb_sm_state
	extern	usb_sm_ctrl_state
	extern	ep1Bo
	extern	ep1Bi
	extern	SetupPkt
	extern	SetupPktCopy
	extern	pSrc
	extern	pDst
	extern	Count
	extern	ctrl_trf_session_owner
	extern	ctrl_trf_mem
;	extern	cmd_poll
;	extern	eep_mark_set

;--------------------------------------------------------------------------
; Variables
BOOT_DATA	UDATA
	global	active_protocol
	global	idle_rate

;--------------------------------------------------------------------------
;	64byteバッファ(コマンド返却用)

	extern	boot_rep	;;;res		BOOT_REP_SIZE
;--------------------------------------------------------------------------
;	64byteバッファ(コマンド受け取り用)

	extern	boot_cmd	;;;res		BOOT_REP_SIZE


;--------------------------------------------------------------------------

blink_cnt	res 1
blink_cnth	res 1
;bTRNIFCount	res	1
active_protocol	res	1
idle_rate	res	1
;--------------------------------------------------------------------------


#define	hid_report_in	boot_rep
#define	hid_report_out	boot_cmd


#if	HAVE_ENDPOINT

; EP1 (HID) buffers
;	global	hid_report_in
;	global	hid_report_out
;hid_report_in	res	HID_IN_EP_SIZE	; IN packed buffer
;hid_report_out	res	HID_OUT_EP_SIZE	; OUT packet buffet

#endif



;--------------------------------------------------------------------------
BOOT_ASM_CODE CODE
	extern	usb_init
	extern	usb_sm_ctrl
	extern	usb_sm_reset
	extern	usb_sm_prepare_next_setup_trf
	extern	hid_process_cmd

	extern	desc_tab

#if USE_SRAM_MARK
	extern	usb_initialized
#endif


	extern	appl_reset_vector
;--------------------------------------------------------------------------
; main
; DESCR : Boot Loader main routine.
; WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
; WARNING                                                 WARNING
; WARNING     This code is not a routine!!!               WARNING
; WARNING     RESET command is used to "exit" from main   WARNING
; WARNING                                                 WARNING
; WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
; INPUT : no
; OUTPUT: no
;--------------------------------------------------------------------------
	global	main
main

	; All I/O to Digital mode
#ifdef __18F14K50
	clrf	ANSEL
	clrf	ANSELH
#endif

	movlw	0x0F
	movwf	ADCON1

;;	movlb	GPR0

;;	LEDポートの初期化.
	bcf		LED_TRIS, LED_PIN0
	bcf		LED_TRIS, LED_PIN1
	bcf		LED, LED_PIN0
	bcf		LED, LED_PIN1

	; Decide what to run bootloader or application
#if USE_EEPROM_MARK
	; Check EEPROM mark
	movlw	EEPROM_MARK_ADDR
	movwf	EEADR
	movlw	0x01
	movwf	EECON1
	movlw	EEPROM_MARK
	subwf	EEDATA, W
	bz		bootloader
#endif

#if USE_SRAM_MARK
	; Check SRAM mark
	if_ne	usb_initialized  ,0x55,		bra,mark_end
	if_ne	usb_initialized+1,0x5e,		bra,mark_end

	rcall	clr_ram			; アプリがいきなり起動する場合には,USB未初期化としておく.
	bra		appl_reset_vector	; Run Application FW

mark_end

#endif


#ifdef USE_JP_BOOTLOADER_EN
;	setf	JP_BOOTLOADER_TRIS
	bsf	    JP_BOOTLOADER_TRIS, JP_BOOTLOADER_PIN
#endif

;;	rcall	clr_ram			; アプリがいきなり起動する場合には,USB未初期化としておく.

	; Check bootloader enable jumper
#ifdef USE_JP_BOOTLOADER_EN
	;;
	;; ブートSWがOFFの時は0x800 番地にジャンプする.
	;;
	btfsc	JP_BOOTLOADER_PORT, JP_BOOTLOADER_PIN	; bitが0ならSKIP
	bra		appl_reset_vector	; Run Application FW

#endif

	;;
	;; しかし、何も書き込んでいないROMでは、またここに戻ってくる
	;;	   ような仕掛けがしてある. (0x800番地にここへのJMPがある)

	global 	bootloader_soft_reset
bootloader_soft_reset
	rcall	clr_ram			; アプリがいきなり起動する場合には,USB未初期化としておく.
	; Run bootloader

;;	bra	bootloader
;;	reset
;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;!!!    WARNING NEVER RETURN IN NORMAL WAY   !!!
;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;--------------------------------------------------------------------------
; bootloader
; DESCR : Run the Boot Loader.
; WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
; WARNING                                                 WARNING
; WARNING     This code is not a routine!!!               WARNING
; WARNING     Branch to bootloader occur if firmware      WARNING
; WARNING     updating mode is detected either throuch    WARNING
; WARNING     EEPROM MARK or FW Junper                    WARNING
; WARNING     RESET command is used to "exit"             WARNING
; WARNING     from bootloader                             WARNING
; WARNING                                                 WARNING
; WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
; INPUT : no
; OUTPUT: no
	global	bootloader
bootloader
;	clrf	eep_mark_set	; EEP_MARK will be cleared
	rcall	usb_init
; Main Loop
	global	bootloader_loop
bootloader_loop
	rcall	led_blink
;;	rcall	usb_state_machine
	rcall	hid_process_cmd
	bra	bootloader_loop
;;	reset


;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;!!!    WARNING NEVER RETURN IN NORMAL WAY   !!!
;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;--------------------------------------------------------------------------
; usb_state_machine
; DESCR : Handles USB state machine according to USB Spec.
;         Process low level action on USB controller.
; INPUT : no
; OUTPUT: no
;-----------------------------------------------------------------------------
	global	usb_state_machine
usb_state_machine
	; Bus Activity detected after IDLE state
usb_state_machine_actif
	btfss	UIR, ACTVIF
	bra		usb_state_machine_actif_end

; usbdrv.cに習いコメントアウト.
;;	btfss	UIE, ACTVIE
;;	bra		usb_state_machine_actif_end

usbWakeFromSuspend:
	bcf		UCON, SUSPND
	bcf		UIE, ACTVIE


;/********************************************************************
;Bug Fix: May 14, 2007
;*********************************************************************
;;; bugfix1 : ACTVIFのクリア方法.
;;;	bcf	UIR, ACTVIF
L001:
	btfss	UIR, ACTVIF
	bra		usb_state_machine_actif_end
	bcf		UIR, ACTVIF
	bra 	L001
;;; bugfix1 : ACTVIFのクリア方法: ここまで.


usb_state_machine_actif_end
	; Pointless to continue servicing if the device is in suspend mode.
	btfsc	UCON, SUSPND
	return
	; USB Bus Reset Interrupt.
	; When bus reset is received during suspend, ACTVIF will be set first,
	; once the UCONbits.SUSPND is clear, then the URSTIF bit will be asserted.
	; This is why URSTIF is checked after ACTVIF.
	;
	; The USB reset flag is masked when the USB state is in
	; DETACHED_STATE or ATTACHED_STATE, and therefore cannot
	; cause a USB reset event during these two states.
usb_state_machine_rstif
	btfss	UIR, URSTIF
	bra		usb_state_machine_rstif_end

; usbdrv.cに習いコメントアウト.
;;	btfss	UIE, URSTIE
;;	bra		usb_state_machine_rstif_end
	rcall	usb_sm_reset
usb_state_machine_rstif_end
	; Idle condition detected
usb_state_machine_idleif
	btfss	UIR, IDLEIF
	bra		usb_state_machine_idleif_end

; usbdrv.cに習いコメントアウト.
;;	btfss	UIE, IDLEIE
;;	bra		usb_state_machine_idleif_end
	UD_TX	'I'

usbSuspend
	bsf		UIE, ACTVIE	; Enable bus activity interrupt
	bcf		UIR, IDLEIF
	bsf		UCON, SUSPND	; Put USB module in power conserve
				; mode, SIE clock inactive

;2014-3-18:Windows8.1 problem.
;;;        ; Now, go into power saving
;;;        bcf	PIR2, USBIF	; Clear flag
;;;        bsf	PIE2, USBIE	; Set wakeup source
;;;	sleep
;;;        bcf	PIE2, USBIE


usb_state_machine_idleif_end

#if	0

	; SOF Flag
usb_state_machine_sof
	btfss	UIR, UERRIF
	bra		usb_state_machine_sof_end
; usbdrv.cに習いコメントアウト.
;;	btfss	UIE, UERRIE
;;	bra		usb_state_machine_sof_end
	UD_TX	'F'
	bcf		UIR, SOFIF
usb_state_machine_sof_end

#endif


	; A STALL handshake was sent by the SIE
usb_state_machine_stallif
	btfss	UIR, STALLIF
	bra		usb_state_machine_stallif_end
; usbdrv.cに習いコメントアウト.
;;	btfss	UIE, STALLIE
;;	bra		usb_state_machine_stallif_end
	UD_TX	'T'

usbStallHandler
;;	btfss	UEP0, EPSTALL
	movff	UEP0, WREG
	btfss	WREG, EPSTALL
	bra		usb_state_machine_stallif_clr

;/********************************************************************
;Bug Fix: May 14, 2007 (#F4)
;*********************************************************************
	rcall	usb_sm_prepare_next_setup_trf	; Firmware Work-Around
;;

#ifdef __18F14K50
	movlb	GPRF
#endif

		bcf		UEP0, EPSTALL

#ifdef __18F14K50
	movlb	GPR0
#endif

usb_state_machine_stallif_clr
	bcf		UIR, STALLIF
usb_state_machine_stallif_end

#if	0
	; USB Error flag
usb_state_machine_err
	btfss	UIR, UERRIF
	bra		usb_state_machine_err_end
	btfss	UIE, UERRIE
	bra		usb_state_machine_err_end
	UD_TX	'E'
	bcf		UIR, UERRIF
usb_state_machine_err_end
#endif

	; Pointless to continue servicing if the host has not sent a bus reset.
	; Once bus reset is received, the device transitions into the DEFAULT
	; state and is ready for communication.
;    if( usb_sm_state < USB_SM_DEFAULT ) 
;		return;
	movlw	(USB_SM_DEFAULT - 1)	; Be carefull while changing USB_SM_* constants
	cpfsgt	usb_sm_state
	return
	; Detect Interrupt bit


usb_state_machine_trnif

;	追加されていたloop.
;;	movlf	1,bTRNIFCount
;usb_state_machine_trnif_loop

	btfss	UIR, TRNIF
	bra		usb_state_machine_trnif_end
; usbdrv.cに習いコメントアウト.
	btfss	UIE, TRNIE
	bra		usb_state_machine_trnif_end
	; Only services transactions over EP0.
	; Ignore all other EP transactions.
	rcall	usb_sm_ctrl
	bcf		UIR, TRNIF

;	return

usb_state_machine_trnif_end
;	追加されていたloop.
;	decfsz	bTRNIFCount
;	bra		usb_state_machine_trnif_loop
	return
;-----------------------------------------------------------------------------
; HID
;-----------------------------------------------------------------------------
; usb_sm_HID_init_EP
; DESCR : Initialize Endpoints for HID
; INPUT : no
; OUTPUT: no
;-----------------------------------------------------------------------------
#if	HAVE_ENDPOINT

	global	usb_sm_HID_init_EP
usb_sm_HID_init_EP

#define USE_HID_EP_OUT 1
#if USE_HID_EP_OUT
	movlw	EP_OUT_IN | HSHK_EN
	movff	WREG,HID_UEP	; Enable 2 data pipes
; 
;;;	movlb	HIGH(HID_BD_OUT)
;	HOST_TO_DEV (ep1Bo) <==boot_cmd ホストから送られてくるコマンド.
	movlw	HID_OUT_EP_SIZE
	movwf	HID_BD_OUT+1	;;BDT_CNT(HID_BD_OUT)
	movlw	LOW(hid_report_out)
	movwf	HID_BD_OUT+2	;;BDT_ADRL(HID_BD_OUT)
	movlw	HIGH(hid_report_out)
	movwf	HID_BD_OUT+3	;;BDT_ADRH(HID_BD_OUT)
	movlw	(_USIE | _DAT0 | _DTSEN)
	movwf	HID_BD_OUT	;;BDT_STAT(HID_BD_OUT)
#else
	movlw	(EP_IN | HSHK_EN)
	movff	WREG,HID_UEP	; Enable 2 data pipes
#endif

;;;	movlb	HIGH(HID_BD_IN)
;	DEV_TO_HOST (ep1Bi)	==>boot_rep 処理結果を返送するバッファ.
	movlw	LOW(hid_report_in)
	movwf	HID_BD_IN+2		;;BDT_ADRL(HID_BD_IN)
	movlw	HIGH(hid_report_in)
	movwf	HID_BD_IN+3		;;BDT_ADRH(HID_BD_IN)
	movlw	(_UCPU | _DAT1)
	movwf	HID_BD_IN	;;BDT_STAT(HID_BD_IN)
	movlb	GPR0
;;;	clrf	boot_rep	;;(boot_rep + cmd)
	return

#endif

;--------------------------------------------------------------------------
;	各種USBディスクリプタのアドレスを pSrc にセット、
;		USBディスクリプタのサイズ  を W レジスタにセット
;--------------------------------------------------------------------------
;	引数  W     : desc_tab[] 先頭からの offset byte.
;	結果  pSrc  : host PC に送る descritor のROM 上アドレス.
;		  Count : host PC に送る descritor のBYTE SIZE
;
	global	get_desc_tab
get_desc_tab
	; W を3倍したい.
	movwf	Count	; W を Count にストア.
	addwf	Count,W	; Count に W を加算.
	addwf	Count,W	; Count に W を加算. 都合３倍.

	addlw	LOW(desc_tab)
	movwf	TBLPTRL

	clrf	TBLPTRU
	movlw	HIGH(desc_tab)
	movwf	TBLPTRH

	tblrd*+				;	LOW
	movff	TABLAT,pSrc
	tblrd*+				;   HIGH
	movff	TABLAT,pSrc+1
	tblrd*+				;   SIZE
	movff	TABLAT,Count
	return

;--------------------------------------------------------------------------
; usb_sm_HID_request
; DESCR : Process USB HID requests
;		  クラスリクエスト(HID)を処理する.
; INPUT : no
; OUTPUT: no
;-----------------------------------------------------------------------------
	global	usb_sm_HID_request
usb_sm_HID_request
	UD_TX	'H'
	movf	(SetupPktCopy + Recipient), W
	andlw	RCPT_MASK
	sublw	RCPT_INTF
	btfss	STATUS, Z
	return
usb_sm_HID_rq_rcpt
	movf	(SetupPktCopy + bIntfID), W
	sublw	HID_INTF_ID
	btfss	STATUS, Z
	return
usb_sm_HID_rq_rcpt_id
	; There are two standard requests that we may support.
	; 1. GET_DSC(DSC_HID,DSC_RPT,DSC_PHY);
	; 2. SET_DSC(DSC_HID,DSC_RPT,DSC_PHY);
	movf	(SetupPktCopy + bRequest), W
	sublw	GET_DSC
	bnz	usb_sm_HID_rq_cls
	movf	(SetupPktCopy + bDscType), W
	; WREG = WREG - DSC_HID !!!
	addlw	(-DSC_HID)	; DSC_HID = 0x21
	bz	usb_sm_HID_rq_dsc_hid
	dcfsnz	WREG		; DSC_RPT = 0x22
	bra	usb_sm_HID_rq_dsc_rpt
	dcfsnz	WREG		; DSC_PHY = 0x23
	bra	usb_sm_HID_rq_dsc_phy
usb_sm_HID_rq_dsc_unknown
	UD_TX('!')
	bra	usb_sm_HID_rq_cls
;--------	Get DSC_HID descrptor address
usb_sm_HID_rq_dsc_hid
;	movlw	LOW(USB_HID_DESC)
;	movwf	pSrc
;	movlw	HIGH(USB_HID_DESC)
;	movwf	(pSrc + 1)
;	movlw	USB_HID_DESC_SIZE
	movlw	USB_HID_DESC_off
usb_sm_HID_rq_dsc_hid_end
	bra	usb_sm_HID_rq_dsc_end
;--------	Get DSC_RPT descrptor address
usb_sm_HID_rq_dsc_rpt
;	movlw	LOW(USB_HID_RPT)
;	movwf	pSrc
;	movlw	HIGH(USB_HID_RPT)
;	movwf	(pSrc + 1)
;	movlw	USB_HID_RPT_SIZE
	movlw	USB_HID_RPT_off
usb_sm_HID_rq_dsc_rpt_end
	bra		usb_sm_HID_rq_dsc_end
;--------	Get DSC_PHY descrptor address
usb_sm_HID_rq_dsc_phy
usb_sm_HID_rq_dsc_phy_end
	bra		usb_sm_HID_request_end
;--------
usb_sm_HID_rq_dsc_end
	rcall	get_desc_tab				; pSrc = desc_tab[W].addr , Count = desc_tab[W].cnt
	bsf		ctrl_trf_session_owner, 0
	bsf		ctrl_trf_mem, _RAM
	bra		usb_sm_HID_request_end
;--------
; Class Request
usb_sm_HID_rq_cls
	movf	(SetupPktCopy + bmRequestType), W
	andlw	RQ_TYPE_MASK
	sublw	CLASS
	bz		usb_sm_HID_rq_cls_rq
	UD_TX('*')
	return
;--------
usb_sm_HID_rq_cls_rq
	movf	(SetupPktCopy + bRequest), W
	dcfsnz	WREG	; GET_REPORT = 0x01
	bra		usb_sm_HID_rq_cls_rq_grpt
	dcfsnz	WREG	; GET_IDLE = 0x02
	bra		usb_sm_HID_rq_cls_rq_gidle
	dcfsnz	WREG	; GET_PROTOCOL = 0x03
	bra		usb_sm_HID_rq_cls_rq_gprot
	; SET_REPORT = 0x09 -> 9 - 3 = 6
	; WREG = WREG - 6 !!!
	addlw	(-(SET_REPORT - GET_PROTOCOL))
	bz		usb_sm_HID_rq_cls_rq_srpt
	dcfsnz	WREG	; SET_IDLE = 0x0A
	bra		usb_sm_HID_rq_cls_rq_sidle
	dcfsnz	WREG	; SET_PROTOCOL = 0x0B
	bra		usb_sm_HID_rq_cls_rq_sprot
usb_sm_HID_rq_cls_rq_unknown
	UD_TX('#')
	bra		usb_sm_HID_request_end

;--------	GET_REPORT
;★	HID Reportをhostに返す.
;	実装が必要
;
usb_sm_HID_rq_cls_rq_grpt

	movlw	0		; No data to be transmitted

#if	0
	// control転送のみによるhidmon:
	movlw	LOW(boot_rep)
	movwf	pSrc
	movlw	HIGH(boot_rep)
	movwf	(pSrc + 1)

	rcall	cmd_poll					; hidmon.asm
	movlw	1
	movff	WREG,boot_rep
	movf	(SetupPktCopy + wLength), W
#endif

	bra		usb_sm_HID_rq_cls_rq_end


;--------	SET_REPORT
;★	HID Reportを受け取る.
;	具体的には、HID Reportを boot_cmd[] バッファにコピーする.
;
usb_sm_HID_rq_cls_rq_srpt
	bra		usb_sm_HID_request_end	;; == unknown

#if	0
	movlw	LOW(boot_cmd)
	movwf	pDst
	movlw	HIGH(boot_cmd)
	movwf	(pDst + 1)
	bra		usb_sm_HID_rq_cls_rq_end_ses
#endif

#define GET_SET_IDLE 0
#if GET_SET_IDLE
;--------	GET_IDLE
usb_sm_HID_rq_cls_rq_gidle
	UD_TX('j')
	movlw	LOW(idle_rate)
	movwf	pSrc
	movlw	HIGH(idle_rate)
	movwf	(pSrc + 1)
        movlw	1	; For Count
usb_sm_HID_rq_cls_rq_gidle_end
	bra		usb_sm_HID_rq_cls_rq_end

;--------	SET_IDLE
usb_sm_HID_rq_cls_rq_sidle
	UD_TX('x')
	movff	(SetupPktCopy + (wValue + 1)), idle_rate
usb_sm_HID_rq_cls_rq_sidle_end
	bra		usb_sm_HID_rq_cls_rq_end_ses
#endif
#define GET_SET_PROTOCOL 0
#if 	GET_SET_PROTOCOL

;--------	GET_PROTOCOL
usb_sm_HID_rq_cls_rq_gprot
	UD_TX('y')
	movlw	LOW(active_protocol)
	movwf	pSrc
	movlw	HIGH(active_protocol)
	movwf	(pSrc + 1)
	movlw	1	; For Count
	bra		usb_sm_HID_rq_cls_rq_end

;--------	SET_PROTOCOL
usb_sm_HID_rq_cls_rq_sprot
	UD_TX('z')
	movf	(SetupPktCopy + wValue), W
	movwf	active_protocol
usb_sm_HID_rq_cls_rq_sprot_end
	bra		usb_sm_HID_rq_cls_rq_end_ses
#endif


usb_sm_HID_rq_cls_rq_end
	movwf	Count
	bcf		ctrl_trf_mem, _RAM		; 返却データは RAM上のbufferである.
usb_sm_HID_rq_cls_rq_end_ses
	bsf		ctrl_trf_session_owner, 0
;--------
#if !GET_SET_IDLE
usb_sm_HID_rq_cls_rq_gidle
usb_sm_HID_rq_cls_rq_sidle
#endif

#if !GET_SET_PROTOCOL
usb_sm_HID_rq_cls_rq_gprot
usb_sm_HID_rq_cls_rq_sprot
#endif

usb_sm_HID_request_end
	return


;--------------------------------------------------------------------------
;	LED点灯
;--------------------------------------------------------------------------
;;led_on
#if	VECT
	;// 0x800動作時はRC1 LEDを点滅
#define	BLINK_LED_PIN LED_PIN1
#else
	;// 0x000動作時はRC0 LEDを点滅
#define	BLINK_LED_PIN LED_PIN0
;;	bsf	LED, LED_PIN0
#endif
;;	return

;--------------------------------------------------------------------------
;	LED点滅
;--------------------------------------------------------------------------
led_blink
	incf	blink_cnt
	bnc		blink_l1
	incf	blink_cnth
blink_l1
	btfss	blink_cnth,7
	bsf	LED, BLINK_LED_PIN

	btfsc	blink_cnth,7
	bcf	LED, BLINK_LED_PIN

	return

	global	clr_ram_loop
clr_ram
	;; CLEAR 0400 .. 04ff 
	lfsr	FSR2,DPRAM			;; 0x400 or 0x200(14k50)
	movlw	0
clr_ram_loop
	clrf	POSTINC2
	decfsz	WREG
	bra		clr_ram_loop
	return

;--------------------------------------------------------------------------
;
;--------------------------------------------------------------------------
	END
