/* hidasp.c
 * original: binzume.net
 * modify: senshu , iruka
 * 2008-09-22 : for HIDaspx.
 * 2008-11-07 : for HIDaspx, hidmon, bootmon88
 */
#include <windows.h>
#include <stdio.h>
#include "usbhid.h"
#include "hidasp.h"
#include "avrspx.h"

#include "../firmware/hidcmd.h"
#include "monit.h"

#ifndef __GNUC__
#pragma comment(lib, "setupapi.lib")
#endif


//----------------------------------------------------------------------------
//	Switches
//----------------------------------------------------------------------------

#define DEBUG 		   	0		// for DEBUG

#define DEBUG_PKTDUMP  	0		// HID Reportパケットをダンプする.
#define DUMP_PRODUCT   	0		// 製造者,製品名を表示.

#define CHECK_COUNT		4		// 4: Connect時の Ping test の回数.
#define BUFF_SIZE		256


#define	PAGE_WRITE_LENGTH	32

#define	ASYNC_RW		1
#define	ASYNC_DEBUG		1

#define	TIMEOUT_MS 		4000

OVERLAPPED g_ovl;                // オーバーラップ構造体
HANDLE     hEvent;

//----------------------------------------------------------------------------
//	Defines
//----------------------------------------------------------------------------
static int MY_VID = 0x04D8;	// MicroChip
static int MY_PID = 0x003C;	// HIDbootloader

//#define	MY_Manufacturer	"YCIT"
#define	MY_Manufacturer	"AVRetc"
#define	MY_Product		"PIC18spx"

//	MY_Manufacturer,MY_Product のdefine を外すと、VID,PIDのみの照合になる.
//	どちらかをはずすと、その照合を省略するようになる.
static int found_hidaspx;
extern int hidmon_mode;

// HID API (from w2k DDK)
_HidD_GetAttributes 		HidD_GetAttributes;
_HidD_GetHidGuid 			HidD_GetHidGuid;
_HidD_GetPreparsedData 		HidD_GetPreparsedData;
_HidD_FreePreparsedData 	HidD_FreePreparsedData;
_HidP_GetCaps 				HidP_GetCaps;
_HidP_GetValueCaps 			HidP_GetValueCaps;
_HidD_GetFeature 			HidD_GetFeature;
_HidD_SetFeature 			HidD_SetFeature;
_HidD_GetManufacturerString	HidD_GetManufacturerString;
_HidD_GetProductString 		HidD_GetProductString;
_HidD_GetSerialNumberString HidD_GetSerialNumberString;	// add by senshu

HINSTANCE hHID_DLL = NULL;		// hid.dll handle
HANDLE hHID = NULL;				// USB-IO dev handle
HIDP_CAPS Caps;

static	int dev_id       = 0;	// ターゲットID: 0x25 もしくは 0x14 だけを許容.
static	int dev_version  = 0;	// ターゲットバージョン hh.ll
static	int dev_bootloader= 0;	// ターゲットにbootloader機能がある.

static	int have_isp_cmd = 0;	// ISP制御の有無.

/* Prototypes */
static char *uni_to_string (char *t, unsigned short *u);
static int LoadHidDLL (void);
static int OpenTheHid (const char *serial, int list_mode);
static int check_product_string (HANDLE handle, const char *serial, int list_mode);
static int hidRead (HANDLE h, char *buf, int Length, int id);
static int hidRead (HANDLE h, char *buf, int Length, int id);
static int hidWrite (HANDLE h, char *buf, int Length, int id);
static int hidWrite (HANDLE h, char *buf, int Length, int id);
static void GetDevCaps (void);
static void hidSetStatus (int ledstat);
static void hid_transmit (BYTE cmd1, BYTE cmd2, BYTE cmd3, BYTE cmd4);

#if HIDMON88
static void hidSetDevCaps (void);
#endif


//----------------------------------------------------------------------------
//--------------------------    Tables    ------------------------------------
//----------------------------------------------------------------------------
//  HID Report のパケットはサイズ毎に 3種類用意されている.
	#define	REPORT_ID1			0
	#define	REPORT_LENGTH1		65
	//	最大の長さをもつ HID ReportID,Length
//	#define	REPORT_IDMAX		0
//	#define	REPORT_LENGTHMAX	65


#if	DEBUG_PKTDUMP || ASYNC_DEBUG
static	void memdump(char *msg, char *buf, int len);
#endif


#if	1
/*
 *	HIDデバイスに HID Report を送信する.
 *	送信バッファの先頭の1バイトにReportID を入れる処理は
 *	この関数内で行うので、先頭1バイトを予約しておくこと.
 *
 *	id と Length の組はデバイス側で定義されたものでなければならない.
 *
 *	戻り値はHidD_SetFeatureの戻り値( 0 = 失敗 )
 *
 */
