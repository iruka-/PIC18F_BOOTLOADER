/* hidasp.c
 * original: binzume.net
 * modify: senshu , iruka
 * 2008-09-22 : for HIDaspx.
 * 2008-10-13 : hidspx-1012b (binzume)
 */

#define DEBUG 		   		0		// for DEBUG
#define DEBUG_PKTDUMP  		0		// HID Reportパケットをダンプする.
#define DUMP_PRODUCT   		0		// 製造者,製品名を表示.
#define	REPORT_ALL_DEVICES	0
#define	REPORT_MATCH_DEVICE	0

#define CHECK_COUNT			4		// 4: Connect時の Ping test の回数.
#define BUFF_SIZE			256

#define LIBUSB  // USE libusb  http://libusb.sourceforge.net/


#include <stdio.h>
#include <string.h>
#include "avrspx.h"
#include "hidasp.h"
#include "hidcmd.h"


#include <usb.h>

#if	0
//  obdev
#define MY_VID		0x16c0		/* 5824 in dec, stands for VOTI */
#define MY_PID_libusb	0x05dc		/* libusb:0x05dc, obdev's free PID */
#define MY_PID 		0x05df		/* HID: 0x05df, obdev's free PID */
#define	MY_Manufacturer	"YCIT"
#define	MY_Product		"HIDaspx"
//	MY_Manufacturer,MY_Product のdefine を外すと、VID,PIDのみの照合になる.
//	どちらかをはずすと、その照合を省略するようになる.
#endif

#define MY_VID			0x04d8
#define MY_PID_libusb	0x003c
#define MY_PID 			0x003c
#define	MY_Manufacturer	"AVCetc"
#define	MY_Product		"PIC18spx"


//	MY_Manufacturer,MY_Product のdefine を外すと、VID,PIDのみの照合になる.
//	どちらかをはずすと、その照合を省略するようになる.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//typedef	cmdBuf RxBuf;

static	int dev_id       = 0;	// ターゲットID: 0x25 もしくは 0x14 だけを許容.
static	int dev_version  = 0;	// ターゲットバージョン hh.ll
static	int dev_bootloader= 0;	// ターゲットにbootloader機能がある.

static	int have_isp_cmd = 0;	// ISP制御の有無.
static	int	flash_size     = 0;	// FLASH_ROMの最終アドレス+1
static	int	flash_end_addr = 0;	// FLASH_ROMの使用済み最終アドレス(4の倍数切捨)
static	int	hidmon_mode =  1;
#define	REPORT_ID1			0
#define	REPORT_LENGTH1		65

void dump_info(  struct usb_device *dev );

/* the device's endpoints */
#define EP_IN  				0x81
#define EP_OUT 				0x01
#define PACKET_SIZE 		64
#define	VERBOSE				1


static	int have_ledcmd = 0;	// LED制御の有無.

#if	DEBUG_PKTDUMP
static	void memdump(char *msg, char *buf, int len);
#endif

#if	0
//----------------------------------------------------------------------------
//--------------------------    Tables    ------------------------------------
//----------------------------------------------------------------------------
//  HID Report のパケットはサイズ毎に ３種類用意されている.
#define	REPORT_ID1			1	// 8  REPORT_COUNT(6)
#define	REPORT_ID2			2	// 32 REPORT_COUNT(30)
#define	REPORT_ID3			3	// 40 REPORT_COUNT(38)

#define	REPORT_LENGTH1		7	// 8  REPORT_COUNT(6)
#define	REPORT_LENGTH2		31	// 32 REPORT_COUNT(30)
#define	REPORT_LENGTH3		39	// 40 REPORT_COUNT(38)


#endif

//	最大の長さをもつ HID ReportID,Length
#define	REPORT_IDMAX		0
#define	REPORT_LENGTHMAX	65	//REPORT_LENGTH3

#define	PAGE_WRITE_LENGTH	32	// Page Writeでは32byte単位の転送を心掛ける.
								// Length5より7バイト少ない値である必要がある.

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



typedef struct usbDevice usbDevice_t;
#define USBRQ_HID_GET_REPORT    0x01
#define USBRQ_HID_SET_REPORT    0x09
#define USB_HID_REPORT_TYPE_FEATURE 3
//static int  usesReportIDs;

usb_dev_handle *usb_dev = NULL;				// USB-IO dev handle
struct usb_device   *gdev;				// @@@ add by tanioka


static int hidInit(){
	usb_init();
	return 1;
}

