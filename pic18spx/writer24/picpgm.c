/*********************************************************************
 *	PIC18 のFlashを読み・書きする.
 *********************************************************************

接続:

 AVR用ISP 6PIN                  PIC18F2550/14K50

	1 MISO    ------------------   PGD
	2 Vcc     ------------------   Vcc
	3 SCK     ------------------   PGC
	4 MOSI    ------------------   PGM
	5 RESET   ------------------   MCLR
	6 GND     ------------------   GND


 PIC18F2550 の接続
	25:RC6=TX  = Rset
	33:RB0=SDI = MISO
	34:RB1=SCK = SCK
	26:RC7=SDO = MOSI
 *********************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "monit.h"
#include "util.h"
#include "hidasp.h"
#include "picdevice.h"

typedef	struct {
	uchar report_id;
	uchar cmd;
	uchar size;		// bit7:6 をarenaに使用する予定.
	uchar adr[3];
	uchar cmd4;
	uchar ms;
	uchar data[64-7];
} picBuf;

typedef	struct {
	uchar report_id;
	uchar cmd;
	uchar size;
	uchar wait;
	uchar data[61];
} bitBuf;

typedef struct {
	union {
		ushort word[3];
		uchar  byte[6];
	};
} PACK_data;


#define	DEBUG_CMD		0		// 1:デバッグする.
#define	DEBUG_STREAM	0		// 1:データ列をデバッグする.

#define	USE_READ_FUNC	1		// 1:デバイス側のreadflash関数を使用する.
#define	USE_WRITE_FUNC	1		// 1:デバイス側のwriteflash関数を使用する.
#define	BLOCK_SIZE		64		// 1回の読み込みバイト数(hexイメージ換算)
								// 実際には 32bit中 24bitのみ有効なので48byteが転送される.

int	portAddress(char *s);

#if	HIDMON88
enum {
	SCK =5,
	MISO=4,
	MOSI=3,
	SS  =2,
};
#endif

#if	HIDMON_2313
// tiny2313
enum {
	SCK =7,
	MISO=5,
	MOSI=6,
	SS  =4,
};
#endif

enum {
	b_0000 = 0x00,
	b_0001 = 0x01,
	b_1000 = 0x08,
	b_1001 = 0x09,
	b_1100 = 0x0c,	//
	b_1101 = 0x0d,	// TBLPTR+=2
	b_1111 = 0x0f,	// Write!
};

#ifdef	_AVR_WRITER_
#define	PGM	 MOSI
#define	PGC	 SCK
#define	PGD	 MISO
#define	MCLR SS
#else

//	for PIC18F Firmware
#define	PGD	 0		// RB0
#define	PGC	 1		// RB1

#define	MCLR 6		// RC6
#define	PGM	 7		// RC7

#endif


#define	if_D	if(DEBUG_CMD)


//  BitBangワーク.

static uchar	bit_data;
static uchar	bit_cnt  ;
static uchar	bit_wait ;
static uchar	bit_stream[64];

//  BitBangデータビット.

#define	bitMCLR	0x08
#define	bitPGM	0x04
#define	bitPGD	0x02
#define	bitPGC	0x01

//	方向レジスタ (0=出力)

#define	dirMCLR	0x80
#define	dirPGM	0x40
#define	dirPGD	0x20
#define	dirPGC	0x10

#define	ENTER_ISP_KEY	"MCHQ"

#define	W_0uS	0
#define	W_1uS	1
#define	W_1mS	201
#define	W_25mS	225
#define	W_50mS	250

/*********************************************************************
 *	１ミリ秒単位の時間待ち. Windowsの Sleep()関数を呼び出す.
 *   実際の関数は、hidasp.c にある。
 *********************************************************************
 */
//------------------------------------------------------------------------
void ispDelay(uchar d)
{
	uchar i;
	for(i=2;i<d;i++) ;
}

extern void wait_ms(int ms);
void SetAddress24(int adr);