static int hidWrite(HANDLE h, char *buf, int Length, int id)
{
	int rc,err;
	DWORD sz;
	buf[0] = 0;		// ReportIDは常に0

#if	ASYNC_RW

	// イベントオブジェクトを作成する
	hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	if( hEvent == NULL ) {
		return -1;	// エラー
	}
	// オーバーラップ構造体の初期化
	ZeroMemory( &g_ovl, sizeof(g_ovl) );
	g_ovl.Offset = 0;
	g_ovl.OffsetHigh = 0;
	g_ovl.hEvent = hEvent;

	rc = WriteFile(h, buf , Length , &sz, &g_ovl);
	if(rc==0) {	// Pending?
		if( (err=GetLastError()) != ERROR_IO_PENDING ) {
#if ASYNC_DEBUG
			memdump("WR", buf, Length);
			fprintf(stderr, "hidWrite() error %x\n" ,err);
#endif
			return -1;		//ERROR
		}
		if( WaitForSingleObject( hEvent, TIMEOUT_MS ) == WAIT_OBJECT_0 ) {
			rc = Length;	//一応成功.
		}else{
#if ASYNC_DEBUG
			memdump("WR", buf, Length);
			fprintf(stderr, "hidWrite() timeout\n" );
#endif
			return -1;		//タイムアウト,破棄,もしくは失敗.
		}
	}

#else
	rc = WriteFile(h, buf , Length , &sz, NULL);
#endif

#if	DEBUG_PKTDUMP
	memdump("WR", buf, Length);
#endif
	return rc;
}


static int hidRead(HANDLE h, char *buf, int Length, int id)
{
	int rc,err;
	DWORD sz;

#if	ASYNC_RW

	// イベントオブジェクトを作成する
	hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	if( hEvent == NULL ) {
		return -1;	// エラー
	}
	// オーバーラップ構造体の初期化
	ZeroMemory( &g_ovl, sizeof(g_ovl) );
	g_ovl.Offset = 0;
	g_ovl.OffsetHigh = 0;
	g_ovl.hEvent = hEvent;

	rc = ReadFile(h, buf, Length,&sz,&g_ovl);
	if(rc==0) {	// Pending?
		if( (err=GetLastError()) != ERROR_IO_PENDING ) {
#if ASYNC_DEBUG
			fprintf(stderr, "hidRead() error %x\n" ,err);
#endif
			return -1;		//ERROR
		}
		if( WaitForSingleObject( hEvent, TIMEOUT_MS  ) == WAIT_OBJECT_0 ) {
			rc = Length;	//一応成功.
		}else{
#if ASYNC_DEBUG
			fprintf(stderr, "hidRead() timeout\n" );
#endif
			return -1;		//タイムアウト,破棄,もしくは失敗.
		}
	}

#else
	rc = ReadFile(h, buf, Length,&sz,NULL);
#endif

#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
	printf("id=%d Length=%d rc=%d\n",buf[0],Length,rc);
#endif
	return rc;
}

int	hidReadPoll(char *buf, int Length, int id)
{
	return hidRead(hHID, buf, Length, id);
}

#else
/*
 * wrapper for HidD_GetFeature / HidD_SetFeature.
 */
//----------------------------------------------------------------------------
/*
 *	HIDデバイスから HID Report を取得する.
 *	受け取ったバッファは先頭の1バイトに必ずReportIDが入っている.
 *
 *	id と Length の組はデバイス側で定義されたものでなければならない.
 *
 *	戻り値はHidD_GetFeatureの戻り値( 0 = 失敗 )
 *
 */
static int hidRead(HANDLE h, char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = HidD_GetFeature(h, buf, Length);
#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
	printf("id=%d Length=%d rc=%d\n",id,Length,rc);
#endif
	return rc;
}

int	hidReadPoll(char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = HidD_GetFeature(hHID, buf, Length);
#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
#endif
	return rc;
}


/*
 *	HIDデバイスに HID Report を送信する.
 *	送信バッファの先頭の1バイトにReportID を入れる処理は
 *	この関数内で行うので、先頭1バイトを予約しておくこと.
 *
 *	id と Length の組はデバイス側で定義されたものでなければならない.
 *
 *	戻り値はHidD_SetFeatureの戻り値( 0 = 失敗 )
 *
 */
static int hidWrite(HANDLE h, char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = HidD_SetFeature(h, buf, Length);
#if	DEBUG_PKTDUMP
	memdump("WR", buf, Length);
#endif
	return rc;
}

#endif

/*
 *	hidWrite()を使用して、デバイス側に buf[] データを len バイト送る.
 *	長さを自動判別して、ReportIDも自動選択する.
 *
 *	戻り値はHidD_SetFeatureの戻り値( 0 = 失敗 )
 *
 */
int	hidWriteBuffer(char *buf, int len)
{
	len = 65;
	return hidWrite(hHID, buf, len , 0);
}

/*
 *	hidRead()を使用して、デバイス側から buf[] データを len バイト取得する.
 *	長さを自動判別して、ReportIDも自動選択する.
 *
 *	戻り値はHidD_GetFeatureの戻り値( 0 = 失敗 )
 *
 */
