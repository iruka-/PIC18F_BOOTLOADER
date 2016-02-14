/********************************************************************
 *	簡易モニタ
 ********************************************************************
 */
#include <string.h>
#include "usb/typedefs.h"
#include "usb/usb.h"
#include "io_cfg.h"
#include "monit.h"
#include "hidcmd.h"
#include "picwrt.h"
#include "timer2.h"

#include "config.h"

/********************************************************************
 *	スイッチ
 ********************************************************************
 */
/********************************************************************
 *	定義
 ********************************************************************
 */
#if defined(__18F14K50)
#define	SOFTWARE_SPI_MODE	1
#define	DEVID	DEV_ID_PIC18F14K	//0x14
#else
#define	SOFTWARE_SPI_MODE	1		//18F4550だとSPI受信データが化ける.
#define	DEVID	DEV_ID_PIC			//0x25
#endif

#define	VER_H	1
#define	VER_L	1


//#define	POLL_ANALOG		0xa0
#define	REPLY_REPORT	0xaa

extern void user_cmd(int arg);

/********************************************************************
 *	データ
 ********************************************************************
 */
#pragma udata access accessram


//	AVR書き込み用コンテクスト.
uchar near page_mode;
uchar near page_addr;
uchar near page_addr_h;

//	データ引取りモードの設定.
uchar near  poll_mode;	// 00=無設定  0xa0=アナログサンプル  0xc0=デジタルサンプル
						// 0xc9=runコマンドターミネート
uchar near *poll_addr;	//

//	コマンドの返信が必要なら1をセットする.
uchar near ToPcRdy;


		uchar	puts_mode;

extern	uchar 	puts_buf[];
extern	uchar 	puts_ptr;

#if	TIMER2_INT_SAMPLE			// タイマー２割り込みでPORTサンプル.
uchar	near	poll_wptr;
uchar	near	poll_rptr;

#pragma udata 	PollBuffer

uchar	poll_buf[256];

#endif


#pragma udata SomeSectionName1
Packet PacketFromPC;			//入力パケット 64byte


#if defined(__18F14K50)
#pragma udata access accessram
Packet near PacketToPC;				//出力パケット 64byte
#else
#pragma udata SomeSectionName2
Packet PacketToPC;				//出力パケット 64byte
#endif




#define	Cmd0	PacketFromPC.cmd
#define	Data0	PacketFromPC.data[0]
#define	Mask1	PacketFromPC.data[1]





/********************************************************************
 *	ISPモジュール インクルード.
 ********************************************************************
 */
#pragma code

#if	SOFTWARE_SPI_MODE
#include "usi_pic18s.h"
#else
#include "usi_pic18.h"
#endif


static void isp_command(uchar *data) {
	uchar i;
	for (i=0;i<4;i++) {
		PacketToPC.raw[i]=usi_trans(data[i]);
	}
}

#if 1	/* Add by senshu */
unsigned char ReadEE(unsigned char Address)
{
	EECON1 = 0x00;
	EEADR = Address;
	EECON1bits.RD = 1;
	return (EEDATA);
}

void WriteEE(unsigned char Address, unsigned char Data)
{
	EEADR = Address;
	EEDATA = Data;
	EECON1 = 0b00000100;	// Setup writes: EEPGD=0,WREN=1
	/* Start Write */
	EECON2 = 0x55;
	EECON2 = 0xAA;
	EECON1bits.WR = 1;
	while( EECON1bits.WR);	// Wait till WR bit is clear, hopefully not long enough to kill USB
}
#endif

#if RAM_SERIAL	/* Add by senshu */
/*
  EEPROM MAP

	0x00（ユーザ領域）
	　:
	0xef（ユーザ領域）

	0xf0 シリアル番号の総文字数（現段階では、4に限定）
	0xf1 シリアル番号（1文字目）
	0xf2 シリアル番号（2文字目）
	0xf3 シリアル番号（3文字目）
	0xf4 シリアル番号（4文字目）
	0xf5 シリアル番号（5文字目）
	0xf6 シリアル番号（6文字目）
	0xf7 予備
	 :
	0xfc 予備
	0xfd USBシリアルの初期化ボーレート
	0xfe DTR/RTSなどの有効・無効の指定など
	0xff JUMPアドレス(H) .... アプリの上位8ビット指定
	※下位は 0x00を想定
 */
#define EE_SERIAL_LEN 0xf0
#define EE_SERIAL_TOP 0xf1