/*********************************************************************
 *	１μ秒単位の時間待ち.
 *********************************************************************
 *	Windows側ではUSBフレーム時間の消費が発生するので、実際には不要.
 *	マイコンファーム側に移植するときはマイコン側でそれぞれ実装の必要がある.
 */
void wait_us(int us)
{
	int ms = us / 1000;

	if(ms) wait_ms(ms);
}


#if	0	// 以下は、PIC18F専用.
/*********************************************************************
 *
 *********************************************************************
 */

void GetData8L(int adr,uchar *buf,int len)
{
	picBuf cmd;
	memset(&cmd,0,sizeof(picBuf));
	cmd.cmd   = PICSPX_GETDATA8L ;
	cmd.size  = len;
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;
	cmd.adr[2]= adr>>16;
	if( QueryAVR((cmdBuf *)&cmd,buf,len) == 0) {
		//失敗.
	}
}

/*********************************************************************
 *	4bitのコマンド と 16bitのデータを送信.
 *********************************************************************
 */
void SetCmd16L(int cmd4,ushort *cmd16,int len,int ms)
{
	picBuf cmd;
	memset(&cmd,0,sizeof(picBuf));
	cmd.cmd   = PICSPX_SETCMD16L ;
	cmd.size  = len;
	cmd.cmd4  = cmd4;
	cmd.ms    = ms;
	memcpy(cmd.data,cmd16,len*2);

	if( QueryAVR((cmdBuf *)&cmd,0,0) == 0) {
		//失敗.
	}
}

/*********************************************************************
 *	4bitのコマンド と 16bitのデータを送信.
 *********************************************************************
 */
void SetCmd16(int cmd4,int data16,int ms)
{
	ushort buf[2];
	buf[0]=data16;
	SetCmd16L(cmd4,buf,1,ms);
}

/*********************************************************************
 *	TBLPTR に24bitアドレスをセットする.
 *********************************************************************
 */
void SetAddress24(int adr)
{
	picBuf cmd;
	memset(&cmd,0,sizeof(picBuf));
	cmd.cmd   = PICSPX_SETADRS24 ;
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;
	cmd.adr[2]= adr>>16;
	if( QueryAVR((cmdBuf *)&cmd,0,0) == 0) {
		//失敗.
	}

}
#endif		// 以上、PIC18F専用.



/*********************************************************************
 *	PIC24F: ターゲット側での ReadFlashの実行.
 *********************************************************************
 */
void SetCmd24L(int cmd8,int cmd4,uchar *data,uchar *result,int len,int reply_len,int adr)
{
	picBuf cmd;
	memset(&cmd,0,sizeof(picBuf));
	cmd.cmd   = cmd8;
	cmd.size  = len;
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;
	cmd.adr[2]= adr>>16;
	cmd.cmd4  = cmd4;
	if(data) {
		memcpy(cmd.data,data,56);
	}

	if( QueryAVR((cmdBuf *)&cmd,(void*)result,reply_len) == 0) {
		//失敗.
	}
}

/*********************************************************************
 *	BitBangストリームをダンプ表示する.
 *********************************************************************
 */
void dumpStream(uchar *p,int cnt,int wait)
{
	int i,c,m=0x80,b,t;
	char tbl[8]="rmdcRMDC";
	printf("stream: cnt=%d wait=%d\n",cnt,wait);
	printf("  ");
		for(i=0;i<cnt;i++) {
			c = i % 10;t = '0'+c;
			putchar(t);
		}
	printf("\n");

	for(b=0;b<8;b++) {
		printf("%c:",tbl[b]);
		for(i=0;i<cnt;i++) {
			c = p[i];
			if(c&m) t='1';else t='0';
			putchar(t);
		}
		printf("\n");
		m>>=1;
	}
}
/*********************************************************************
 *	BitBangストリームをデバイスに送信する。必要なら受信データも受け取る
 *********************************************************************
 */
