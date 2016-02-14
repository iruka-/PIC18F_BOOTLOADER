/********************************************************************
 *	簡易モニタ
 ********************************************************************
 */
#include <string.h>
//#include "usb/typedefs.h"

#include "USB/usb.h"
#include "HardwareProfile.h"
#include "USB/usb_function_generic.h"

#include "io_cfg.h"
#include "monit.h"
#include "hidcmd.h"
//#include "picwrt.h"
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
#define	DEVID	DEV_ID_PIC18F14K	//0x14
#else
#define	DEVID	DEV_ID_PIC			//0x25
#endif

#define	VER_H	1
#define	VER_L	1


//#define	POLL_ANALOG		0xa0
#define	REPLY_REPORT	0xaa

extern void memcpy64(char *t,char *s);
extern void user_cmd(int arg);

//-------------------------------------------------------------------
//
#if defined(__18F14K50)
#pragma udata usbram2
#else
#pragma udata
#endif
extern Packet PacketFromPC;			//入力パケット 64byte
extern unsigned char OUTPacket[64];
extern unsigned char INPacket[64];

//-------------------------------------------------------------------
//
#pragma udata
extern BOOL blinkStatusValid;
extern USB_HANDLE USBGenericOutHandle;
extern USB_HANDLE USBGenericInHandle;


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
//Packet PacketFromPC;			//入力パケット 64byte


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





#pragma code

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
//	puts_ptr = 0;
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
#if	0
	}else if(area & AREA_EEPROM) {
		unsigned char ee_adr = (unsigned char)(PacketFromPC.adrs & 0xff);

		for(i=0;i<size;i++) {
			PacketToPC.raw[i] = ReadEE(ee_adr++);
		}
#endif
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

	if( Mask1 ) {	//マスク書き込み.
		*p = (*p & Mask1) | Data0;
	}else{			//通常書き込み.
		*p = Data0;
	}
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
//	usi_set_delay(PacketFromPC.raw[1]);	// '-dN'
}

/********************************************************************
 *	モニタコマンド受信と実行.
 ********************************************************************
 */
void ProcessIO(void)
{
	// 返答パケットが空であること、かつ、
	// 処理対象の受信データがある.
	if((ToPcRdy == 0) && (USBHandleBusy(USBGenericOutHandle)==0) ) {
		//受信データがあれば、受信データを受け取る.
		memcpy64((char*)&PacketFromPC,(char*)OUTPacket);
		//次の読み込みを発行する.
		USBGenericOutHandle = USBGenRead(USBGEN_EP_NUM,(BYTE*)&OUTPacket
							  ,USBGEN_EP_SIZE);

		PacketToPC.raw[0]=Cmd0;		// CMD ECHOBACK
		//コマンドに対応する処理を呼び出す.
		     if(Cmd0==HIDASP_PEEK) 	{cmd_peek();}	// メモリー読み出し.
		else if(Cmd0==HIDASP_POKE) 	{cmd_poke();}	// メモリー書き込み.
		else if(Cmd0==HIDASP_JMP) 	{cmd_exec();}	// 実行.
		else if(Cmd0==HIDASP_TEST) 	{cmd_echo();}	// 接続テスト.
		else if(Cmd0==HIDASP_GET_STRING){cmd_get_string();}
		else if(Cmd0==HIDASP_USER_CMD)  {cmd_user_cmd();}
		else if(Cmd0==HIDASP_SET_MODE)  {cmd_set_mode();}
	}

	// 必要なら、返答パケットをバルク転送(EP1)でホストPCに返却する.
	if( ToPcRdy ) {
		if(!USBHandleBusy(USBGenericInHandle)) {
			memcpy64(INPacket,(char*)&PacketToPC);
			USBGenericInHandle=USBGenWrite(USBGEN_EP_NUM,(BYTE*)INPacket,USBGEN_EP_SIZE);	
			ToPcRdy = 0;

			if(poll_mode!=0) {
				if( USBHandleBusy(USBGenericOutHandle) ) {//コマンドが来ない限り送り続ける.
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