int	hidReadBuffer(char *buf, int len , int id)
{
	len = 65;
	return hidRead(hHID, buf, len, 0);
}
/*
 *	hidWrite()を使用して、デバイス側に4バイトの情報を送る.
 *	4バイトの内訳は cmd , arg1 , arg2 , arg 3 である.
 *  ReportIDはID1を使用する.
 *
 *	戻り値はHidD_SetFeatureの戻り値( 0 = 失敗 )
 *
 */
int hidCommand(int cmd,int arg1,int arg2,int arg3)
{
	unsigned char buf[BUFF_SIZE];

	memset(buf , 0, sizeof(buf) );

	buf[1] = cmd;
	buf[2] = arg1;
	buf[3] = arg2;
	buf[4] = arg3;

	return hidWrite(hHID, buf, REPORT_LENGTH1, REPORT_ID1);
}

//
//	mask が   0 の場合は、 addr に data0 を1バイト書き込み.
//	mask が 非0 の場合は、 addr に data0 と mask の論理積を書き込む.
//		但し、その場合は mask bitが 0 になっている部分に影響を与えないようにする.
//
//	例:	PORTB の 8bit に dataを書き込む.
//		hidPokeMem( PORTB , data , 0 );
//	例:	PORTB の bit2 だけを on
//		hidPokeMem( PORTB , 1<<2 , 1<<2 );
//	例:	PORTB の bit2 だけを off
//		hidPokeMem( PORTB ,    0 , 1<<2 );
//
int hidPokeMem(int addr,int data0,int mask)
{
	unsigned char buf[BUFF_SIZE];
	memset(buf , 0, sizeof(buf) );

	buf[1] = HIDASP_POKE;
	buf[2] = 0;
	buf[3] = addr;
	buf[4] =(addr>>8);
	if( mask ) {
		buf[5] = data0 & mask;
		buf[6] = ~mask;
	}else{
		buf[5] = data0;
		buf[6] = 0;
	}
	return hidWrite(hHID, buf, REPORT_LENGTH1, REPORT_ID1);
}

int hidPeekMem(int addr)
{
	unsigned char buf[BUFF_SIZE];
	memset(buf , 0, sizeof(buf) );

	buf[1] = HIDASP_PEEK;
	buf[2] = 1;
	buf[3] = addr;
	buf[4] =(addr>>8);

	hidWrite(hHID, buf, REPORT_LENGTH1, REPORT_ID1);
	hidReadBuffer( buf, REPORT_LENGTH1,0 );
	return buf[1];
}

#if HIDMON88
static void hidSetDevCaps(void)
{
	hidCommand(HIDASP_GET_CAPS,0,0,0);	// bootloadHIDが取得すべき情報をバッファに置く.
}
#endif

#define	USICR			0x2d	//
#define	DDRB			0x37	// PB4=RST PB3=LED
#define	DDRB_WR_MASK	0xf0	// 制御可能bit = 1111_0000
#define	PORTB			0x38	// PB4=RST PB3=LED
#define	PORTB_WR_MASK	0		// 0 の場合はMASK演算は省略され、直書き.

#define HIDASP_RST		0x10	// RST bit

/*
 *	LEDの制御. (hidasp.h)
#define HIDASP_RST_H_GREEN	0x18	// RST解除,LED OFF
#define HIDASP_RST_L_BOTH	0x00	// RST実行,LED ON
#define HIDASP_SCK_PULSE 	0x80	// RST-L SLK-pulse ,LED ON	@@kuga
 */


static void hidSetStatus(int ledstat)
{
	unsigned char rd_data[BUFF_SIZE];
	static int once = 0;
	int ddrb;
	if (have_isp_cmd) {
		hidCommand(HIDASP_SET_STATUS,0,ledstat,0);	// cmd,portd(&0000_0011),portb(&0001_1111),0
		// ↓PIC18Fファームの場合、ハンドシェークパケットは必ず受け取る必要がある.
		hidRead(hHID, rd_data ,REPORT_LENGTH1, REPORT_ID1);
		//   そうでないと、次のコマンドの応答パケットに、このゴミが送信され、以後１つずつずれる.
	}else{
		if (once == 0 && !hidmon_mode) {
//			fprintf(stderr, "Warnning: Please update HIDaspx firmware.\n");
			fprintf(stderr, "Warnning: Please check HIDaspx mode.\n");
			once++;
		}

		if (hidmon_mode) {
			ddrb = 0xff;			// DDRB = 0xff : 1が出力ピン ただしbit3-0は影響しない.
			hidPokeMem(PORTB,ledstat,PORTB_WR_MASK);
			hidPokeMem(DDRB ,ddrb   ,DDRB_WR_MASK);
		} else if(ledstat & HIDASP_RST) {	// RST解除.
			ddrb = 0x10;			// PORTB 全入力.
			hidPokeMem(USICR,0      ,0);
			hidPokeMem(PORTB,ledstat,PORTB_WR_MASK);
			hidPokeMem(DDRB ,ddrb   ,DDRB_WR_MASK);
		}else{
			// RSTをLにする.
			ddrb = 0xd0;			// DDRB 1101_1100 : 1が出力ピン ただしbit3-0は影響しない.
			hidPokeMem(USICR,0      ,0);
			hidPokeMem(DDRB ,ddrb   ,DDRB_WR_MASK);
			hidPokeMem(PORTB,ledstat,PORTB_WR_MASK);
			hidPokeMem(PORTB,ledstat|HIDASP_RST,PORTB_WR_MASK);	// RSTはまだHi
			hidPokeMem(PORTB,ledstat,PORTB_WR_MASK);	// RSTはあとでLに.
			hidPokeMem(USICR,0x1a   ,0);
		}
	}
}