void BitBang(uchar *stream,int cnt,int wait,uchar *result)
{
	bitBuf cmd;
	int	reply_len = 0;
	memset(&cmd,0,sizeof(bitBuf));
	memcpy(&cmd.data,stream,cnt);

	if(result) {
		cmd.cmd   = PICSPX_BITREAD ;
		reply_len = cnt;
	}else{
		cmd.cmd   = PICSPX_BITBANG ;
	}
	cmd.size= cnt;
	cmd.wait= wait;

#if	DEBUG_STREAM			// 1:データ列をデバッグする.
	dumpStream(cmd.data,cmd.size,cmd.wait);
#endif

	if( QueryAVR((cmdBuf *)&cmd,result,reply_len) == 0) {
		//失敗.
	}

}

/*********************************************************************
 *	BitBang送信バッファを初期化する.
 *********************************************************************
 */
void resetBitBang(int data,int wait)
{
	bit_data = data;
	bit_cnt  = 0;
	bit_wait = wait;
}
/*********************************************************************
 *	１レコード分(最大60byte)のBitBangストリームを送信.
 *********************************************************************
 */
void flushBitBang()
{
	if(bit_cnt) {
	//	printf("BitBang [%d]\n",bit_cnt);
		BitBang(bit_stream,bit_cnt,bit_wait,NULL);
		bit_cnt  = 0;
	}
}
/*********************************************************************
 *	１レコード分のBitBangストリームを送信して、結果も受け取る.
 *********************************************************************
 */
void flushBitBangRead(uchar *buf)
{
	if(bit_cnt) {
#if	DEBUG_STREAM			// 1:データ列をデバッグする.
		printf("*flushBitBangRead bit_cnt=%d wait=%d\n",bit_cnt,bit_wait);
#endif

		BitBang(bit_stream,bit_cnt,bit_wait,buf);

#if	DEBUG_STREAM			// 1:データ列をデバッグする.
		printf("*INPUT=\n");
		dumpStream(buf,bit_cnt,0);
#endif
		bit_cnt  = 0;
	}
}
/*********************************************************************
 *	１サンプル分のデータをBitBangバッファに追記する.
 *********************************************************************
 	data は MSB [dirMCLR,dirPGM,dirPGD,dirPGC , MCLR,PGM,PGD,PGC] LSB

 */
void addBitBang(int data)
{
	if(bit_cnt >= 60) {
		printf("BitBang buffer overflow\n");
		exit(1);
	}

	bit_data = data;
	bit_stream[bit_cnt++]=data;
}
/*********************************************************************
 *	１クロック分の PGD 送信
 *********************************************************************

   PGD <データ1bit>
   
   PGC _____|~~~~~~


 */
void addClock1(int b)
{
	int bit = bit_data;
	if(b) bit |=  bitPGD;
	else  bit &= ~bitPGD;
	
	bit &= ~bitPGC;
	addBitBang(bit);

	bit |=  bitPGC;
	addBitBang(bit);
}

/*********************************************************************
 *	PGDのポート方向切り替え ( 0 = 出力 )
 *********************************************************************
 */
//	0=out 1=in
void addPGDdir(int b)
{
	int bit = bit_data;
	if(b) bit |=  dirPGD;
	else  bit &= ~dirPGD;

	addBitBang(bit);
}

/*********************************************************************
 *	LSBから bitsカウント分のビットをPGDに乗せて送りこむ.
 *********************************************************************
 */
void addData(int c,int bits)
{
	int m,b;
	m = 1;
	for(b=0;b<bits;b++) {
		addClock1(c & m);
		m<<=1;
	}
	b = bit_data & ~bitPGC;
	addBitBang(b);
}
/*********************************************************************
 *	１バイト分のビットエンディアン反転 (LSB <--> MSB)
 *********************************************************************
 */
int bswap(int c)
{
	int i,r=0,m=0x80;
	for(i=0;i<8;i++) {
		if(c & 1) r|=m;
		c>>=1;
		m>>=1;
	}
	return r;
}


/*********************************************************************
 *********************************************************************
 *	ターゲット側で実行する関数のプロトタイピング
 *********************************************************************
 *********************************************************************
 */