static void hidFree(){
}


static int usbhidGetStringAscii(usb_dev_handle *dev, int index, char *buf, int buflen)
{
	char    buffer[256];
	int     rval, i;

    if((rval = usb_get_string_simple(dev, index, buf, buflen)) >= 0) /* use libusb version if it works */
        return rval;
    rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, 
		(USB_DT_STRING << 8) + index, 0x0409, buffer, sizeof(buffer), 5000);

    if (rval < 0)
        return rval;
    if(buffer[1] != USB_DT_STRING){
        *buf = 0;
        return 0;
    }
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1: */
    for(i=1;i<rval;i++){
        if(i > buflen)              /* destination buffer overflow */
            break;
        buf[i-1] = buffer[2 * i];
        if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
            buf[i-1] = '?';
    }
    buf[i-1] = 0;
    return i-1;
}

static int hidOpen(int my_pid)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *handle;
	int rc;

	for(bus = usb_get_busses(); bus; bus = bus->next) {
		for(dev = bus->devices; dev; dev = dev->next) {

#if	REPORT_ALL_DEVICES
			dump_info(dev);
#endif

			//
			//	VendorID,ProductID の一致チェック.
			//
			if(dev->descriptor.idVendor  == MY_VID &&
			        dev->descriptor.idProduct == my_pid) {
#if	REPORT_MATCH_DEVICE
				dump_info(dev);
#endif

				handle = usb_open(dev);
				//printf("usb_open()=0x%x\n",handle);
				
				int cf=dev->config->bConfigurationValue;
				if((rc = usb_set_configuration(handle, cf))<0) {
					printf("usb_set_configuration error(1) cf=%d\n",cf);
					int fn=dev->config->interface->altsetting->bInterfaceNumber;
					if((rc = usb_detach_kernel_driver_np(handle,fn)) <0 ) {
						printf("usb_detach_kernel_driver_np(%d) error\n",fn);
						usb_close(handle);
						return 0;
					}

					if((rc = usb_set_configuration(handle, dev->config->bConfigurationValue))<0) {
					printf("usb_set_configuration error(2)\n");
						usb_close(handle);
						return 0;

					}
				}
				if(usb_claim_interface(handle, 0) < 0) {
					printf("error: claiming interface 0 failed\n");
					usb_close(handle);
					return 0;
				}


//				if( open_dev_check_string(dev,handle) == 1) 
				{
					usb_dev = handle;
					return 1;	//(int)handle;	//一致.
				}
				usb_close(handle);
				handle = NULL;
			}
		}
	}
	return 0;
}


static void hidClose(){
	if (usb_dev) {
		usb_release_interface((void*)usb_dev, gdev->config->interface->altsetting->bInterfaceNumber);
//		usb_reset((void*)usb_dev);
		usb_close((void*)usb_dev);
	}
	usb_dev=NULL;
#if DEBUG
    printf("usb_close.\n");
#endif
}

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
static int hidRead(char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = usb_interrupt_read(usb_dev, EP_IN , buf+1 , Length-1, 5000);
#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
	printf("id=%d Length=%d rc=%d\n",id,Length,rc);
#endif
	return rc;
}

int	hidReadPoll(char *buf,int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = usb_interrupt_read(usb_dev, EP_IN , buf+1 , Length, 5000);
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
//static 
int hidWrite(char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = usb_interrupt_write(usb_dev, EP_OUT , buf+1 , Length -1 , 5000);
//	rc = HidD_SetFeature(h, buf, Length);

#if	DEBUG_PKTDUMP
	memdump("WR", buf, Length);
#endif
	return rc;
}

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
	return hidWrite(buf, len , 0);
}

/*
 *	hidRead()を使用して、デバイス側から buf[] データを len バイト取得する.
 *	長さを自動判別して、ReportIDも自動選択する.
 *
 *	戻り値はHidD_GetFeatureの戻り値( 0 = 失敗 )
 *
 */
int	hidReadBuffer(char *buf, int len)
{
	len = 65;
	return hidRead(buf, len, 0);
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

	return hidWrite((char*)buf, 65, 0);
}