////////////////////////////////////////////////////////////////////////
//             hid.dll をロード
static int LoadHidDLL()
{
	hHID_DLL = LoadLibrary("hid.dll");
	if (!hHID_DLL) {
#if 1
		fprintf(stderr, "Error at Load 'hid.dll'\n");
#else
		MessageBox(NULL, "Error at Load 'hid.dll'", "ERR", MB_OK);
#endif
		return 0;
	}
	HidD_GetAttributes = (_HidD_GetAttributes)GetProcAddress(hHID_DLL, "HidD_GetAttributes");
	if (!HidD_GetAttributes) {
#if 1
		fprintf(stderr, "Error at HidD_GetAttributes\n");
#else
		MessageBox(NULL, "Error at HidD_GetAttributes", "ERR", MB_OK);
#endif
		return 0;
	}
	HidD_GetHidGuid = (_HidD_GetHidGuid)GetProcAddress(hHID_DLL, "HidD_GetHidGuid");
	if (!HidD_GetHidGuid) {
#if 1
		fprintf(stderr, "Error at HidD_GetHidGuid\n");
#else
		MessageBox(NULL, "Error at HidD_GetHidGuid", "ERR", MB_OK);
#endif
		return 0;
	}
	HidD_GetPreparsedData =	(_HidD_GetPreparsedData)GetProcAddress(hHID_DLL, "HidD_GetPreparsedData");
	HidD_FreePreparsedData = (_HidD_FreePreparsedData)GetProcAddress(hHID_DLL, "HidD_FreePreparsedData");
	HidP_GetCaps = (_HidP_GetCaps)GetProcAddress(hHID_DLL, "HidP_GetCaps");
	HidP_GetValueCaps =	(_HidP_GetValueCaps) GetProcAddress(hHID_DLL, "HidP_GetValueCaps");

//
	HidD_GetFeature = (_HidD_GetFeature)GetProcAddress(hHID_DLL, "HidD_GetFeature");
	HidD_SetFeature = (_HidD_SetFeature)GetProcAddress(hHID_DLL, "HidD_SetFeature");
	HidD_GetManufacturerString = (_HidD_GetManufacturerString)GetProcAddress(hHID_DLL, "HidD_GetManufacturerString");
	HidD_GetProductString = (_HidD_GetProductString)GetProcAddress(hHID_DLL, "HidD_GetProductString");
	HidD_GetSerialNumberString = (_HidD_GetSerialNumberString)GetProcAddress(hHID_DLL, "HidD_GetSerialNumberString");

#if	DEBUG
	printf("_HidD_GetFeature= %x\n", (int) HidD_GetFeature);
	printf("_HidD_SetFeature= %x\n", (int) HidD_SetFeature);
#endif
	return 1;
}

////////////////////////////////////////////////////////////////////////
// ディバイスの情報を取得
static void GetDevCaps()
{
	PHIDP_PREPARSED_DATA PreparsedData;
	HIDP_VALUE_CAPS *VCaps;
	char buf[1024];

	VCaps = (HIDP_VALUE_CAPS *) (&buf);

	HidD_GetPreparsedData(hHID, &PreparsedData);
	HidP_GetCaps(PreparsedData, &Caps);
	HidP_GetValueCaps(HidP_Input, VCaps, &Caps.NumberInputValueCaps,  PreparsedData);
	HidD_FreePreparsedData(PreparsedData);

#if DEBUG
	fprintf(stderr, " Caps.OutputReportByteLength = %d\n", Caps.OutputReportByteLength);
	fprintf(stderr, " Caps.InputReportByteLength = %d\n", Caps.InputReportByteLength);
#endif


#if	REPORT_LENGTH_OVERRIDE
	//複数のREPORT_COUNTが存在するときは下記Capsが 0 のままなので、上書きする.
	Caps.OutputReportByteLength = REPORT_LENGTH_OVERRIDE;
	Caps.InputReportByteLength = REPORT_LENGTH_OVERRIDE;
#endif
}

//----------------------------------------------------------------------------
/*
 * 	unicode を ASCIIに変換.
 */
static char *uni_to_string(char *t, unsigned short *u)
{
	char *buf = t;
	int c;
	// short を char 配列に入れなおす.
	while (1) {
		c = *u++;
		if (c == 0)
			break;
		if (c & 0xff00)
			c = '?';
		*t++ = c;
	}

	*t = 0;
	return buf;
}

//----------------------------------------------------------------------------
/*  Manufacturer & Product name check.
 *  名前チェック : 成功=1  失敗=0 読み取り不能=(-1)
 */