void add_data(uchar c,uchar bits)
{
	uchar b;
	for(b=0;b<bits;b++) {	// LSBから bits ビット送る.
		addClock1(c & 1);
		c>>=1;
	}
}
void add_data4(uchar c)
{
	add_data(c,4);
}
void add_data8(uchar c)
{
	add_data(c,8);
}
void add_data24(uchar u,uchar h,uchar l)
{
	add_data8(l);
	add_data8(h);
	add_data8(u);
	uchar b = bit_data & ~bitPGC;
	addBitBang(b);
}
void send_data24(uchar u,uchar h,uchar l)
{
	add_data4(b_0000);	// send SIX command '0000'
	add_data24(u,h,l);
	flushBitBang();
}

/*********************************************************************
 *********************************************************************
 *	ターゲット側で実行する関数のプロトタイピングここまで.
 *********************************************************************
 *********************************************************************
 */


//----------------------------------------------------------------------------
//  メモリーダンプ.
//
/*
static void memdump(char *msg, char *buf, int len)
{
	int j;
//	fprintf(stderr, "%s", msg);
	fprintf(stderr, "%x", (int)buf);
	for (j = 0; j < len; j++) {
		fprintf(stderr, " %02x", buf[j] & 0xff);
		if((j & 0xf)== 0x0f)
			fprintf(stderr, "\n +");
	}
	fprintf(stderr, "\n");
}
*/
/*********************************************************************
 *
 *********************************************************************
 */
void EnterISP(void)
{
	int   bit = dirPGM;
	uchar key[] = ENTER_ISP_KEY;

	resetBitBang(bit,W_25mS);
	addBitBang(bit);
	addBitBang(bit);
	addBitBang(bit);
	addBitBang(bit);
	flushBitBang();
	resetBitBang(bit,W_1mS);

	addBitBang(bit);
	addBitBang(bit | bitMCLR);
	addBitBang(bit | bitMCLR);
	addBitBang(bit | bitMCLR);
	addBitBang(bit);

	flushBitBang();
	resetBitBang(bit,W_1uS);

	// 32bitのKEYを送信する.
	addData(bswap(key[0]),8);
	addData(bswap(key[1]),8);

	flushBitBang();		//streamの長さが足りないので、ここで一回送信する.

	addData(bswap(key[2]),8);
	addData(bswap(key[3]),8);

	addBitBang(bit);
	addBitBang(bit);
	addBitBang(bit);
	addBitBang(bit);
	flushBitBang();


	resetBitBang(bit,W_25mS);
	addBitBang(bit);
	addBitBang(bit | bitMCLR);
	addBitBang(bit | bitMCLR);
	flushBitBang();
	resetBitBang(bit| bitMCLR,W_0uS);

	addData(b_0000,4);
	addData(0,5);
	flushBitBang();
	addData(0,24);
	flushBitBang();
}

/*********************************************************************
 *	<n>bitデータの送信
 *********************************************************************
 */
void sendData(int c,int bits)
{
	addData(b_0000,4);	// send SIX command '0000'
	addData(c,bits);	// send 24bit inst.
	flushBitBang();

#if	DEBUG_CMD			// 1:データ列をデバッグする.
	printf("sendData(0x%06x,%d)\n",c,bits);
#endif
}
/*********************************************************************
 *	24bitデータの送信
 *********************************************************************
 */
void sendData24(int c)
{
#if	0
	sendData(c,24);
#else
	uchar u = c>>16;
	uchar h = c>>8;
	uchar l = c;
	send_data24(u,h,l);
#endif
}
/*********************************************************************
 *	16bitデータの受信
 *********************************************************************
 */
static	int	getData16(void)
{
	uchar buf[64];
	int i,r=0,m=1;
	addData(0x0000,16);	// send 2 byte '0'
	flushBitBangRead(buf);

	// 送信したBitBangストリームが以下のような波形であると仮定.
	//      01234567890123456789012345678901
	// PGD  ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ = Hi-Z
	// PGC  LHLHLHLHLHLHLHLHLHLHLHLHLHLHLHLH
	//       | | | | | | | | | | | | | | | |
	//		上記Hのエッジでサンプリングする.
	
	for(i=0;i<16;i++) {
		if(buf[i*2+1] & bitPGD) r|=m;
		m<<=1;
	}
#if	DEBUG_CMD			// 1:データ列をデバッグする.
	printf("getData()=%04x\n",r);
#endif
	return r;
}

