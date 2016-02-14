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
	uchar data[60-2];
} picBuf;

typedef	struct {
	uchar report_id;
	uchar cmd;
	uchar size;
	uchar wait;
	uchar data[61];
} bitBuf;



#define	DEBUG_CMD		0		// 1:デバッグする.
//
#define	DEBUG_STREAM	0		// 1:データ列をデバッグする.
//
#define	SOFTWARE_PGC	0		// PGC/PGDのコントロールをホストPCみずから実行する.(遅い)


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


#if	SOFTWARE_PGC		// PGC/PGDのコントロールをホストPCみずから実行する.(遅い)

/*********************************************************************
 *
 *********************************************************************
 */
static  int datab=0;	// PortB に最後に書き込んだ値を覚えておく.
static  int datac=0;	// PortC に最後に書き込んだ値を覚えておく.

//	各ポートの実アドレスを保持する.
static	int	Portb;
static	int Pinb ;
static	int Ddrb ;		// 1=out 0=in

static	int	Portc;
static	int Pinc ;
static	int Ddrc ;		// 1=out 0=in

/*********************************************************************
 *	USBを経由してポートへの書き込みを行う.
 *********************************************************************
 */
static void set_port(int adr,int data)
{
	UsbPoke(adr,0,data,0);
}

/*********************************************************************
 *	USBを経由してポートの読み出しを行う.
 *********************************************************************
 */
static int  get_port(int adr)
{
	return UsbPeek(adr,0);
}
/*********************************************************************
 *	ポートＢの書き込み専用.最後に書き込んだ値を覚えておく.
 *********************************************************************
 */
static void set_portb(int data)
{
	set_port(Portb,data);
	datab = data;
}

static void set_portc(int data)
{
	set_port(Portc,data);
	datac = data;
}

/*********************************************************************
 *	ポートＢの書き込みデータに対して任意ビットのOn/Off (実際の書き込みは行わない)
 *********************************************************************
 */
static void set_bitf(int bit,int f)
{

	if(bit<2) {
		if(f) {
			datab |=  (1<<bit);
		}else{
			datab &= ~(1<<bit);
		}
	
	}else{

		if(f) {
			datac |=  (1<<bit);
		}else{
			datac &= ~(1<<bit);
		}
	}
}

/*********************************************************************
 *	ポートＢの書き込みデータに対して任意ビットのOn/Offを行い、書き込みを実行.
 *********************************************************************
 */
static void set_bit(int bit,int f)
{
	set_bitf(bit,f);
	if(bit<2) {
		set_portb(datab);
	}else{
		set_portc(datac);
	}
}


/*********************************************************************
 *	PGDビットの入出力方向を切り替える.(0=入力)
 *********************************************************************
 *	f=0 ... PGD=in
 *	f=1 ... PGD=out
 */

void SetPGDDir(int f)
{
#ifdef	_AVR_WRITER_
	int dirb = (1<<PGC) | (1<<PGM) | (1<<MCLR);

	if(f) dirb |= (1<<PGD);

	set_port(Ddrb ,dirb);
#else

//	PIC
	int dirc = (1<<PGM) | (1<<MCLR);

	int dirb = (1<<PGC) ;
	if(f) dirb |= (1<<PGD);

	set_port(Ddrb ,dirb ^ 0xff);
	set_port(Ddrc ,dirc ^ 0xff);
	
#endif
}

/*********************************************************************
 *	初期化
 *********************************************************************
 */
void PicInit(void)
{
#ifdef	_AVR_WRITER_
	Portb = portAddress("portb");
	Pinb  = portAddress("pinb");
	Ddrb  = portAddress("ddrb");
#else
	Portb = portAddress("latb");
	Pinb  = portAddress("portb");
	Ddrb  = portAddress("trisb");

	Portc = portAddress("latc");
	Pinc  = portAddress("portc");
	Ddrc  = portAddress("trisc");

/*	printf("Portb=%x\n",Portb);
	printf("Portc=%x\n",Portc);
	printf("Pinb=%x\n" ,Pinb);
	printf("Pinc=%x\n" ,Pinc);
*/
#endif

	set_portb(0);
	SetPGDDir(1);		// PGD=out
}

