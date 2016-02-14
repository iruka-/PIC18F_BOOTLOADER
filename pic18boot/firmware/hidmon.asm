; Tabsize: 4
;-----------------------------------------------------------------------------
;	H I D m o n  : メイン処理
;-----------------------------------------------------------------------------
	#include "p18fxxxx.inc"
	#include "usb.inc"
	#include "boot.inc"
	#include "boot_if.inc"
	#include "usb_defs.inc"
	#include "hidcmd.inc"
	#include "io_cfg.inc"
;-----------------------------------------------------------------------------
; 公開API:
;	hid_process_cmd		// HID_Reportを解析し、返答パケットを作成する.
;	cmd_echo			// エコーバックを行う.
;	cmd_peek			// メモリー読み出しを行う.
;	cmd_poke			// メモリー書き込みを行う.
;-----------------------------------------------------------------------------
; 主要ワーク:
;	boot_cmd[64];		// ホストから送りつけられたHID_Reportパケット.
;	boot_rep[64];		// ホストに返送するHID_Reportパケット.
;-----------------------------------------------------------------------------

;//	Version 1.1
#define	DEV_VERSION_H	1
#define	DEV_VERSION_L	1
#define	DEV_BOOTLOADER	1
;-----------------------------------------------------------------------------
;	外部関数.
;-----------------------------------------------------------------------------
	extern	cmd_page_tx		; Flash書き込み処理.
	extern	cmd_page_erase	; Flash消去.
	extern	usb_state_machine

; write eeprom EEADR,EEDATA must be preset, EEPGD must be cleared       
	extern	eeprom_write

;-----------------------------------------------------------------------------
;	外部変数(バッファ).
;-----------------------------------------------------------------------------
	extern	ep1Bo
	extern	ep1Bi
;;	extern	boot_cmd		; cmd受信用バッファ.
;;	extern	boot_rep		; rep送信用バッファ.

	extern	SetupPktCopy
	extern	usb_init

#if USE_SRAM_MARK
	extern	usb_initialized
#endif

;-----------------------------------------------------------------------------
;	グローバル変数.
;-----------------------------------------------------------------------------
	
	global	cmdsize;
;-----------------------------------------------------------------------------
;	ローカル変数.
;-----------------------------------------------------------------------------
BOOT_DATA	UDATA

cmdsize		res	1	; 読み出しサイズ.
area		res	1	; 読み出しエリア.
ToPcRdy		res	1	; コマンド実行結果をPCに返却するなら1.

poll_mode	res	1	; POLLコマンド受付時に実行する読み取りモード (0=digital 0xAx=analog)
poll_addr_l	res	1	; POLLコマンド受付時に実行するメモリー読み出しアドレス
poll_addr_h	res	1	; POLLコマンド受付時に実行するメモリー読み出しアドレス

;pollstat	res	1	; POLLコマンド待ち=1 POLL 実行済み=0
;str_chr		res	1	; 文字出力用テンポラリワーク
;key_chr		res	1	; 文字入力用バッファ(１文字のみ) 0 のときは入力無し.
;hid_chr		res	1	; HIDブロックを受信したら1.
;snapcnt		res 1

#if	0
		global	BiosPutcBuf
		global	BiosGetsBuf

;	Putc/Getc用
BiosPutcBuf		res		8	;;64
BiosGetsBuf		res		8	;;64
#endif

		global	boot_cmd
		global	boot_rep
;-----------------------------------------------------------------------------
HID_BUF	UDATA	DPRAM+0x70

boot_cmd		res		64	;;EP0_BUFF_SIZE
boot_rep		res		64	;;BOOT_REP_SIZE	; CtrlTrfDataと共用にします.

;--------------------------------------------------------------------------
;//	メモリー種別.
#define	AREA_RAM     0
#define	AREA_EEPROM  0x40
#define	AREA_PGMEM   0x80
#define	AREA_MASK	 0xc0
#define	SIZE_MASK	 0x3f


; boot_cmd[64] の内訳.
#define	size_area	boot_cmd+1	;size(0..63)|area(bit7,6)
#define	addr_l		boot_cmd+2
#define	addr_h		boot_cmd+3
#define	data_0		boot_cmd+4	;書き込みデータ.
#define	data_1		boot_cmd+5	;書き込みマスク.



