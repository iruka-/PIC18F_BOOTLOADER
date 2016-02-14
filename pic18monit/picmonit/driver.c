/*
 ******************************************************************************
 *	USB Driver
 ******************************************************************************
int		open_dev_check_string(struct usb_device *dev,usb_dev_handle *handle);
usb_dev_handle	*open_dev(void);
int		bulkRead(uchar *buf, int Length);
int		hidReadPoll(uchar *buf,int Length, int id);
int		hidWrite(uchar *buf, int Length, int id);
int		hidWriteBuffer(uchar *buf, int len);
int		hidReadBuffer(uchar *buf, int len , int id);
int		hidCommand(int cmd,int arg1,int arg2,int arg3);
int		hidPokeMem(int addr,int data0,int mask);
int		hidPeekMem(int addr);
int		hidasp_init(void);
void	hidasp_close(void);
int		UsbGetDevID(void);
int		UsbGetDevVersion(void);
int		UsbGetDevCaps(void);
void	wait_ms(int ms);
 ******************************************************************************
static	void memdump(char *msg, uchar *buf, int len);
static	int  usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen);
 ******************************************************************************
*/

#ifndef	_LINUX_

#include <windows.h>
#include "../libusb/include/lusb0_usb.h"

#else

//	libusb0-dev: /usr/include/usb.h
#include <usb.h>

#endif

#include <stdio.h>
#include <memory.h>


#include "driver.h"
#include "../firmware/hidcmd.h"
#include "monit.h"

//----------------------------------------------------------------------------
//	Switches
//----------------------------------------------------------------------------

#define DEBUG 		   	0		// for DEBUG

#define DEBUG_PKTDUMP  	0		// HID Reportパケットをダンプする.

#define CHECK_COUNT		4		// 4: Connect時の Ping test の回数.

#define BUFF_SIZE		256

#define	TIMEOUT_MS 		400		//4000

//----------------------------------------------------------------------------
//	Defines
//----------------------------------------------------------------------------
static int MY_VID = 0x04D8;	// MicroChip
static int MY_PID = 0x0204;	// Bulk

static	int dev_id       = 0;	// ターゲットID: 0x25 もしくは 0x14 だけを許容.
static	int dev_version  = 0;	// ターゲットバージョン hh.ll
static	int dev_bootloader= 0;	// ターゲットにbootloader機能がある.

static	int	flash_size     = 0;	// FLASH_ROMの最終アドレス+1
static	int	flash_end_addr = 0;	// FLASH_ROMの使用済み最終アドレス(4の倍数切捨)

/* the device's endpoints */
#define EP_IN  				0x81
#define EP_OUT 				0x01
#define PACKET_SIZE 		64
#define	VERBOSE				1
#define	REPORT_MATCH_DEVICE	0
#define	REPORT_ALL_DEVICES	0

#define	USE_BULK_TRANSFER	1		//1:バルク転送使用をデフォルトにする.

#define	if_V	if(VERBOSE)

static  usb_dev_handle *usb_dev = NULL; /* the device handle */
static	char verbose_mode = 0;

//----------------------------------------------------------------------------
//--------------------------    Tables    ------------------------------------
//----------------------------------------------------------------------------
//  HID Report のパケットはサイズ毎に 3種類用意されている.
	#define	REPORT_ID1			0
	#define	REPORT_LENGTH1		65


#if	DEBUG_PKTDUMP 
//----------------------------------------------------------------------------
//  メモリーダンプ.
static void memdump(char *msg, uchar *buf, int len)
{
	int j;
	fprintf(stderr, "%s", msg);
	for (j = 0; j < len; j++) {
		fprintf(stderr, " %02x", buf[j] & 0xff);
		if((j & 0x0f)== 0x0f)
			fprintf(stderr, "\n +");
	}
	fprintf(stderr, "\n");
}
#endif


static int  usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen)
{
	char    buffer[256];
	int     rval, i;

	if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, buffer, sizeof(buffer), 1000)) < 0)
		return rval;
	if(buffer[1] != USB_DT_STRING)
		return 0;
	if((unsigned char)buffer[0] < rval)
		rval = (unsigned char)buffer[0];
	rval /= 2;
	/* lossy conversion to ISO Latin1 */
	for(i=1; i<rval; i++) {
		if(i > buflen)  /* destination buffer overflow */
			break;
		buf[i-1] = buffer[2 * i];
		if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
			buf[i-1] = '?';
	}
	buf[i-1] = 0;
	return i-1;
}

int	open_dev_check_string(struct usb_device *dev,usb_dev_handle *handle)
{
	int len1,len2;
	char string1[256];
	char string2[256];

	len1 = usbGetStringAscii(handle, dev->descriptor.iManufacturer,
	                         0x0409, string1, sizeof(string1));
	len2 = usbGetStringAscii(handle, dev->descriptor.iProduct,
	                         0x0409, string2, sizeof(string2));
	if((len1<0)||(len2<0)) {
		return 0;
	}
#if	1
	printf("iManufacturer:%s\n",string1);
	printf("iProduct:%s\n",string2);
#endif
	return 1;	//合致した.
}