/*********************************************************************
 *	PICターゲットへのアクセス開始 (f=1) / 終了(f=0)
 *********************************************************************
 */
void PicPgm(int f)
{
	if(f) {
		set_portb(0);	//全ての信号をLにする.
		set_portc(0);	//全ての信号をLにする.
		wait_ms(1);
		set_bit(PGM,1);	// PGM=H
		wait_ms(1);
		set_bit(MCLR,1);// MCLR=H
	}else{
		set_bit(PGD,0);// MCLR=L
		set_bit(PGC,0);// MCLR=L
		wait_ms(1);
		set_bit(MCLR,0);// MCLR=L
		wait_ms(1);
		set_bit(PGM,0);	// PGM=L
		wait_ms(1);
		set_portb(0);	//全ての信号をLにする.
		set_portc(0);	//全ての信号をLにする.
	}
}

/*********************************************************************
 *	1bit送信.
 *********************************************************************

        +---+
  PGC --+   +---

  PGD [ b=0ならL ]
        b=1ならH
 */
void SetCmdb1(int b)
{
	set_bitf(PGD,b);
	set_bit(PGC,1);wait_us(1);
	set_bit(PGC,0);wait_us(1);
if_D	printf("%d",b);
}

void SetCmdb1Long(int b,int ms)
{
	set_bitf(PGD,b);
	set_bit(PGC,1);
	if(ms) wait_ms(ms);
	else   wait_us(1);
	set_bit(PGC,0);wait_us(1);
if_D	printf("%d",b);
}

/*********************************************************************
 *	1bit受信.
 *********************************************************************
        +---+
  PGC --+   +---

           ↑ ここで読む.

  PGD [ 入力    ]
 */
int GetCmdb1(void)
{
	int b;

	set_bit(PGC,1);wait_us(1);

	b = get_port(Pinb) & (1<<PGD);

//	printf(" --- Pinb(%x)=%02x\n",Pinb,get_port(Pinb));

	set_bit(PGC,0);wait_us(1);

	return b;
}
/*********************************************************************
 *	n bit 送信.
 *********************************************************************
 */
void SetCmdN(int cmdn,int len)
{
	int i,b;
if_D	printf("[");

	for(i=0;i<len;i++) {
		b = cmdn & 1;
		SetCmdb1(b);
		cmdn >>=1;
	}
if_D	printf("]");

	if(len>=16) {
if_D	printf("\n");
	}
}

/*********************************************************************
 *	4 bit 送信 , 最後の PGC_H時間を指定.
 *********************************************************************
 */
void SetCmdNLong(int cmdn,uchar len,int ms)
{
	int i,b;
if_D	printf("[");

	for(i=0;i<(len-1);i++) {
		b = cmdn & 1;
		SetCmdb1(b);
		cmdn >>=1;
	}
	{
		b = cmdn & 1;
		SetCmdb1Long(b,ms);
	}
if_D	printf(" } ");
}

/*********************************************************************
 *	8 bit 受信.
 *********************************************************************
 */
static	int GetData8b(void)
{
	int i,data8=0,mask=1;
	SetPGDDir(0);		// PGD=in

	for(i=0;i<8;i++,mask<<=1) {
		if( GetCmdb1() ) {
			data8 |= mask;
		}
	}
	SetPGDDir(1);		// PGD=out
	return data8;
}

/*********************************************************************
 *	4bitのコマンド と 16bitのデータを送信.
 *********************************************************************
 */