#define	PGD_INPUT	1	// PGDを入力モードに.
#define	PGD_OUTPUT	0	// PGDを出力モードに.

/*********************************************************************
 *	16bitデータの受信
 *********************************************************************
 */
int	recvData16(void)
{
	int rc;
	addData(b_0001,4);	// send SIX command '0000'
	addData(0x00  ,8);	// send 1 byte '0000_0000'
	addPGDdir(PGD_INPUT);
	//printf("\n\n==recvData16==\n");
	flushBitBang();
	
	rc = getData16();
	
	addPGDdir(PGD_OUTPUT);
	sendData24(0x000000);
	
	return rc;
}
/*********************************************************************
 *
 *********************************************************************
 */
 
void ExitISP(void)
{
	int   bit = dirPGM;
	resetBitBang(bit,W_50mS);

	addBitBang(bit | bitMCLR);
	addBitBang(bit);
	addBitBang(bit | bitMCLR);
	addBitBang(bit | bitMCLR);
	addBitBang(bit | bitMCLR | dirMCLR | dirPGM | dirPGC | dirPGD);	//全て Hi-Z化.
	addBitBang(bit | bitMCLR | dirMCLR | dirPGM | dirPGC | dirPGD);	//全て Hi-Z化.
	flushBitBang();
}
/*********************************************************************
 *	デバイス接続チェック.
 *********************************************************************
 */
int	DeviceCheck(void)
{
	int id;

	sendData24(0x040200);	// GOTO 0x200
	sendData24(0x000000);	// NOP 
	sendData24(0x255AA2);	// MOV #0x55AA, W2
	sendData24(0x883C22);	// MOV W2, VLSI
	sendData24(0x000000);	// NOP 
	id = recvData16();
	printf(" DeviceCheck=%04x\n",id);
	return id;
}
/*********************************************************************
 *	共通ルーチン・アドレスセットアップ
 *********************************************************************
 */
static	void setup_addr24(int adr)
{
	int adr_l =(adr)       & 0xffff;
	int adr_h =(adr >>16 ) & 0x00ff;

	sendData24(0x000000);				// NOP
	sendData24(0x040200);				// GOTO 0x200
	sendData24(0x000000);				// NOP 

	sendData24(0x200000| (adr_h<<4) );	// MOV  #adr_h, W0
	sendData24(0x880190);				// MOV  W0, TBLPAG
	sendData24(0x200006| (adr_l<<4) );	// MOV  #adr_l, W6

	sendData24(0x207847);				// MOV  #VLSI, W7
	sendData24(0x000000);				// NOP 
}
static	void setup_addr24write(int adr,int nvmcon)
{
	int adr_l =(adr)       & 0xffff;
	int adr_h =(adr >>16 ) & 0x00ff;

	sendData24(0x000000);				// NOP
	sendData24(0x040200);				// GOTO 0x200
	sendData24(0x000000);				// NOP 
	sendData24(0x20000A| (nvmcon<<4));	// MOV  #0x4001, W10
	sendData24(0x883B0A);				// MOV  W10, NVMCON

	sendData24(0x200000| (adr_h<<4) );	// MOV  #adr_h, W0
	sendData24(0x880190);				// MOV  W0, TBLPAG
	sendData24(0x200007| (adr_l<<4) );	// MOV  #adr_l, W7

}
/*********************************************************************
 *	デバイスＩＤの取得.
 *********************************************************************
 */