#define	UOWNbit		7

#define	REPLY_REPORT	0xaa


;#define mHIDRxIsBusy()              HID_BD_OUT.Stat.UOWN
;#define mHIDTxIsBusy()              HID_BD_IN.Stat.UOWN



BOOT_ASM_CODE	CODE

;-----------------------------------------------------------------------------
;	
;-----------------------------------------------------------------------------
	global	bios_task
bios_task
	movlb	GPR0
	;;; アプリが単体起動して、最初の１回目にbios_taskが呼ばれたら、
	;;; usbの初期化を行う必要がある.
#if USE_SRAM_MARK
	if_ne	usb_initialized  ,0x55,		bra,re_init
	if_ne	usb_initialized+1,0xaa,		bra,re_init
	bra		hid_process_cmd
#endif

	;; USB 未初期化を検出した場合、再初期化.
re_init
	rcall	usb_init
	;;; ↓↓↓
;-----------------------------------------------------------------------------
;	HIDパケットに対する処理コード.
;-----------------------------------------------------------------------------

	global	hid_process_cmd
hid_process_cmd
	movlb	GPR0

	rcall	usb_state_machine

;	// 返答パケットが空であること、かつ、
;	// 処理対象の受信データがある.
;	if((!mHIDRxIsBusy())) {

	btfsc	ep1Bi ,UOWNbit					; SIE 稼動中?(返答中)
	bra		hid_proc_end					; 稼動中なのでコマンド処理しない.

	btfsc	ep1Bo ,UOWNbit					; SIE (コマンド受信中)
	bra		hid_proc_poll					; コマンドは届いていない.


; コマンド処理
;	{
		rcall	switch_case_cmd
;;;		rcall	led0_flip
;;;		rcall	snapshot

		;; 次受信をキック.
;		lfsr	FSR0 , ep1Bo
;		rcall	mUSBBufferReady
;	}
hid_proc_1
;	// 必要なら、返答パケットをインタラプト転送(EP1)でホストPCに返却する.
;	if( ToPcRdy ) {
;		if(!mHIDTxIsBusy()) {
;			HIDTxReport64((char *)&PacketToPC);
;			ToPcRdy = 0;
;
;			if(poll_mode!=0) {
;				if(mHIDRxIsBusy()) {	//コマンドが来ない限り送り続ける.
;					make_report();
;				}
;			}
;		}
;	}
;	rcall	led0_flip

	if_eq	ToPcRdy	, 0 , 	bra,hid_proc_2	; ToPcRdy==0ならUSB返答しない.
	btfsc	ep1Bi ,UOWNbit					; SIE 稼動中?(返答中)
	bra		hid_proc_3						; 稼動中なので返答しない.
		
		clrf	ToPcRdy
		;; 返信をキック.
		lfsr	FSR0 , ep1Bi
		rcall	mUSBBufferReady
;;		bra		hid_proc_3

hid_proc_2

hid_proc_3

	;; 次受信をキック.
	lfsr	FSR0 , ep1Bo
;;	rcall	mUSBBufferReady
;;	return

;#define mUSBBufferReady(buffer_dsc)                                         \
;{                                                                           \
;    buffer_dsc.Stat._byte &= _DTSMASK;          /* Save only DTS bit */     \
;    buffer_dsc.Stat.DTS = !buffer_dsc.Stat.DTS; /* Toggle DTS bit    */     \
;    buffer_dsc.Stat._byte |= _USIE|_DTSEN;      /* Turn ownership to SIE */ \
;}
mUSBBufferReady
	movlw	64
	st		PREINC0
	ld		POSTDEC0
	
	ld		INDF0
	xorlw	_DTSMASK
	andlw	_DTSMASK
	st		INDF0
	iorlw	_USIE|_DTSEN
	st		INDF0
hid_proc_end
	return


;-----------------------------------------
hid_proc_poll
;;	btfsc	ep1Bi ,UOWNbit					; SIE 稼動中?(返答中)
;;	bra		hid_proc_end					; 稼動中なのでコマンド処理しない.

	if_eq	poll_mode,0		,bra,hid_proc_poll_end

		rcall	make_report
		;; 返信をキック.
		lfsr	FSR0 , ep1Bi
		rcall	mUSBBufferReady

hid_proc_poll_end
	return

#if	0	;// debug only
led0_flip
	btfsc	LED,LED_PIN0
	bra		led0_off
led0_on
	bsf		LED, LED_PIN0	;; debug
	return
led0_off
	bcf		LED, LED_PIN0	;; debug
	return

snapshot
	lfsr	FSR2,0x400
	lfsr	FSR0,0
	movlw	0
	rcall	cmd_peek_ram_l	;; fsr2->fsr0

	incf	snapcnt
	movff	snapcnt,0x6f
	return
#endif

;-----------------------------------------------------------------------------
;	hidmonコマンドに対する処理コード.
;-----------------------------------------------------------------------------
switch_case_cmd
	;; boot_cmd[] の最初のバイトを boot_rep[]にコピー.
	ld		boot_cmd
	st		boot_rep

	andlw	1
	st		ToPcRdy

	ld		boot_cmd
	dcfsnz	WREG	; HIDASP_TEST         1 ;//エコーバック(R)
	bra		cmd_echo		
	dcfsnz	WREG	; HIDASP_BOOT_EXIT	  2 ;//bootloadを終了し、アプリケーションを起動する.
	bra		cmd_reset		
	decf	WREG	; reserved            3

	dcfsnz	WREG	; HIDASP_POKE         4	;//メモリー書き込み.
	bra		cmd_poke		
	dcfsnz	WREG	; HIDASP_PEEK         5	;//メモリー読み出し.(R)
	bra		cmd_peek		
	dcfsnz	WREG	; HIDASP_JMP	      6	;//指定番地のプログラム実行.
	bra		cmd_jmp			
	dcfsnz	WREG	; HIDASP_SET_MODE     7	;//pollモードの設定 (R)
	bra		cmd_set_mode	
	dcfsnz	WREG	; HIDASP_PAGE_ERASE	  8 ;//Flash消去.
	bra		cmd_page_erase	
	dcfsnz	WREG	; HIDASP_KEYINPUT	  9 ;// ホスト側のキーボードを送る.
	bra		cmd_keyinput	
    dcfsnz	WREG    ; HIDASP_PAGE_WRITE   10;//
	bra		cmd_page_tx		
;;-----------------------------------------------------------------------------
;;	不明なパケットに対する応答.
;;-----------------------------------------------------------------------------
unknown_cmd
return_hid_process_cmd
	return



;-----------------------------------------------------------------------------
;	boot_rep[64] 返却バッファのセットアップ.
;-----------------------------------------------------------------------------

#ifdef __18F14K50
#define	DEVICE_ID	DEV_ID_PIC18F14K
#else
#define	DEVICE_ID	DEV_ID_PIC
#endif

;-----------------------------------------------------------------------------
;	buf[0] = HIDASP_ECHO;
;	buf[1] = ECHO BACK DATA;
cmd_echo
	lfsr	FSR0 , boot_rep+1
;;	movff	boot_cmd 		, POSTINC0	; [0] = CMD_ECHO
	movlf	DEVICE_ID		, POSTINC0	; [1] = DEV_ID
	movlf	DEV_VERSION_L	, POSTINC0	; [2] = HIDmon Device Version lo
	movlf	DEV_VERSION_H	, POSTINC0	; [3] = HIDmon Device Version hi
	movlf	DEV_BOOTLOADER	, POSTINC0	; [4] = AVAILABLE BOOTLOADER FUNC
	movff	boot_cmd+1   	, POSTINC0	; [5] = ECHO BACK DATA
	return



;-----------------------------------------------------------------------------
;	変数 cmdsize と area をセットする.
;	buf[1] = HIDASP_PEEK;
;	buf[2] = size | area;
;
	global	get_size_arena
get_size_arena
	ld		size_area
	andlw	SIZE_MASK
	
	bnz		get_size_a1
	movlw	SIZE_MASK + 1	; size==0 は 64と解釈する.
get_size_a1
	st		cmdsize

	ld		size_area
	andlw	AREA_MASK
	st		area
	return

;-----------------------------------------------------------------------------
;	buf[0] = HIDASP_PEEK;
;	buf[1] = size | area;
;	buf[2] = addr;
;	buf[3] =(addr>>8);
cmd_peek
	;; 転送先は boot_rep
	lfsr	FSR0 , boot_rep
	
	rcall	get_size_arena
	bz		cmd_peek_ram
	if_eq	area,AREA_EEPROM	,bra ,cmd_peek_eeprom
;-----------------------------------------------------------------------------
;	ROM 読み出し.
;-----------------------------------------------------------------------------
cmd_peek_rom
	;; 読み出し元 (ROM) アドレスをセット.
	clrf	TBLPTRU
	movff	addr_h, TBLPTRH
	movff	addr_l, TBLPTRL

	;; 読み出し先＝送信バッファをセット.
	;lfsr	FSR0 , boot_rep
	;; ReportID をセット.
	;;movlf	REPORT_ID1 , POSTINC0

	;; 転送長を WREG にセット.
	movf	cmdsize , W
cmd_peek_rom_l
	tblrd*+						;; ROMから読み出す.
	movff	TABLAT , POSTINC0	;; ROMのラッチを 送信バッファに転送.
	decfsz	WREG
	bra		cmd_peek_rom_l			;; 転送長分だけ繰り返す.
cmd_peek_rom_q
	return
;-----------------------------------------------------------------------------
;	RAM 読み出し.
;-----------------------------------------------------------------------------
cmd_peek_ram
	movff	addr_h, FSR2H
	movff	addr_l, FSR2L

	;; 転送長を WREG にセット.
	movf	cmdsize , W

memcpy_fsr2to0

cmd_peek_ram_l
	movff	POSTINC2 , POSTINC0		;; RAMのエリアを 送信バッファに転送.
	decfsz	WREG
	bra		cmd_peek_ram_l			;; 転送長分だけ繰り返す.
cmd_peek_ram_q
	return

;-----------------------------------------------------------------------------
;	RAM 読み出し.
;-----------------------------------------------------------------------------
cmd_peek_eeprom
	ld		addr_l		; EEPROM address
	st		EEADR
	clrf	EECON1, W

	;; 転送長を WREG にセット.
	;ld		cmdsize		--> WREGは使わず直接decする.
cmd_peek_eeprom_l
	bsf		EECON1, RD			; Read data
	ld		EEDATA
	st		POSTINC0
	incf	EEADR, F			; Next address
	decfsz	cmdsize			;WREG
	bra		cmd_peek_eeprom_l
	return
;-----------------------------------------------------------------------------


;-----------------------------------------------------------------------------
;	buf[0] = HIDASP_POKE;
;	buf[1] = size | area;
;	buf[2] = addr;
;	buf[3] =(addr>>8);
;	buf[4] = data_0;
;	buf[5] = data_1;
cmd_poke
	rcall	get_size_arena
	bz		cmd_poke_ram
;;	if_eq	area,AREA_EEPROM	,bra ,cmd_poke_eeprom
;-----------------------------------------------------------------------------
;	EEPROM 書き込み.
;-----------------------------------------------------------------------------
cmd_poke_eeprom
	ld		addr_l		; EEPROM address
	st		EEADR
	clrf	EECON1, W

;;write_eeprom_loop
	ld		data_0
	st		EEDATA
	bcf		EECON1, EEPGD	; Access EEPROM (not code memory)
	rcall	eeprom_write
	btfsc	EECON1, WR			; Is WRITE completed?
	bra		$ - 2				; Wait until WRITE complete
;	incf	EEADR, F			; Next address
;;	decfsz	cntr
;;	bra		write_eeprom_loop
	bcf		EECON1, WREN	; Disable writes
	return

;	flash.asm 側に実装.
;eeprom_write
;	bcf		EECON1, CFGS	; Access code memory (not Config)
;	bsf		EECON1, WREN	; Enable write
;	movlf	0x55,EECON2
;	movlf	0xAA,EECON2
;	bsf		EECON1, WR	; Start flash/eeprom writing
;	clrf	hold_r		; hold_r=0
;	return

;-----------------------------------------------------------------------------
;	RAM 書き込み.
;-----------------------------------------------------------------------------
;	uchar data=cmd->memdata[0];
;	uchar mask=cmd->memdata[1];
;	if(mask) {	//マスク付の書き込み.
;		*p = (*p & mask) | data;
;	}else{			//べた書き込み.
;		*p = data;
;	}

cmd_poke_ram
	movff	addr_h, FSR0H
	movff	addr_l, FSR0L
	ld		data_1
	bz		direct_write_ram
; MASK 書き込み.
	andwf	INDF0 , W
	iorwf	data_0, W
	st		data_0
direct_write_ram
	movff	data_0 , POSTINC0		;; RAMのエリアを書き換え.
	return


#if	0
;-----------------------------------------------------------------------------
;	USB: HID_Report IN から呼び出される.
;-----------------------------------------------------------------------------
	global	cmd_poll
cmd_poll
;;	movlw	REPORT_ID2				; HID ReportID = 4 (poll)
	cpfseq	(SetupPktCopy + wValue)	; request HID_ReportID
	return
	;; if( wValue == REPORT_ID2 ) {
		movlw	0
		cpfseq	poll_mode
		;; if ( pagemode != 0 ) goto cmd_poll_string
		bra		cmd_poll_string

		;; else {
			movff	pageaddr_l,FSR0L
			movff	pageaddr_h,FSR0H
			ld		INDF0 				; ポート読み取り.
;;;			st		boot_rep + 1		; 返却バッファ[1]に書く.
			movff	WREG,boot_rep + 1	; 返却バッファ[1]に書く.
;;			movlf	REPORT_ID2,boot_rep	; HID ReportID = 2 (poll)を 返却バッファ[0]に書く.
		;; }
	;; }

	return
#endif

#if	0
;-----------------------------------------------------------------------------
;	USB: ホストPCからの任意をHIDデータ(63byte)を受け取る.
;-----------------------------------------------------------------------------
;cmd_gethid
;--- EP0 in にそのままコピーする.
;	lfsr	FSR0,BiosGetsBuf
;	lfsr	FSR2,boot_rep
;	movlw	64
;	rcall	memcpy_fsr2to0		; memcpy(boot_rep,BiosPutcBuf,62);
;	movlf	1,hid_chr
;	return
;-----------------------------------------------------------------------------
;	USB: HID_Report IN から呼び出される.
;	printf文字列をホストＰＣに送る.
;-----------------------------------------------------------------------------
;	文字列データの格納場所:
;		BiosPutcBuf[2]     = 文字数.
;		BiosPutcBuf[3～63] = 文字列.
;	以下は固定値:
;		BiosPutcBuf[0]     = REPORT_ID2.
;		BiosPutcBuf[1]     = 0xc0 返却情報が文字列であることを示すマーク.
;
cmd_poll_string
	lfsr	FSR2,BiosPutcBuf
;;	movlf	REPORT_ID2,POSTINC2			; HID ReportID = 2 (poll)をBiosPutcBuf[0]に書く.
	movff	pagemode,  POSTINC2			; String Mark を BiosPutcBuf[1]に書く.
;--- EP0 in にそのままコピーする.
	lfsr	FSR2,BiosPutcBuf
	lfsr	FSR0,boot_rep
	movlw	62
	rcall	memcpy_fsr2to0		; memcpy(boot_rep,BiosPutcBuf,62);
	clrf	pollstat
;--- 文字数をリセット.
	lfsr	FSR0,BiosPutcBuf+2
	clrf	INDF0
	return
#endif

#if	0
;-----------------------------------------------------------------------------
;	API: １文字キー入力. 入力なしの場合０を返す.  結果=>WREG
;-----------------------------------------------------------------------------
	global	bios_getc
bios_getc
	movlb	GPR0
	ld		key_chr
	clrf	key_chr
	return
;-----------------------------------------------------------------------------
;	API: １文字出力. (WREG) ===> BiosPutcBuf に溜める.
;-----------------------------------------------------------------------------
;		一杯になったら、USB通信の完了を待って、また溜める.
	global	bios_putc
bios_putc
	movlb	GPR0

	st		str_chr
	lfsr	FSR0,BiosPutcBuf+2	;; 文字数.
	if_ge	INDF0 , 59,		rcall, bios_putc_flush	;;すでに60文字登録済み.

	ld		INDF0				;; WREG=文字数
	addlw	LOW(BiosPutcBuf+3);	;; 文字列バッファの先頭.
	st		FSR0L				;; 文字列バッファの現在位置.
	
	movff	str_chr , INDF0		;; 引数の文字をBiosPutcBuf[3+cnt]に格納.

	lfsr	FSR0,BiosPutcBuf+2	;; 文字数.
	incf	INDF0				;; 文字数 cnt を+1.

	return
;-----------------------------------------------------------------------------
;		一杯になったら、USB通信の完了を待って、また溜める.
;-----------------------------------------------------------------------------
bios_putc_flush
	rcall	bios_task
	lfsr	FSR0,BiosPutcBuf+2	;; 文字数.
	if_ge	INDF0 , 59,		bra, bios_putc_flush	;;すでに60文字登録済み.


	return

#endif

;-----------------------------------------------------------------------------
;	API: HID Reportによる文字列転送モードの設定.
;-----------------------------------------------------------------------------
;	pagemode = 0    : 普通の usb_poll (Graphコマンドで使用)
;	pagemode = 0xAx : HIDmon-2313でのアナログ入力サポートコマンド.
;	pagemode = 0xc0 : 通常の文字列転送.(ホストPCへの引き取りモード)
;	pagemode = 0xc9 : アプリケーション終了予告 (ホストPCがTerminalループを抜ける)
;
;;	global	bios_setmode
;bios_setmode
;	movlb	GPR0
;	st		poll_mode
;
;	return
;-----------------------------------------------------------------------------
;	実行.
;-----------------------------------------------------------------------------
;	uchar bootflag=PacketFromPC.raw[5];
;	if(	bootflag ) {
;		wait_ms(10);
;		usbModuleDisable();		// USBを一旦切り離す.
;		wait_ms(10);
;	}
cmd_jmp
;;	if_eq	data_1,0	,	bra,cmd_jmp_1

	;; USB disconnect
	dcfsnz	data_1			; data_1 == 1 なら USBを切り離す.
	rcall	usb_disconnect	; Disable USB Engine


cmd_jmp_1
	clrf	PCLATU
	ld		addr_h
	st		PCLATH
	ld		addr_l

	movlb	0
	st		PCL
	return						;; たぶん不要.

;-----------------------------------------------------------------------------
;	Reset動作.
;-----------------------------------------------------------------------------
cmd_reset
	rcall	usb_disconnect	 ; Disable USB Engine

#if USE_SRAM_MARK
	movlf	0x55,usb_initialized
	movlf	0x5e,usb_initialized+1
#endif

	reset

;-----------------------------------------------------------------------------

usb_disconnect
	rcall	delay_loop
	bcf     UCON,USBEN      ; Disable USB Engine

delay_loop
	; Delay to show USB device reset
	clrf	cmdsize
cmd_reset_1
	clrf	WREG
cmd_reset_2
	decfsz	WREG
	bra		cmd_reset_2
	decfsz	cmdsize
	bra		cmd_reset_1
	return

;-----------------------------------------------------------------------------
;	buf[0] = HIDASP_SET_MODE;
;	buf[1] = mode (digital=0, analog=0xAx) ;
;	buf[2] = addr;
;	buf[3] =(addr>>8);
;	CMD_POLL で読み出すポートとモードをあらかじめセットする.
cmd_set_mode
	movff	size_area,poll_mode		; size値を 読み取りモードとして扱う.
	movff	addr_l	 ,poll_addr_l
	movff	addr_h	 ,poll_addr_h

make_report
;	//サンプル値を１個だけ返す実装.
	movlf	REPLY_REPORT , boot_rep		;// コマンド実行完了をHOSTに知らせる.
	;;		PacketToPC.raw[1] = 1;
	movlf	1			 , boot_rep+1	;//サンプル数.
	;;		PacketToPC.raw[2] = *poll_addr;
	;; {
		movff	poll_addr_l,FSR0L
		movff	poll_addr_h,FSR0H
		ld		INDF0 				; ポート読み取り.
		st		boot_rep + 2		; 返却バッファ[2]に書く.
	;; }
;	movlf	1,ToPcRdy
;;	return

;-----------------------------------------------------------------------------
;	buf[1] = HIDASP_KEYINPUT;
;	buf[2] = KEY Charactr
;	ホストからキー入力が送られた.
cmd_keyinput
;;	movff	size_area,key_chr		; size値を Character として扱う.
	return
;-----------------------------------------------------------------------------
;
;-----------------------------------------------------------------------------

	END