void SetCmd16L(int cmd4,ushort *buf,int len,int ms)
{
	int i,data16;
	for(i=0;i<len;i++) {
		data16 = *buf++;
		SetCmdNLong(cmd4,4,ms);
		SetCmdN(data16,16);
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
 *	TBLPTR[U,H,L] のどれかに8bit値をセットする.
 *********************************************************************
 */
static	void SetAddress8x(int adr,int inst)
{
	adr &= 0x00ff;
	adr |= 0x0e00;

	SetCmd16(b_0000,adr,0);		// 0E xx  : MOVLW xx
	SetCmd16(b_0000,inst,0);		// 6E Fx  : MOVWF TBLPTR[U,H,L]
}
/*********************************************************************
 *	TBLPTR に24bitアドレスをセットする.
 *********************************************************************
 */
void SetAddress24(int adr)
{
	SetAddress8x(adr>>16,0x6ef8);	// TBLPTRU
	SetAddress8x(adr>>8 ,0x6ef7);	// TBLPTRH
	SetAddress8x(adr    ,0x6ef6);	// TBLPTRL
}

/*********************************************************************
 *	TBLPTR から１バイト読み出し. TBLPTRはポスト・インクリメントされる.
 *********************************************************************
 */
static	int GetData8(void)
{
	SetCmdN(b_1001,4);
	SetCmdN(0 , 8);
	return GetData8b();
}


void GetData8L(int adr,uchar *buf,int len)
{
	int i,c;
	SetAddress24(adr);
	for(i=0;i<len;i++) {
		c = GetData8();
		*buf++ = c;
//		if(dumpf)printf(" %02x",c);
	}
}

#else	//HARDWARE_PGC		// PGC/PGDのコントロールをホストPCみずから実行する.(遅い)



/*********************************************************************
 *********************************************************************
 *	API
 *		void GetData8L(int adr,uchar *buf,int len)	ＲＯＭ読み出し
 *		void setAddress24L(int adr) 				ＲＯＭアドレスセット.
 *		void SetCmd16L(int cmd4,uchar *buf,int len,int ms);	16bit値書き込みを実行.
 *********************************************************************

//	PICライター系 (0x10～0x1f) 
#define PICSPX_SETADRS24      0x10	// 24bitのアドレスをTBLPTRにセットする.
// SETADRS24 [ CMD ] [ --- ] [ADRL] [ADRH] [ADRU]  ---> 返答不要 LEN=0のGETDATA8Lと同じ処理.

#define PICSPX_GETDATA8L      0x11	//  8bitデータ列を取得する.
// GETDATA8L [ CMD ] [ LEN ] [ADRL] [ADRH] [ADRU]  ---> 返答 data[32]

#define PICSPX_SETCMD16L      0x12	// 16bitデータ列を書き込む.
// SETCMD16L [ CMD ] [ LEN ] [ADRL] [ADRH] [ADRU] [CMD4] [ MS ] data[32]

#define PICSPX_SETPGM         0x14	// 書き込み開始(f=1)/終了(f=0).
// SETPGM    [ CMD ] [ f ] 

 *********************************************************************
 */

void GetData8L(int adr,uchar *buf,int len)
{
	picBuf cmd;
	memset(&cmd,0,sizeof(cmdBuf));
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
	memset(&cmd,0,sizeof(cmdBuf));
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
	memset(&cmd,0,sizeof(cmdBuf));
	cmd.cmd   = PICSPX_SETADRS24 ;
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;
	cmd.adr[2]= adr>>16;
	if( QueryAVR((cmdBuf *)&cmd,0,0) == 0) {
		//失敗.
	}

}

/*********************************************************************
 *	PICターゲットへのアクセス開始 (f=1) / 終了(f=0)
 *********************************************************************
 */
/*
void PicPgm(int f)
{
	picBuf cmd;
	memset(&cmd,0,sizeof(cmdBuf));
	cmd.cmd   = PICSPX_SETPGM ;	//| arena;
	cmd.size  = f;
	if( QueryAVR((cmdBuf *)&cmd,0,0) == 0) {
		//失敗.
	}
}
*/
/*********************************************************************
 *	初期化
 *********************************************************************
 */
void PicInit(void)
{
//	set_portb(0);
//	SetPGDDir(1);		// PGD=out
}



#endif	//SOFTWARE_PGC		// PGC/PGDのコントロールをホストPCみずから実行する.(遅い)

/*********************************************************************
 *
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
 *
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
#if	0
	if(result) {
		int i;
		printf("RESULT:");
		for(i=0;i<reply_len;i++) {
			printf("%x",result[i]);
		}
		printf("\n");
	}
#endif
}

/*********************************************************************
 *
 *********************************************************************
 */
void resetBitBang(int data,int wait)
{
	bit_data = data;
	bit_cnt  = 0;
	bit_wait = wait;
}
/*********************************************************************
 *
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
void flushBitBangRead(uchar *buf)
{
	if(bit_cnt) {
		printf("*flushBitBangRead bit_cnt=%d wait=%d\n",bit_cnt,bit_wait);
		BitBang(bit_stream,bit_cnt,bit_wait,buf);
#if	DEBUG_STREAM			// 1:データ列をデバッグする.
		printf("*INPUT=\n");
		dumpStream(buf,bit_cnt,0);
#endif
		bit_cnt  = 0;
	}
}
/*********************************************************************
 *
 *********************************************************************
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
 *
 *********************************************************************
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
//----------------------------------------------------------------------------
//  メモリーダンプ.
//static 
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

/*********************************************************************
 *
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
void sendData24(int c,int nops)
{
	int i;
	sendData(c,24);
	
	for(i=0;i<nops;i++) {
		sendData(0x000000,24);
	}
}
int	getData16(void)
{
	uchar buf[64];
	int i,r=0,m=1;
	addData(0x0000,16);	// send 2 byte '0'
	flushBitBangRead(buf);

	for(i=0;i<16;i++) {
		if(buf[i] & bitPGD) r|=m;
		m<<=1;
	}
	return r;
}

#define	PGD_INPUT	1
#define	PGD_OUTPUT	0

int	recvData16(void)
{
	int rc;
	addData(b_0001,4);	// send SIX command '0000'
	addData(0x00  ,8);	// send 1 byte '0000_0000'
	addPGDdir(PGD_INPUT);
printf("\n\n==recvData16==\n");
	flushBitBang();
	
	rc = getData16();
	
	addPGDdir(PGD_OUTPUT);
	sendData(0x000000,24);
	
	return rc;
}
/*********************************************************************
 *
 *********************************************************************
 */
void EnterISP(void)
{
	int bit = 0;	// MCLR,PGM,PGD,PGC全て出力、全てLo
	resetBitBang(bit,W_1mS);
	addBitBang(bit);					// 全てLo
	addBitBang(bit);					// 全てLo
	addBitBang(bit | bitPGM);			// PGM =H
	addBitBang(bit | bitPGM | bitMCLR);	// MCLR=H
	flushBitBang();
}
/*********************************************************************
 *
 *********************************************************************
 */
void ExitISP(void)
{
	int bit = 0;
	resetBitBang(bit,W_1mS);
	addBitBang(bit | bitPGM|bitMCLR) ;	// PGM=H,MCLR=Hと仮定.
	addBitBang(bit | bitPGM );			// PGM=H,MCLR=L
	addBitBang(bit );					// PGM=L
	addBitBang(bit | bitMCLR | dirMCLR | dirPGM | dirPGC | dirPGD);	//全て Hi-Z化.
	addBitBang(bit | bitMCLR | dirMCLR | dirPGM | dirPGC | dirPGD);	//全て Hi-Z化.
	flushBitBang();
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

/*********************************************************************
 *	チップ消去コマンドの発行.
 *********************************************************************
 */
void EraseCmd(int cmd05,int cmd04)
{
	SetAddress24(0x3c0005);
	SetCmd16(b_1100,cmd05,0);

	SetAddress24(0x3c0004);
	SetCmd16(b_1100,cmd04,0);

	SetCmd16(b_0000,0x0000,0);	// NOP
	SetCmd16(b_0000,0x0000,0);	// NOP
	wait_ms(300);
}
/*********************************************************************
 *	バルク・イレース.
 *********************************************************************
 */
void BulkErase(int cmd80)
{
	switch(cmd80) {
	 default:
	 case Command80  :
		EraseCmd(0x0000,0x8080);break;

	 case Command81  :
		EraseCmd(0x0f0f,0x8f8f);break;	// mod by senshu

	 case Command3F8F:
		EraseCmd(0x3f3f,0x8f8f);break;

	}
}
/*********************************************************************
 *	書き込みテスト.
 *********************************************************************
 */
#define Flash_Wait	(2)		/* 安全のため */
void WriteFlash(int adr,uchar *buf,int wlen)
{
	int data;

	SetCmd16(b_0000,0x8EA6,0);	//BSF   EECON1, EEPGD
	SetCmd16(b_0000,0x9CA6,0); //BCF   EECON1, CFGS
	SetCmd16(b_0000,0x84A6,0); //BSF   EECON1, WREN
	SetAddress24(adr);

#if	0
	int i;
	for(i=0;i<wlen-1;i++) {
		data = buf[0]|(buf[1]<<8);buf+=2;
		SetCmd16(b_1101,data,0);
	}
#else
		SetCmd16L(b_1101,(ushort*)buf,wlen-1,0);
		buf = buf + 2*(wlen-1);
#endif
	{	//最後の１ワード書き込み.
		data = buf[0]|(buf[1]<<8);
		SetCmd16(b_1111,data,0);
		SetCmd16(b_0000,0x0000,Flash_Wait);	// NOP 1mSの待ちを確保
	}
}

#define	FW	7	// 5mSで十分だが、安全の為、7mSにした(by senshu)
/*********************************************************************
 *	書き込みテスト.
 *********************************************************************
 */
void WriteFuse(uchar *buf)
{
	int adr = 0x300000;
	int i,data;
	SetCmd16(b_0000,0x8EA6,0);	//BSF   EECON1, EEPGD
	SetCmd16(b_0000,0x8CA6,0); //BSF   EECON1, CFGS
	SetCmd16(b_0000,0x84A6,0); //BSF   EECON1, WREN
#if	1
	//不思議だ.これを入れると何回書いても ゼロ化けしない.
	SetCmd16(b_0000,0xef00,0);	// GOTO 10000H ???
	SetCmd16(b_0000,0xf800,0);
	// ↑PIC18Test.exe からコピペさせていただきました.
#endif
	SetAddress24(adr);
	for(i=0;i<7;i++) {
		data = buf[0]|(buf[1]<<8);buf+=2;//fusedata[i];

//		printf("%04x\n",data);			// for DEBUG

		SetCmd16(b_1111,data,0);		// set DATA
		SetCmd16(b_0000,0x0000,FW);	// NOP(Write)
		SetCmd16(b_0000,0x2af6,0);		// INCF TBLPTRL

		SetCmd16(b_1111,data,0);		// set DATA
		SetCmd16(b_0000,0x0000,FW);	// NOP(Write)
		SetCmd16(b_0000,0x2af6,0);		// INCF TBLPTRL
	}
#if	0
	//これは不要.
	SetCmd16(b_0000,0x94A6,0);    //BCF   EECON1, WREN
	SetCmd16(b_0000,0x9CA6,0);    //BCF   EECON1, CFGS
#endif
	wait_ms(100);
}
/*********************************************************************
 *	読み出しテストルーチン・メイン.
 *********************************************************************
 */
void ReadFlash(int adr,uchar *buf,int len,int dumpf)
{
	int i;

	if(dumpf)printf("%06x :",adr);

	GetData8L(adr,buf,len);

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
#if	0
void PicRead(char *buf,int cnt)
{
	int adr=0;
	uchar rbuf[256];
	if( strcmp(buf,"on")==0 ) {
		printf("PGM ON\n");
		PicPgm(1);return;
	}
	if( strcmp(buf,"off")==0 ) {
		printf("PGM OFF\n");
		PicPgm(0);return;
	}
	if( strcmp(buf,"erase")==0 ) {
		printf("ERASE\n");
		BulkErase();return;
	}
	if( strcmp(buf,"write")==0 ) {
		printf("WRITE\n");
//		WriteFlash();
		return;
	}
	if( strcmp(buf,"wfuse")==0 ) {
		printf("WRITE FUSE\n");
		WriteFuse();return;
	}

	if( sscanf(buf,"%x",&adr) ) {
		ReadFlash(adr,rbuf,cnt);
	}
}
#endif
/*********************************************************************
 *
 *********************************************************************
 */