void set_serial_number()
{
	unsigned char ch;
	int i, j;
	if (ReadEE(EE_SERIAL_LEN) == 4) {
		/* データの妥当性を検証 */
		j = 0;
		for (i = EE_SERIAL_TOP; i < EE_SERIAL_TOP + 4; i++) {
			ch = ReadEE(i);
			if (' ' <= ch && ch <= 'z') {
				j++;
			}
		}
		/* データが妥当であれば、値をセットする */
		if (j == 4) {
			for (i = 0; i < 4; i++) {
				sd003.string[i] = ReadEE (EE_SERIAL_TOP + i);
			}
		}
	}
}

#endif

/********************************************************************
 *	初期化.
 ********************************************************************
 */
void UserInit(void)
{
    mInitAllLEDs();
	timer2_interval(5);	// '-d5'
	poll_mode = 0;
	poll_addr = 0;
	puts_ptr = 0;
	ToPcRdy = 0;

#if	TIMER2_INT_SAMPLE			// タイマー２割り込みでPORTサンプル.
	poll_wptr=0;
	poll_rptr=0;
#endif
}


/********************************************************************
 *	ポート・サンプラ
 ********************************************************************
 */
#if	TIMER2_INT_SAMPLE			// タイマー２割り込みでPORTサンプル.
void mon_int_handler(void)
{
	uchar c;

	if(	poll_mode == 0) return;

	if(	poll_mode == POLL_ANALOG) {
		// ANALOG
        while(ADCON0bits.NOT_DONE);     // Wait for conversion

		c = ADRESL;			//ADC ポート読み取り,
		poll_buf[poll_wptr] = c;
		poll_wptr++;
		c = ADRESH;			//ADC ポート読み取り,
		poll_buf[poll_wptr] = c;
		poll_wptr++;

        ADCON0bits.GO = 1;              // Start AD conversion
		return;
	}else{
		// DIGITAL
		c = *poll_addr;			//ポート読み取り,
		poll_buf[poll_wptr] = c;
		poll_wptr++;
	}
}

int mon_read_sample(void)
{
	uchar c;
	if(	poll_rptr != poll_wptr) {
		c = poll_buf[poll_rptr];
		poll_rptr++;
		return c;
	}else{
		return -1;
	}
}

#endif
/********************************************************************
 *	接続テストの返答
 ********************************************************************
 */
void cmd_echo(void)
{
	PacketToPC.raw[1]=DEVID;				// PIC25/14K
	PacketToPC.raw[2]=VER_L;				// version.L
	PacketToPC.raw[3]=VER_H;				// version.H
	PacketToPC.raw[4]=0;					// bootloader
	PacketToPC.raw[5]=PacketFromPC.raw[1];	// ECHOBACK
	ToPcRdy = 1;
}

/********************************************************************
 *	メモリー読み出し
 ********************************************************************
 */
void cmd_peek(void)
{
	uchar i,size,area;
	uchar *p;

 	size = PacketFromPC.size;
	area = size & AREA_MASK;
	size = size & SIZE_MASK;
 	p = (uchar*)PacketFromPC.adrs;

	if(area & AREA_PGMEM) {
//		PacketFromPC.data[0]=0;	// TBLPTRUをゼロクリア.
		TBLPTR = (unsigned short long)PacketFromPC.adrs;

		for(i=0;i<size;i++) {
				_asm
				tblrdpostinc
				_endasm
			PacketToPC.raw[i]=TABLAT;
		}
	}else if(area & AREA_EEPROM) {
		unsigned char ee_adr = (unsigned char)(PacketFromPC.adrs & 0xff);

		for(i=0;i<size;i++) {
			PacketToPC.raw[i] = ReadEE(ee_adr++);
		}
	}else{
		for(i=0;i<size;i++) {
			PacketToPC.raw[i]=*p++;
		}
	}
	ToPcRdy = 1;
}
/********************************************************************
 *	メモリー書き込み
 ********************************************************************
 */
void cmd_poke(void)
{
	uchar size,area;
	uchar *p;
 	area = PacketFromPC.size;
 	p = (uchar*)PacketFromPC.adrs;

#if 1	/* Add by senshu */
	if(area & AREA_EEPROM) {
		unsigned char ee_adr = (unsigned char)(PacketFromPC.adrs & 0xff);
		WriteEE(ee_adr, Data0);
	} else {
		if( Mask1 ) {	//マスク書き込み.
			*p = (*p & Mask1) | Data0;
		}else{			//通常書き込み.
			*p = Data0;
		}
	}
#else
	if( Mask1 ) {	//マスク書き込み.
		*p = (*p & Mask1) | Data0;
	}else{			//通常書き込み.
		*p = Data0;
	}
#endif
}

void usbModuleDisable(void)
{
    UCON = 0;                               // Disable module & detach from bus
    UIE = 0;                                // Mask all USB interrupts
//	usb_device_state = DETACHED_STATE;      // Defined in usbmmap.c & .h
}