static int check_product_string(HANDLE handle, const char *serial, int list_mode)
{
	static int first = 1;
	int i;
	unsigned short unicode[BUFF_SIZE*2];
	char string1[BUFF_SIZE];
	char string2[BUFF_SIZE];
	char string3[BUFF_SIZE];
	char tmp[2][BUFF_SIZE];

#if	DEBUG
	list_mode = 1;
#endif

	Sleep(20);
	if (!HidD_GetManufacturerString(handle, unicode, sizeof(unicode))) {
		return -1;
	}
	uni_to_string(string1, unicode);

	Sleep(20);
	if (!HidD_GetProductString(handle, unicode, sizeof(unicode))) {
		return -1;
	}
	uni_to_string(string2, unicode);

	// シリアル番号のチェックを厳密化 (2010/02/12 13:24:08)
	if (serial[0]=='*') {
#define f_auto_retry 3		/* for HIDaspx (Auto detect, Retry MAX) */

		for (i=0; i<f_auto_retry; i++) {
			Sleep(20);
			if (!HidD_GetSerialNumberString(handle, unicode, sizeof(unicode))) {
				return -1;
			}
			uni_to_string(tmp[i%2], unicode);
			if ((i>0) && ((i%2) == 1)) {
				if (strcmp(tmp[0], tmp[1])==0) {
					strcpy(string3, tmp[0]);	// OK
				} else {
					return -1;
				}
			}
		}
		if (list_mode) {
			if (first) {
				fprintf(stderr,
				"VID=%04x, PID=%04x, [%s], [%s], serial=[%s] (*)\n", MY_VID, MY_PID, string1,  string2, string3);
				first = 0;
			} else {
				fprintf(stderr,
				"VID=%04x, PID=%04x, [%s], [%s], serial=[%s]\n", MY_VID, MY_PID, string1,  string2, string3);
			}
		}
	} else {
		Sleep(20);
		if (!HidD_GetSerialNumberString(handle, unicode, sizeof(unicode))) {
			return -1;
		}
		uni_to_string(string3, unicode);
	}


#ifdef	MY_Manufacturer
	if (strcmp(string1, MY_Manufacturer) != 0)
		return 0;	// 一致せず
#endif

#ifdef	MY_Product
	if (strcmp(string2, MY_Product) != 0)
		return 0;	// 一致せず
#endif

	found_hidaspx++;

	/* Check serial number */
	if (found_hidaspx) {
		if (strcmp(string3, serial) == 0)
			return 1;		//合致した.
		else if (strcmp(serial ,"*") == 0)
			return 1;		//合致した.
	}

	return 0;	// 一致せず

}

////////////////////////////////////////////////////////////////////////
// HIDディバイス一覧からUSB-IOを検索
static int OpenTheHid(const char *serial, int list_mode)
{
	int f, i, rc;
	ULONG Needed, l;
	GUID HidGuid;
	HDEVINFO DeviceInfoSet;
	HIDD_ATTRIBUTES DeviceAttributes;
	SP_DEVICE_INTERFACE_DATA DevData;
	PSP_INTERFACE_DEVICE_DETAIL_DATA DevDetail;
	//SP_DEVICE_INTERFACE_DETAIL_DATA *MyDeviceInterfaceDetailData;

	DeviceAttributes.Size = sizeof(HIDD_ATTRIBUTES);
	DevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	HidD_GetHidGuid(&HidGuid);
#if 1							/* For vista */
	DeviceInfoSet =
		SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
#else
	DeviceInfoSet =
		SetupDiGetClassDevs(&HidGuid, "", NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
#endif

	f = i = 0;
	while ((rc = SetupDiEnumDeviceInterfaces(DeviceInfoSet, 0, &HidGuid, i++, &DevData))!=0) {
		SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DevData, NULL, 0, &Needed, 0);
		l = Needed;
		DevDetail = (SP_DEVICE_INTERFACE_DETAIL_DATA *) GlobalAlloc(GPTR, l + 4);
		DevDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
		SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DevData, DevDetail, l, &Needed, 0);

		hHID = CreateFile(DevDetail->DevicePath,
						  GENERIC_READ | GENERIC_WRITE,
						  FILE_SHARE_READ | FILE_SHARE_WRITE,
						  NULL, OPEN_EXISTING,
#if	ASYNC_RW
						  FILE_FLAG_OVERLAPPED,
#else
						  0,
#endif
						  NULL);

		GlobalFree(DevDetail);

		if (hHID == INVALID_HANDLE_VALUE)	// Can't open a device
			continue;
		HidD_GetAttributes(hHID, &DeviceAttributes);

		// HIDaspかどうか調べる.
		if (DeviceAttributes.VendorID == MY_VID && DeviceAttributes.ProductID == MY_PID) {
			int rc;
			rc = check_product_string(hHID, serial, list_mode);
			if ( rc == 1) {
				f++;				// 見つかった
				// 複数個をリストする (2010/02/12 13:19:38)
				if (list_mode == 0) {
					break;			// 自動認識時は、最初の候補を利用する
				}
			}
		} else {
			// 違ったら閉じる
			CloseHandle(hHID);
			hHID = NULL;
		}
	}
	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	return f;
}