int GetDeviceID(void)
{
	int dev_id,revision;

	sendData24(0x000000);	// NOP
	sendData24(0x040200);	// GOTO 0x200
	sendData24(0x000000);	// NOP 
	sendData24(0x200FF0);	// MOV  #0x00FF, W0
	sendData24(0x880190);	// MOV  W0, TBLPAG
	sendData24(0x207841);	// MOV  #VLSI, W1
	sendData24(0x200000);	// MOV  #0x0000, W0
	sendData24(0x000000);	// NOP 
	sendData24(0xBA0890);	// TBLRDL [W0], [W1]
	sendData24(0x000000);	// NOP 
	sendData24(0x000000);	// NOP 
	dev_id = recvData16();   
	sendData24(0x200020);	// MOV #0x0002, W0
	sendData24(0x000000);	// NOP 
	sendData24(0xBA0890);	// TBLRDL [W0], [W1]
	sendData24(0x000000);	// NOP 
	sendData24(0x000000);	// NOP 
	revision = recvData16();

	printf(" dev_id=%04x ",dev_id);
	printf(" revision=%04x\n",revision);

	return dev_id;
}

/*********************************************************************
 *	PICターゲットへのアクセス開始 (f=1) / 終了(f=0)
 *********************************************************************
 */
void PicPgm(int f)
{
	if(f) {
		EnterISP();
	}else{
		ExitISP();
	}
}

static	void bset_nvmcon(void)
{
	sendData24(0x000000);					// NOP 
	sendData24(0x000000);					// NOP 
	sendData24(0xA8E761);					// BSET NVMCON
	sendData24(0x000000);					// NOP 
	sendData24(0x000000);					// NOP 
}

static	void WaitComplete(void)
{
	int r;
	bset_nvmcon();
	do {
		sendData24(0x040200);	// GOTO 0x200
		sendData24(0x000000);	// NOP 
		sendData24(0x803B00);	// MOV NVMCON, W0
		sendData24(0x883C20);	// MOV W0, VLSI
		sendData24(0x000000);	// NOP 
		r = recvData16();
	} while (r & 0x8000);
}
/*********************************************************************
 *	チップ消去コマンドの発行.
 *********************************************************************
 */
void EraseCmd(int devtype)
{
	sendData24(0x000000);	// NOP
	sendData24(0x040200);	// GOTO 0x200
	sendData24(0x000000);	// NOP 

	sendData24(0x200800);	// MOV #0x80, W0
	sendData24(0x880190);	// MOV W0, TBLPAG
	sendData24(0x207FE1);	// MOV #0x7FE, W1
	sendData24(0x2000C2);	// MOV #12, W2
	sendData24(0x000000);	// NOP 
	sendData24(0xBA1931);	// TBLRDL [W1++], [W2++]
	sendData24(0x000000);	// NOP 
	sendData24(0x000000);	// NOP 

	sendData24(0x2404F0);	// MOV #0x404F, W0 
	sendData24(0x883B00);	// MOV W0, NVMCON
	sendData24(0x200000);	// MOV #0, W0
	sendData24(0x200801);	// MOV #0x80
	sendData24(0x880191);	// MOV W1, TBLPAG 
	sendData24(0xBB0800);	// TBLWTL W0, [W0] 

	WaitComplete();

	sendData24(0x240031);	// MOV #0x4003, W1
	sendData24(0x883B01);	// MOV W1, NVMCON 
	sendData24(0x200800);	// MOV #0x80, W0 
	sendData24(0x880190);	// MOV W0, TBLPAG 
	sendData24(0x207FE1);	// MOV #0x7FE, W1 
	sendData24(0x2000C2);	// MOV #12, W2 
	sendData24(0x000000);	// NOP 
	sendData24(0xBB18B2);	// TBLWTL [W2++], [W1++]

	WaitComplete();

	sendData24(0x040200);	// GOTO 0x200
	sendData24(0x000000);	// NOP 

}
/*********************************************************************
 *	バルク・イレース.
 *********************************************************************
 */
void BulkErase(int cmd80)
{
	EraseCmd(0);
}

/*********************************************************************
    [ 0 ][ 1 ]   w0
    [ 2 ][ 5 ]   w1
    [ 3 ][ 4 ]   w2
 *********************************************************************
 */