void wait_ms(uchar ms);

/********************************************************************
 *	番地指定の実行
 ********************************************************************
 */
void cmd_exec(void)
{
	uchar bootflag=PacketFromPC.raw[5];

	if(	bootflag ) {

		INTCONbits.GIEL = 0; 		// Low  Priority 割り込みを禁止.
		INTCONbits.GIEH = 0; 		// High Priority 割り込みを禁止.

		wait_ms(10);
		usbModuleDisable();		// USBを一旦切り離す.
		wait_ms(10);
	}


	PCLATU=0;
	PCLATH=PacketFromPC.raw[3];	// adrs_h
#if	0
	PCL   =PacketFromPC.raw[2];	// adrs_l
#else
	{
		uchar adrs_l=PacketFromPC.raw[2];
		_asm
		movf	adrs_l,1,1
		movwf	PCL,0
		_endasm
	}
#endif
}


/********************************************************************
 *	puts 文字列をホストに返送する.
 ********************************************************************
 */
void cmd_get_string(void)
{
	PacketToPC.raw[0] =  puts_mode;	//'p';		/* コマンド実行完了をHOSTに知らせる. */
	PacketToPC.raw[1] =  puts_ptr;	// 文字数.
	memcpy( (void*)&PacketToPC.raw[2] , (void*)puts_buf , puts_ptr & 0x3f);	//文字列.
	puts_ptr = 0;
	ToPcRdy = 1;
}
/********************************************************************
 *	ユーザー定義関数の実行.
 ********************************************************************
 */
void cmd_user_cmd(void)
{
	puts_ptr = 0;
	puts_mode = 'p';
	user_cmd(PacketFromPC.adrs);
	puts_mode = 'q';
}

/********************************************************************
 *	1mS単位の遅延を行う.
 ********************************************************************
 */
void cmd_wait_msec(void)
{
	uchar ms = PacketFromPC.size;
	if(ms) {
		wait_ms(ms);
	}
}

/********************************************************************
 *	データの連続送信実行.
 ********************************************************************
 */
void make_report(void)
{
#if	TIMER2_INT_SAMPLE			// タイマー２割り込みでPORTサンプル.
	uchar i;
	uchar cnt=0;
	int c;
	//サンプル値を最大60個まで返す実装.
	PacketToPC.raw[0] =  REPLY_REPORT;		/* コマンド実行完了をHOSTに知らせる. */
	for(i=0;i<60;i++) {
		c = mon_read_sample();
		if(c<0) break;
		PacketToPC.raw[2+i]=c;
		cnt++;
	}
	PacketToPC.raw[1] =  cnt;
#else
	//サンプル値を１個だけ返す実装.
	PacketToPC.raw[0] =  REPLY_REPORT;		/* コマンド実行完了をHOSTに知らせる. */

	if(	poll_mode == POLL_ANALOG) {
		PacketToPC.raw[1] = 2;
        while(ADCON0bits.NOT_DONE);     // Wait for conversion
		PacketToPC.raw[2] = ADRESL;
		PacketToPC.raw[3] = ADRESH;

	}else{
		PacketToPC.raw[1] = 1;
		PacketToPC.raw[2] = *poll_addr;
	}
#endif
	ToPcRdy = 1;
}
/********************************************************************
 *	データ引取りモードの設定
 ********************************************************************
 */
void cmd_set_mode(void)
{
	poll_mode = PacketFromPC.size;
	poll_addr = (uchar near *) PacketFromPC.adrs;

	if(	poll_mode == POLL_ANALOG) {
		mInitPOT();
		ADCON0bits.GO = 1;              // Start AD conversion
	}

	make_report();
}

/********************************************************************
 *	AVRライターの設定
 ********************************************************************
 */
void cmd_set_status(void)
{
/* ISP用のピンをHi-Z制御 */
/* ISP移行の手順を、ファーム側で持つ */
	if(PacketFromPC.raw[2] & 0x10) {// RST解除の場合
		ispDisconnect();
	}else{
		if(PacketFromPC.raw[2] & 0x80) {// RST状態でSCK Hは SCKパルス要求
			ispSckPulse();
		} else {
			ispConnect();
		}
	}
	PacketToPC.raw[0] = REPLY_REPORT;	/* コマンド実行完了をHOSTに知らせる. */
	ToPcRdy = 1;
}
/********************************************************************
 *	PORTへの出力制御
 ********************************************************************
 */
void cmd_tx(void)
{
	isp_command( &PacketFromPC.raw[1]);
	ToPcRdy = Cmd0 & 1;	// LSBがOnなら返答が必要.
}
/********************************************************************
 *	ページアドレスの設定.
 ********************************************************************
 */