#if	DEBUG_PKTDUMP || ASYNC_DEBUG
//----------------------------------------------------------------------------
//  メモリーダンプ.
void memdump(char *msg, char *buf, int len)
{
	int j;
	fprintf(stderr, "%s", msg);
	for (j = 0; j < len; j++) {
		fprintf(stderr, " %02x", buf[j] & 0xff);
		if((j & 0x1f)== 31)
			fprintf(stderr, "\n +");
	}
	fprintf(stderr, "\n");
}
#endif


int hidasp_list(char * string)
{
	int r, rc;

	LoadHidDLL();
	found_hidaspx = 0;
	r = OpenTheHid("*", 1);
	if (r == 0) {
		rc = 1;
	} else {
		rc = 0;
	}
	if (found_hidaspx==0) {
		fprintf(stderr, "%s: HIDaspx(VID=%04x, PID=%04x) isn't found.\n", string, MY_VID, MY_PID);
	}
	if (hHID_DLL) {
		FreeLibrary(hHID_DLL);
	}
	return rc;
}

//----------------------------------------------------------------------------
//	初期化
//----------------------------------------------------------------------------
int hidasp_init(int type, const char *serial)
{
	unsigned char rd_data[BUFF_SIZE];
	int i, r, result;

	result = 0;

	LoadHidDLL();
	if (OpenTheHid(serial, 0) == 0) {
#if DEBUG
		fprintf(stderr, "ERROR: fail to OpenTheHid(%s)\n", serial);
#endif
		return HIDASP_MODE_ERROR;
	}

	GetDevCaps();

#if DEBUG
	fprintf(stderr, "HIDaspx Connection check!\n");
#endif

	for (i = 0; i < CHECK_COUNT; i++) {
		hidCommand(HIDASP_TEST,(i),0,0);	// Connection test
		r = hidRead(hHID, rd_data ,REPORT_LENGTH1, REPORT_ID1);

#if DEBUG
		fprintf(stderr, "HIDasp Ping(%2d) = %d\n", i, rd_data[4]);
#endif
		if (r == 0) {
			fprintf(stderr, "Error: fail to Read().\n");
			return HIDASP_MODE_ERROR;
		}

		dev_id         = rd_data[2];
		dev_version    = rd_data[3] | (rd_data[4]<<8) ;
		dev_bootloader = rd_data[5];

		if((dev_id != DEV_ID_PIC)
		 &&(dev_id != DEV_ID_PIC18F14K)) {
			fprintf(stderr, "Error: fail to ping test. (id = %x)\n",dev_id);
			return HIDASP_MODE_ERROR;
		}

		if (hidmon_mode) {
//			fprintf(stderr, "TARGET DEV_ID=%x\n",dev_id);
		}

		if (rd_data[6] != i) {
			fprintf(stderr, "Error: fail to ping test. %d != %d\n", rd_data[6] , i);
			return HIDASP_MODE_ERROR;
		}
	}

	{	char *sBoot="";
		if(dev_bootloader==1) {sBoot="(Bootloader)";}
		fprintf(stderr, "TARGET DEV_ID=%x VER=%d.%d%s\n",dev_id
				,dev_version>>8,dev_version & 0xff,sBoot);
	}

#if	1
	if (!hidmon_mode) {
		hidCommand(HIDASP_SET_STATUS,0,HIDASP_RST_H_GREEN,0);	// RESET HIGH
		// ↓PIC18Fファームの場合、ハンドシェークパケットは必ず受け取る必要がある.
	r = hidRead(hHID, rd_data ,REPORT_LENGTH1, REPORT_ID1);
	if( rd_data[1] == 0xaa ) {				// ISPコマンド(isp_enable)が正常動作した.
		have_isp_cmd = HIDASP_ISP_MODE;		// ISP制御OK.
		result |= HIDASP_ISP_MODE;
	} else if( rd_data[1] == DEV_ID_FUSION ) {
		result |= HIDASP_FUSION_OK;			// NEW firmware
	} else if( rd_data[1] == DEV_ID_STD ) {
		result &= ~HIDASP_FUSION_OK;		// OLD firmware
	} else if( rd_data[1] == DEV_ID_MEGA88 ) {		// USB-IO mode
		result |= HIDASP_USB_IO_MODE;		// ISP制御NG.
	} else if( rd_data[1] == DEV_ID_MEGA88_USERMODE ) {		// USB-IO mode
		result |= HIDASP_USB_IO_MODE;		// ISP制御NG.
	}

#if DEBUG
	if (result & HIDASP_ISP_MODE) {
		fprintf(stderr, "[ISP CMD] ");
	}
	if (result & HIDASP_FUSION_OK) {
		fprintf(stderr, "[FUSION] ");
	}
	if (result & HIDASP_USB_IO_MODE) {
		fprintf(stderr, "[USB-IO mode] ");
	}
	if (result == 0) {
		fprintf(stderr, "ISP CMD Not support.\n");
	} else {
		fprintf(stderr, "OK.\n");
	}
#endif
#endif
	}

	return result;
}