#if	0
static int hidRead(usb_dev_handle *h, char *buf, int Length, int id)
{
	int rc;

	buf[0] = id;
/*
int usb_control_msg(usb_dev_handle *dev, int requesttype, 
	int request, int value, int index, char *bytes, int size, int timeout);
 */
	rc = usb_control_msg(usb_dev, USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_ENDPOINT_IN, 
	USBRQ_HID_GET_REPORT, USB_HID_REPORT_TYPE_FEATURE << 8 | id , 0, buf, Length, 5000);

	if (rc == Length){
		return rc+1;
	} else {
		fprintf(stderr, "Error hidRead(): %s (rc=%d), %d\n", usb_strerror(),rc, Length);
		return 0;
	}
}

static int hidWrite(usb_dev_handle *h, char *buf, int Length, int id)
{
	int rc;

	buf[0] = id;

	rc = usb_control_msg(usb_dev, USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_ENDPOINT_OUT,
	USBRQ_HID_SET_REPORT,  USB_HID_REPORT_TYPE_FEATURE << 8 | (id & 0xff), 0, buf, Length, 5000);

    if (rc == Length){
		return rc;
	} else {
		fprintf(stderr, "Error hidWrite(): %s (rc=%d), %d \n", usb_strerror(), rc, Length);
        return 0;
	}
}




/*
 *	hidWrite()を使用して、デバイス側に buf[] データを len バイト送る.
 *	長さを自動判別して、ReportIDも自動選択する.
 *
 *	戻り値はHidD_SetFeatureの戻り値( 0 = 失敗 )
 *
 */
int	hidWriteBuffer(char *buf, int len)
{
	int report_id = 0;
	int length    = 0;

	if (len <= REPORT_LENGTH1) {
		length= REPORT_LENGTH1;report_id = REPORT_ID1;
	} else if (len <= REPORT_LENGTH2) {
		length= REPORT_LENGTH2;report_id = REPORT_ID2;
	} else if (len <= REPORT_LENGTH3) {
		length= REPORT_LENGTH3;report_id = REPORT_ID3;
	}

	if( report_id == 0) {
		// 適切な長さが選択できなかった.
		fprintf(stderr, "Error at hidWriteBuffer. len=%d\n",len);
		exit(1);
		return 0;
	}

	return hidWrite(buf, length, report_id);
}

/*
 *	hidRead()を使用して、デバイス側から buf[] データを len バイト取得する.
 *	長さを自動判別して、ReportIDも自動選択する.
 *
 *	戻り値はHidD_GetFeatureの戻り値( 0 = 失敗 )
 *
 */
int	hidReadBuffer(char *buf, int len)
{
	int report_id = 0;
	int length    = 0;

	if (len <= REPORT_LENGTH1) {
		length= REPORT_LENGTH1;report_id = REPORT_ID1;
	} else if (len <= REPORT_LENGTH2) {
		length= REPORT_LENGTH2;report_id = REPORT_ID2;
	} else if (len <= REPORT_LENGTH3) {
		length= REPORT_LENGTH3;report_id = REPORT_ID3;
	}

	if( report_id == 0) {
		// 適切な長さが選択できなかった.
		fprintf(stderr, "Error at hidWriteBuffer. len=%d\n",len);
		exit(1);
		return 0;
	}

	return hidRead(usb_dev, buf, length, report_id);
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
	unsigned char buf[128];

	memset(buf , 0, sizeof(buf) );

	buf[1] = cmd;
	buf[2] = arg1;
	buf[3] = arg2;
	buf[4] = arg3;

	return hidWrite(buf, REPORT_LENGTH1, REPORT_ID1);
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
	unsigned char buf[128];
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
	return hidWrite(buf, REPORT_LENGTH1, REPORT_ID1);
}

int hidPeekMem(int addr)
{
	unsigned char buf[128];
	memset(buf , 0, sizeof(buf) );

	buf[1] = HIDASP_PEEK;
	buf[2] = 1;
	buf[3] = addr;
	buf[4] =(addr>>8);

	hidWrite(buf, REPORT_LENGTH1, REPORT_ID1);
	hidReadBuffer( buf, REPORT_LENGTH1 );
	return buf[1];
}

#endif

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
	return hidWrite(buf, REPORT_LENGTH1, REPORT_ID1);
}

int hidPeekMem(int addr)
{
	unsigned char buf[BUFF_SIZE];
	memset(buf , 0, sizeof(buf) );

	buf[1] = HIDASP_PEEK;
	buf[2] = 1;
	buf[3] = addr;
	buf[4] =(addr>>8);

	hidWrite(buf, REPORT_LENGTH1, REPORT_ID1);
	hidReadBuffer( buf, REPORT_LENGTH1 );
	return buf[1];
}