void cmd_set_page(void)
{
	page_mode = PacketFromPC.raw[1];
	page_addr = PacketFromPC.raw[2];
	page_addr_h = PacketFromPC.raw[3];
}
/********************************************************************
 *	ISP書き込みクロックの設定.
 ********************************************************************
 */
void cmd_set_delay(void)
{
	usi_set_delay(PacketFromPC.raw[1]);	// '-dN'
}

/********************************************************************
 *	AVR書き込み(Fusion)コマンドの実行
 ********************************************************************
 *	Cmd0 : 0x20～0x27
 */
void cmd_page_tx(void)
{
	uchar i;
	//
	//	page_write開始時にpage_addrをdata[1]で初期化.
	//
	if( Cmd0 & (HIDASP_PAGE_TX_START & MODE_MASK)) {
		page_mode = 0x40;
		page_addr = 0;
		page_addr_h = 0;
	}
	//
	//	page_write (またはpage_read) の実行.
	//
	for(i=0;i<PacketFromPC.size;i++) {
		usi_trans(page_mode);
		usi_trans(page_addr_h);
		usi_trans(page_addr);
		PacketToPC.raw[i]=usi_trans(PacketFromPC.raw[i+2]);

		if (page_mode & 0x88) { // EEPROM or FlashH
			page_addr++;
			if(page_addr==0) {page_addr_h++;}
			page_mode&=~0x08;
		} else {
			page_mode|=0x08;
		}
	}
	//
	//	isp_command(Flash書き込み)の実行.
	//
	if( Cmd0 & (HIDASP_PAGE_TX_FLUSH & MODE_MASK)) {
		isp_command( &PacketFromPC.raw[PacketFromPC.size+2]);
	}
	ToPcRdy = Cmd0 & 1;	// LSBがOnなら返答が必要.
}

/********************************************************************
 *	AVRライター系コマンド受信と実行.
 ********************************************************************
 *	Cmd0 : 0x20～0x2F
 */
void cmd_avrspx(void)
{
	if(Cmd0 < (HIDASP_CMD_TX) ) 	{cmd_page_tx();}	// 0x20～0x27

	// 0x28～0x2F
	else if(Cmd0==HIDASP_SET_STATUS){cmd_set_status();}
	else if(Cmd0==HIDASP_SET_PAGE) 	{cmd_set_page();}
	else if(Cmd0==HIDASP_CMD_TX) 	{cmd_tx();}
	else if(Cmd0==HIDASP_CMD_TX_1) 	{cmd_tx();}
	else if(Cmd0==HIDASP_SET_DELAY) {cmd_set_delay();}
	else if(Cmd0==HIDASP_WAIT_MSEC) {cmd_wait_msec();}
}


/********************************************************************
 *	モニタコマンド受信と実行.
 ********************************************************************
 */
void ProcessIO(void)
{
	// 返答パケットが空であること、かつ、
	// 処理対象の受信データがある.
	if((ToPcRdy == 0) && (!mHIDRxIsBusy())) {

		//受信データがあれば、受信データを受け取る.
		HIDRxReport64((char *)&PacketFromPC);
		PacketToPC.raw[0]=Cmd0;		// CMD ECHOBACK

		//コマンドに対応する処理を呼び出す.
		if(Cmd0 >= HIDASP_PAGE_TX)  {cmd_avrspx();}	// AVRライターコマンド.
		else if(Cmd0 >= PICSPX_SETADRS24){cmd_picspx();}	// PICライターコマンド.
		else if(Cmd0==HIDASP_PEEK) 	{cmd_peek();}	// メモリー読み出し.
		else if(Cmd0==HIDASP_POKE) 	{cmd_poke();}	// メモリー書き込み.
		else if(Cmd0==HIDASP_JMP) 	{cmd_exec();}	// 実行.
		else if(Cmd0==HIDASP_SET_MODE)  {cmd_set_mode();}
		else if(Cmd0==HIDASP_GET_STRING){cmd_get_string();}
		else if(Cmd0==HIDASP_USER_CMD)  {cmd_user_cmd();}
		else if(Cmd0==HIDASP_TEST) 	{cmd_echo();}	// 接続テスト.
	}

	// 必要なら、返答パケットをインタラプト転送(EP1)でホストPCに返却する.
	if( ToPcRdy ) {
		if(!mHIDTxIsBusy()) {
			HIDTxReport64((char *)&PacketToPC);
			ToPcRdy = 0;

			if(poll_mode!=0) {
				if(mHIDRxIsBusy()) {	//コマンドが来ない限り送り続ける.
					make_report();
				}
			}
		}
	}
}
/********************************************************************
 *
 ********************************************************************
 */