//----------------------------------------------------------------------------
//  終了.
//----------------------------------------------------------------------------
void hidasp_close()
{
#define HIDASP_NOP 			  0	//これは ISPのコマンドと思われる?

	if (hHID) {
		if (!hidmon_mode) {
		/* これを実行すると、PORTB7(SCK) が一瞬 ONになる */
			unsigned char buf[BUFF_SIZE];

			buf[0] = 0x00;
			buf[1] = HIDASP_NOP;
			buf[2] = 0x00;
			buf[3] = 0x00;
			hidasp_cmd(buf, NULL);					// AVOID BUG!

			hidSetStatus(HIDASP_RST_H_GREEN);		// RESET HIGH
		}
#if	HIDMON88
		hidSetDevCaps();
#endif
		CloseHandle(hHID);
	}
	if (hHID_DLL) {
		FreeLibrary(hHID_DLL);
	}
	hHID = NULL;
	hHID_DLL = NULL;
}

//----------------------------------------------------------------------------
//  ISPコマンド発行.
//----------------------------------------------------------------------------
int hidasp_cmd(const unsigned char cmd[4], unsigned char res[4])
{
	char buf[128];
	int r;

	memset(buf , 0, sizeof(buf) );
	if (res != NULL) {
		buf[1] = HIDASP_CMD_TX_1;
	} else {
		buf[1] = HIDASP_CMD_TX;
	}
	memcpy(buf + 2, cmd, 4);

	r = hidWrite(hHID, buf, REPORT_LENGTH1 , REPORT_ID1);
#if DEBUG
	fprintf(stderr, "hidasp_cmd %02X, cmd: %02X %02X %02X %02X ",
		buf[1], cmd[0], cmd[1], cmd[2], cmd[3]);
#endif

	if (res != NULL) {
		r = hidRead(hHID, buf, REPORT_LENGTH1 , REPORT_ID1);
		memcpy(res, buf + 1, 4);
#if DEBUG
		fprintf(stderr, " --> res: %02X %02X %02X %02X\n",
				res[0], res[1], res[2], res[3]);
#endif
	}

	return 1;
}


#if (HIDMON || HIDASPX)
//----------------------------------------------------------------------------
//  ターゲットマイコンをプログラムモードに移行する.
//----------------------------------------------------------------------------
int hidasp_program_enable(int delay)
{
	unsigned char buf[BUFF_SIZE];
	unsigned char res[4];
	int i, rc;
	int tried;	//AVRSP と同様のプロトコルを採用

	rc = 1;
	hidSetStatus(HIDASP_RST_H_GREEN);			// RESET HIGH
	hidSetStatus(HIDASP_RST_L_BOTH);			// RESET LOW (H PULSE)
	hidCommand(HIDASP_SET_DELAY,delay,0,0);		// SET_DELAY
	Sleep(30);				// 30

	for(tried = 0; tried < 3; tried++) {
		for(i = 0; i < 32; i++) {

			buf[0] = 0xAC;
			buf[1] = 0x53;
			buf[2] = 0x00;
			buf[3] = 0x00;
			hidasp_cmd(buf, res);

			if (res[2] == 0x53) {
				rc = 0;					// AVRマイコンと同期を確認
				goto hidasp_program_enable_exit;
			}
			if (tried < 2) {			// 2回までは通常の同期方法
				break;
			}
			// AT90S用の同期方法で同期を取る
			hidSetStatus(HIDASP_SCK_PULSE);			// RESET LOW SCK H PULSE shift scan point
		}
	}

	hidasp_program_enable_exit:
#if DEBUG
	if (rc == 0) {
		fprintf(stderr, "hidasp_program_enable() == OK\n");
	} else  {
		fprintf(stderr, "hidasp_program_enable() == NG\n");
	}
#endif
	return rc;
}




static	void hid_transmit(BYTE cmd1, BYTE cmd2, BYTE cmd3, BYTE cmd4)
{
	unsigned char cmd[4];

	cmd[0] = cmd1;
	cmd[1] = cmd2;
	cmd[2] = cmd3;
	cmd[3] = cmd4;
	hidasp_cmd(cmd, NULL);
}

//----------------------------------------------------------------------------
//  フュージョンサポートのページライト.
//----------------------------------------------------------------------------
int hidasp_page_write_fast(long addr, const unsigned char *wd, int pagesize)
{
	int n, l;
	char buf[BUFF_SIZE];
	int cmd = HIDASP_PAGE_TX_START;

	memset(buf , 0, sizeof(buf) );

	// Load the page data into page buffer
	n = 0;	// n はPageBufferへの書き込み基点offset.
	while (n < pagesize) {
		l = PAGE_WRITE_LENGTH;		// 32バイトが最大データ長.
		if (pagesize - n < l) {		// 残量が32バイト未満のときは len を残量に置き換える.
			l = pagesize - n;
		}
		buf[1] = cmd;				// HIDASP_PAGE_TX_* コマンド.
		buf[2] = l;					// l       書き込みデータ列の長さ.
		memcpy(buf + 3, wd + n, l);	// data[l] 書き込みデータ列.

		if((pagesize - n) == l) {
			buf[1] |= HIDASP_PAGE_TX_FLUSH;		//最終page_writeではisp_commandを付加する.
			// ISP コマンド列.
			buf[3 + l + 0] = C_WR_PAGE;
			buf[3 + l + 1] = (BYTE)(addr >> 9);
			buf[3 + l + 2] = (BYTE)(addr >> 1);
			buf[3 + l + 3] = 0;
		}
		hidWriteBuffer(buf, REPORT_LENGTHMAX);

		n += l;
		cmd = HIDASP_PAGE_TX;		// cmd をノーマルのpage_writeに戻す.
	}


	return 0;
}