#define	USICR			0x2d	//
#define	DDRB			0x37	// PB4=RST PB3=LED
#define	DDRB_WR_MASK	0xf0	// 制御可能bit = 1111_0000
#define	PORTB			0x38	// PB4=RST PB3=LED
#define	PORTB_WR_MASK	0		// 0 の場合はMASK演算は省略され、直書き.

#define HIDASP_RST		0x10	// RST bit

/*
 *	LEDの制御.
#define HIDASP_RST_H_GREEN	0x18	// RST解除,LED OFF
#define HIDASP_RST_L_BOTH	0x00	// RST実行,LED ON
#define HIDASP_SCK_PULSE 	0x80	// RST-L SLK-pulse ,LED ON	@@kuga
 */


static void hidSetStatus(int ledstat)
{
	unsigned char rd_data[BUFF_SIZE];
	int ddrb;
//	if( have_ledcmd ) 
	{
		hidCommand(HIDASP_SET_STATUS,0,ledstat,0);	// cmd,portd(&0000_0011),portb(&0001_1111),0
		// ↓PIC18Fファームの場合、ハンドシェークパケットは必ず受け取る必要がある.
		hidRead( rd_data ,REPORT_LENGTH1, REPORT_ID1);
		//   そうでないと、次のコマンドの応答パケットに、このゴミが送信され、以後１つずつずれる.
	}
}



#if	DEBUG_PKTDUMP
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