static	inline void pack3word(uchar *t,uchar *p)
{
	p[0] = t[0];
	p[1] = t[1];
	p[2] = t[2];
	
	p[4] = t[4];
	p[5] = t[5];
	p[3] = t[6];
}

#if	USE_WRITE_FUNC			// 1:デバイス側のwriteflash関数を使用する.

/*********************************************************************
 *	書き込み.
 *********************************************************************
 */
void WriteFlash(int adr,uchar *buf,int len)
{
	int i,cmd4=0;
	uchar *t=buf;
	uchar *pd;
	uchar data[256];
	int cnt;
	cnt = BLOCK_SIZE / 8;

	while(len>0) {
		pd = data;
		for(i=0;i<cnt;i++) {	// 6byte x 8 = 48byte
			pack3word(t,pd);	// 8byteを6byteにパックする.
			t +=8;	// 読み出しバッファは８バイトx8=64byte進める.
			pd+=6;	// デバイスからのデータは６バイト単位で処理.
		}

		if(len == BLOCK_SIZE) cmd4=2;

		SetCmd24L(PICSPX_WRITE24F,cmd4,(uchar *)data,NULL,cnt,0,adr>>1);

		adr += BLOCK_SIZE;
		len -= BLOCK_SIZE;
		cmd4 = 1;
	}
}



#else	//	USE_WRITE_FUNC			// 1:デバイス側のwriteflash関数を使用する.

/*********************************************************************
 *	書き込み.
 *********************************************************************
 */
void WriteFlash(int adr,uchar *buf,int len)
{
	int i;
	uchar *t=buf;
	PACK_data pd;

	setup_addr24write(adr>>1,0x4001);

	for(i=0;i<len;i+=8,t+=8) {
		pack3word(t,(char *)&pd);

		sendData24(0xEB0300);					// CLR W6
		sendData24(0x200000| (pd.word[0]<<4) );	// MOV  #w0, W0
		sendData24(0x200001| (pd.word[1]<<4) );	// MOV  #w1, W1
		sendData24(0x200002| (pd.word[2]<<4) );	// MOV  #w2, W2
		sendData24(0xBB0BB6);					// TBLWTL [W6++], [W7]
		sendData24(0x000000);					// NOP 
		sendData24(0x000000);					// NOP 
		sendData24(0xBBDBB6);					// TBLWTH.B [W6++], [W7++]
		sendData24(0x000000);					// NOP 
		sendData24(0x000000);					// NOP 
		sendData24(0xBBEBB6);					// TBLWTH.B [W6++], [++W7]
		sendData24(0x000000);					// NOP 
		sendData24(0x000000);					// NOP 
		sendData24(0xBB1BB6);					// TBLWTL [W6++], [W7++]
		sendData24(0x000000);					// NOP 
		sendData24(0x000000);					// NOP 
	}
	
	WaitComplete();
	
	sendData24(0x040200);					// GOTO 0x200
	sendData24(0x000000);					// NOP 
}

#endif	//	USE_WRITE_FUNC			// 1:デバイス側のwriteflash関数を使用する.

/*********************************************************************
 *	書き込みテスト.
 *********************************************************************
 */
void WriteFuse(int adr,uchar *buf)
{
	int i,cfg;
	setup_addr24write(adr>>1,0x4003);
	for (i = 0; i < 8; i += 4) {					/* ---- Repeat for two cfg words ---- */
		cfg=buf[i]|(buf[i+1]<<8);
		sendData24(0x200006| (cfg<<4) );	// MOV #cfg, W6
		sendData24(0x000000);				// NOP 
		sendData24(0xBB1B86);				// TBLWTL W6, [W7++]

		WaitComplete();
	}
	sendData24(0x040200);					// GOTO 0x200
	sendData24(0x000000);					// NOP 
}


/*********************************************************************
    [ 0 ][ 1 ]   w0
    [ 2 ][ 5 ]   w1
    [ 3 ][ 4 ]   w2
 *********************************************************************
 */