//----------------------------------------------------------------------------
//  ページライト.
//----------------------------------------------------------------------------
int hidasp_page_write(long addr, const unsigned char *wd, int pagesize,int flashsize)
{
	int n, l;
	char buf[BUFF_SIZE];

//	if(	(dev_id == DEV_ID_FUSION) && (flashsize <= (128*1024)) ) {
	if(	(flashsize <= (128*1024)) ) {
//		((addr & 0xFF) == 0 	) ) {
		// addres_set , page_write , isp_command が融合された高速版を実行.
		return hidasp_page_write_fast(addr,wd,pagesize);
	}
#if	0
	fprintf(stderr,"dev_id=%x flashsize=%x addr=%x\n",
		dev_id,flashsize,(int)addr
	);
#endif
	// set page
	hidCommand(HIDASP_SET_PAGE,0x40,0,(addr & 0xFF));	// Set Page mode , FlashWrite

	// Load the page data into page buffer
	n = 0;	// n はPageBufferへの書き込み基点offset.
	while (n < pagesize) {
		l = PAGE_WRITE_LENGTH;	// MAX
		if (pagesize - n < l) {
			l = pagesize - n;
		}
		buf[0] = 0x00;
		buf[1] = HIDASP_PAGE_TX;	// PageBuf
		buf[2] = l;				// Len
		memcpy(buf + 3, wd + n, l);
		hidWriteBuffer(buf,  3 + l);

#if DEBUG
		fprintf(stderr, "  p: %02x %02x %02x %02x\n",
				buf[1] & 0xff, buf[2] & 0xff, buf[3] & 0xff, buf[4] & 0xff);
#endif
		n += l;
	}

	/* Load extended address if needed */
	if(flashsize > (128*1024)) {
		hid_transmit(C_LD_ADRX,0,(BYTE)(addr >> 17),0);
	}

	/* Start page programming */
	hid_transmit(C_WR_PAGE,(BYTE)(addr >> 9),(BYTE)(addr >> 1),0);

	return 0;
}

//----------------------------------------------------------------------------
//  ページリード.
//----------------------------------------------------------------------------
int hidasp_page_read(long addr, unsigned char *wd, int pagesize)
{
	int n, l;
	char buf[BUFF_SIZE];

	// set page
	if (addr >= 0) {
		hidCommand(HIDASP_SET_PAGE,0x20,0,0);	// Set Page mode , FlashRead
	}

	// Load the page data into page buffer
	n = 0;	// n は読み込み基点offset.
	while (n < pagesize) {
		l = REPORT_LENGTHMAX - 1;	// MAX
		if (pagesize - n < l) {
			l = pagesize - n;
		}
		hidCommand(HIDASP_PAGE_RD,l,0,0);	// PageRead , length

		memset(buf + 3, 0, l);
		hidRead(hHID, buf, REPORT_LENGTHMAX, REPORT_IDMAX);
		memcpy(wd + n, buf + 1, l);			// Copy result.

#if 1	// 進捗状況を逐次表示する
		report_update(l);
#else	// 進捗状況を128バイト毎に表示する
		if (n % 128 == 0) {
			report_update(128);
		}
#endif

#if DEBUG
		fprintf(stderr, "  p: %02x %02x %02x %02x\n",
				buf[1] & 0xff, buf[2] & 0xff, buf[3] & 0xff, buf[4] & 0xff);
#endif
		n += l;
	}

	return 0;
}
#endif
//----------------------------------------------------------------------------
int	UsbGetDevID(void)
{
	return dev_id;
}
//----------------------------------------------------------------------------
int	UsbGetDevVersion(void)
{
	return dev_version;
}
int	UsbGetDevCaps(void)
{
	return dev_bootloader;
}
//----------------------------------------------------------------------------
void wait_ms(int ms)
{
	Sleep(ms);
}
//----------------------------------------------------------------------------
void hidasp_delay_ms(int dly)
{
	// 10mS以上待つ場合は、分割.
	while(dly>10) {
		hidCommand(HIDASP_WAIT_MSEC,10,0,0);	// WAIT <n> mSec
		dly-=10;
	}

	// 10の端数だけ待つ. 0の場合は NOPコマンドのようになる(実質1mSは待つ)
	hidCommand(HIDASP_WAIT_MSEC,dly,0,0);	// WAIT <n> mSec
}