//----------------------------------------------------------------------------
//  初期化.
//----------------------------------------------------------------------------
int hidasp_init(char *string)
{
	extern bool f_hid_libusb;
	unsigned char rd_data[128];
	int *fl_size;
	int result;
	int i, r, pid;

	hidInit();
	if (f_hid_libusb) {
		pid = MY_PID_libusb;
	} else {
		pid = MY_PID;
	}

#if	DEBUG
	// USBのデバッグをする場合は、ここを有効にする.
	usb_set_debug(3);
#endif

	usb_find_busses(); /* find all busses */
	usb_find_devices(); /* find all connected devices */


	if (hidOpen(pid) == 0) {
#if DEBUG
		fprintf(stderr, "ERROR: fail to hidOpen()\n");
#endif
		return 1;
	}

//	GetDevCaps();
//	Sleep(100);

#if DEBUG
	fprintf(stderr, "HIDaspx Connection check!\n");
#endif

	for (i = 0; i < CHECK_COUNT; i++) {
//		hidCommand(HIDASP_TEST,(i),0,0);	// Connection test
		hidCommand(HIDASP_TEST,(i),(i+1),(i+2));	// Connection test
		r = hidRead(rd_data ,REPORT_LENGTH1, REPORT_ID1);
#if DEBUG
		fprintf(stderr, "HIDasp Ping(%d) = HIDASP_TEST(%02x) %02x %02x\n", i, 
							rd_data[0], rd_data[1], rd_data[2]);
#endif
		if (r == 0) {
			fprintf(stderr, "ERR(rc=0). fail to Read().\n");
			return HIDASP_MODE_ERROR;
		}

		dev_id         = rd_data[2];
		dev_version    = rd_data[3] | (rd_data[4]<<8) ;
		dev_bootloader = rd_data[5];

		fl_size = (int*)&rd_data[9];
//		memdump("fl_size",fl_size,8);
		flash_size     = fl_size[0];
		flash_end_addr = fl_size[1];
	}
	{	char *sBoot="(Application)";
		if(dev_bootloader==1) {sBoot="(Bootloader)";}
		fprintf(stderr, "TARGET DEV_ID=%x VER=%d.%d%s\n",dev_id
				,dev_version>>8,dev_version & 0xff,sBoot
				);
	}

#if	1
	if (!hidmon_mode) {
		hidCommand(HIDASP_SET_STATUS,0,HIDASP_RST_H_GREEN,0);	// RESET HIGH
		// ↓PIC18Fファームの場合、ハンドシェークパケットは必ず受け取る必要がある.
	r = hidRead((char*)rd_data ,REPORT_LENGTH1, REPORT_ID1);
	if( rd_data[1] == 0xaa ) {				// ISPコマンド(isp_enable)が正常動作した.
		have_isp_cmd = HIDASP_ISP_MODE;		// ISP制御OK.
		result |= HIDASP_ISP_MODE;
	} else if( rd_data[1] == DEV_ID_FUSION ) {
		result |= HIDASP_FUSION_OK;			// NEW firmware
	} else if( rd_data[1] == DEV_ID_STD ) {
		result &= ~HIDASP_FUSION_OK;		// OLD firmware
//	} else if( rd_data[1] == DEV_ID_MEGA88 ) {		// USB-IO mode
//		result |= HIDASP_USB_IO_MODE;		// ISP制御NG.
//	} else if( rd_data[1] == DEV_ID_MEGA88_USERMODE ) {		// USB-IO mode
//		result |= HIDASP_USB_IO_MODE;		// ISP制御NG.
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

	return 0;	// OK.
}


#define HIDASP_NOP 			  0	//これは ISPのコマンドと思われる?

//----------------------------------------------------------------------------
//  終了.
//----------------------------------------------------------------------------
void hidasp_close()
{
	if(	usb_dev ) {
		usb_release_interface(usb_dev, 0);
		usb_close(usb_dev);
	}
}




//----------------------------------------------------------------------------
//  ターゲットマイコンをプログラムモードに移行する.
//----------------------------------------------------------------------------
int hidasp_program_enable(int delay)
{
	unsigned char buf[128];
	unsigned char res[4];
	int i, rc;
#if 1		//AVRSP type protocole
	int tried;

	rc = 1;
	hidSetStatus(HIDASP_RST_H_GREEN);			// RESET HIGH
	hidSetStatus(HIDASP_RST_L_BOTH);			// RESET LOW (H PULSE)
	hidCommand(HIDASP_SET_DELAY,delay,0,0);		// SET_DELAY
	Sleep(30);				// 30

	for(tried = 1; tried <= 3; tried++) {
		for(i = 1; i <= 32; i++) {

			buf[0] = 0xAC;
			buf[1] = 0x53;
			buf[2] = 0x00;
			buf[3] = 0x00;
			hidasp_cmd(buf, res);

			if (res[2] == 0x53) {
				rc = 0;
				goto hidasp_program_enable_exit;
			}
			if (tried <= 2) {
				break;
			}
			hidSetStatus(HIDASP_SCK_PULSE);			// RESET LOW SCK H PULSE shift scan point
		}
	}
#else
	// ISPモード移行が失敗したら再試行するように修正 by senshu(2008-9-16)
	rc = 1;
	for (i = 0; i < 3; i++) {
		hidSetStatus(HIDASP_RST_H_GREEN);			// RESET HIGH
		Sleep(2);				// 10 => 100

		hidSetStatus(HIDASP_RST_L_BOTH);			// RESET LOW
		hidCommand(HIDASP_SET_DELAY,delay,0,0);		// SET_DELAY
		Sleep(30);				// 30

		buf[0] = 0xAC;
		buf[1] = 0x53;
		buf[2] = 0x00;
		buf[3] = 0x00;
		hidasp_cmd(buf, res);

		if (res[2] == 0x53) {
			/* AVRマイコンからの同期(ISPモード)が確認できた */
			rc = 0;
			break;
		} else {
			/* AVRマイコンからの同期が確認できないので再試行 */
			Sleep(50);
		}
	}
#endif
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

	r = hidWrite(buf, REPORT_LENGTH1 , REPORT_ID1);
#if DEBUG
	fprintf(stderr, "hidasp_cmd %02X, cmd: %02X %02X %02X %02X ", buf[1], cmd[0], cmd[1], cmd[2], cmd[3]);
#endif

	if (res != NULL) {
		r = hidRead(buf, REPORT_LENGTH1 , REPORT_ID1);
		memcpy(res, buf + 1, 4);
#if DEBUG
		fprintf(stderr, " --> res: %02X %02X %02X %02X\n", res[0], res[1], res[2], res[3]);
#endif
	}

	return 1;
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
	char buf[128];
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
	char buf[128];

#if	0
	fprintf(stderr,"dev_id=%x flashsize=%x addr=%x\n",	dev_id,flashsize,(int)addr);
#endif
	if(	(dev_id == DEV_ID_FUSION) && (flashsize <= (128*1024)) ) {
		// addres_set , page_write , isp_command が融合された高速版を実行.
		return hidasp_page_write_fast(addr,wd,pagesize);
	}
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
	char buf[128];

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
		hidRead(buf, REPORT_LENGTHMAX, REPORT_IDMAX);
		memcpy(wd + n, buf + 1, l);			// Copy result.

#if 1
		report_update(l);
#else
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
//----------------------------------------------------------------------------