static	inline void unpack3word(uchar *t,uchar *p)
{
	t[0] = p[0];
	t[1] = p[1];

	t[2] = p[2];
	t[6] = p[3];

	t[4] = p[4];
	t[5] = p[5];

	t[3] = 0;
	t[7] = 0;
}

#if	USE_READ_FUNC			// 1:デバイス側の関数を使用する.

/*********************************************************************
 *	読み出しルーチン・メイン.
 *********************************************************************
 */
void ReadFlash(int adr,uchar *buf,int len,int dumpf)
{
	int   len0=len;
	int   cmd4=0;
	uchar *t=buf;
	uchar *pd;
	uchar result[256];
	int   cnt = BLOCK_SIZE / 8;
	int	  i;

	if(dumpf)printf("%06x :",adr);

	while(len>0) {
		pd = result;
		SetCmd24L(PICSPX_READ24F,cmd4,NULL,result,cnt,64,adr>>1);
		// 取り込んだpackデータをHEXファイルのバッファに展開する.
		
		for(i=0;i<cnt;i++) {	// 6byte x 8 = 48byte
			unpack3word(t,pd);
			t +=8;	// 書き込みバッファは８バイトx8=64byte進める.
			pd+=6;	// デバイスからのデータは６バイト単位で処理.
		}
		adr += BLOCK_SIZE;
		len -= BLOCK_SIZE;
	}

	if(dumpf) {
		for(i=0;i<len0;i++) {
			printf(" %02x",buf[i]);
		}
		printf("\n");
	}
}

#else	//	USE_READ_FUNC			// 1:デバイス側の関数を使用する.


/*********************************************************************
 *	読み出しルーチン・メイン.
 *********************************************************************
 */
void ReadFlash(int adr,uchar *buf,int len,int dumpf)
{
	int i;
	uchar *t=buf;
	PACK_data pd;

	if(dumpf)printf("%06x :",adr);

	setup_addr24(adr>>1);

	for(i=0;i<len;i+=8,t+=8) {
		sendData24(0xBA0B96);				// TBLRDL [W6], [W7]
		sendData24(0x000000);				// NOP 
		sendData24(0x000000);				// NOP 
		pd.word[0] = recvData16();

		sendData24(0xBADBB6);				// TBLRDH.B [W6++], [W7++]
		sendData24(0x000000);				// NOP 
		sendData24(0x000000);				// NOP 
		sendData24(0xBAD3D6);				// TBLRDH.B [++W6], [W7--]
		sendData24(0x000000);				// NOP 
		sendData24(0x000000);				// NOP 
		pd.word[1] = recvData16();

		sendData24(0xBA0BB6);				// TBLRDL [W6++], [W7]
		sendData24(0x000000);				// NOP 
		sendData24(0x000000);				// NOP 
		pd.word[2] = recvData16();
		unpack3word(t,(uchar *)&pd);
	}

	if(dumpf) {
		for(i=0;i<len;i++) {
			printf(" %02x",buf[i]);
		}
		printf("\n");
	}
}


#endif	//	USE_READ_FUNC			// 1:デバイス側の関数を使用する.


/*********************************************************************
 *	読み出しルーチン・ config用.
 *********************************************************************
 */
void ReadFuse(int adr,uchar *buf,int len,int dumpf)
{
	int i,data;
	uchar *t=buf;

	if(dumpf)printf("%06x :",adr);

	setup_addr24(adr>>1);

	for(i=0;i<len;i+=4,t+=4) {
		sendData24(0xBA0BB6);				// TBLRDL [W6++], [W7]
		sendData24(0x000000);				// NOP 
		sendData24(0x000000);				// NOP 
		data = recvData16();
		t[0]=data;
		t[1]=data>>8;
		t[2]=0;
		t[3]=0;
		sendData24(0x040200);				// GOTO 0x200
		sendData24(0x000000);				// NOP 
	}
	if(dumpf) {
		for(i=0;i<len;i++) {
			printf(" %02x",buf[i]);
		}
		printf("\n");
	}
}
/*********************************************************************
 *
 *********************************************************************
 */