usb_dev_handle *open_dev(void)
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
			        dev->descriptor.idProduct == MY_PID) {
#if	REPORT_MATCH_DEVICE
				if(verbose_mode) {
					dump_info(dev);
				}
#endif

				handle = usb_open(dev);
				int cf=dev->config->bConfigurationValue;
				if((rc = usb_set_configuration(handle, cf))<0) {
					printf("usb_set_configuration error(1) cf=%d\n",cf);
#ifdef	_LINUX_
					int fn=dev->config->interface->altsetting->bInterfaceNumber;

					if((rc = usb_detach_kernel_driver_np(handle,fn)) <0 ) {
						printf("usb_detach_kernel_driver_np(%d) error\n",fn);
						usb_close(handle);
						return NULL;
					}
#endif


					if((rc = usb_set_configuration(handle, dev->config->bConfigurationValue))<0) {
					printf("usb_set_configuration error(2)\n");
						usb_close(handle);
						return NULL;

					}
				}

				if(usb_claim_interface(handle, 0) < 0) {
					printf("error: claiming interface 0 failed\n");
					usb_close(usb_dev);
					return NULL;
				}


				if( open_dev_check_string(dev,handle) == 1) {
					return handle;	//一致.
				}
				usb_close(handle);
				handle = NULL;
			}
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
/*
 */
int bulkRead(uchar *buf, int Length)
{
	int rc;
	rc = usb_bulk_read(usb_dev, EP_IN , (char*)buf , Length, TIMEOUT_MS);
#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
	printf("Length=%d rc=%d\n",Length,rc);
#endif
	return rc;
}

int	hidReadPoll(uchar *buf,int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = usb_bulk_read(usb_dev, EP_IN , (char*) buf+1 , Length-1, TIMEOUT_MS);
#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
#endif
	return rc;
}
/*
 */
int hidWrite(uchar *buf, int Length, int id)
{
	int rc;
	buf[0] = id;

#if	DEBUG_PKTDUMP
//	printf("bulkWrite(buf,%d,%d)\n",Length,id);
	memdump("WR", buf, Length);
#endif

	buf++;
	rc = usb_bulk_write(usb_dev, EP_OUT , (char*)buf , Length -1 , TIMEOUT_MS);

#if	DEBUG_PKTDUMP
//	memdump("WR", buf, Length);
#endif
	return rc;
}

/*
 */
int	hidWriteBuffer(uchar *buf, int len)
{
	len = 65;
	return hidWrite(buf, len , 0);
}

/*
 */
int	hidReadBuffer(uchar *buf, int len , int id)
{
	len = 64;
	return bulkRead(buf+1, len);
}
/*
 */
int hidCommand(int cmd,int arg1,int arg2,int arg3)
{
	uchar buf[BUFF_SIZE];

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
	uchar buf[BUFF_SIZE];
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
	uchar buf[BUFF_SIZE];
	memset(buf , 0, sizeof(buf) );

	buf[1] = HIDASP_PEEK;
	buf[2] = 1;
	buf[3] = addr;
	buf[4] =(addr>>8);

	hidWrite(buf, REPORT_LENGTH1, REPORT_ID1);
	hidReadBuffer( buf, REPORT_LENGTH1,0 );
	return buf[1];
}

/*********************************************************************
 *
 *********************************************************************
 */
int hidasp_init(void)
{
	unsigned char rd_data[BUFF_SIZE];
	int *fl_size;
	int i, r, result;

	result = 0;

	verbose_mode = VERBOSE;

	usb_init(); /* initialize the library */

#if	0
	// USBのデバッグをする場合は、ここを有効にする.
	usb_set_debug(3);
#endif

	usb_find_busses(); /* find all busses */
	usb_find_devices(); /* find all connected devices */

	if(!(usb_dev = open_dev())) {
		printf("error: device not found!\n");
		return 0;
	}
	printf("open device ok.\n");


	for (i = 0; i < CHECK_COUNT; i++) {
		hidCommand(HIDASP_TEST,(i),(i+1),(i+2));	// Connection test
		r = bulkRead(rd_data+1 , 64);

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

		fl_size = (int*)&rd_data[9];
//		memdump("fl_size",fl_size,8);
		flash_size     = fl_size[0];
		flash_end_addr = fl_size[1];

	}
	{
		char *sBoot="(Application)";
		if(dev_bootloader==1) {sBoot="(Bootloader)";}
		fprintf(stderr, "TARGET DEV_ID=%x VER=%d.%d%s\n",dev_id
				,dev_version>>8,dev_version & 0xff,sBoot
				);
	}
	return 1;	// OK.
}



/*********************************************************************
 *
 *********************************************************************
 */

void hidasp_close(void)
{
	if(	usb_dev ) {
		usb_release_interface(usb_dev, 0);
		usb_close(usb_dev);
	}
}
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
