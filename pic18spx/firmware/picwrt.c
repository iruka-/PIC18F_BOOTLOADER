/********************************************************************
 *	簡易モニタ
 ********************************************************************
 */
#include "usb/typedefs.h"
#include "usb/usb.h"
#include "io_cfg.h"
#include "monit.h"
#include "hidcmd.h"
#include "picwrt.h"

/********************************************************************
 *	
 ********************************************************************
	// PICwriter専用.
	struct{
		uchar  piccmd;
		uchar  picsize;
		uchar  picadrl;
		uchar  picadrh;
		uchar  picadru;
		uchar  piccmd4;
		uchar  picms;
		uchar  picdata[32];
	};
 */
enum CMD_4 {
	b_0000 = 0x00,
	b_0001 = 0x01,
	b_1000 = 0x08,
	b_1001 = 0x09,
	b_1100 = 0x0c,	//
	b_1101 = 0x0d,	// TBLPTR+=2
	b_1111 = 0x0f,	// Write!
};


#if defined(__18F14K50)

//	配線は２種類あるので注意.
#if	ALTERNATE_PGxPIN_ASSIGN		// PGC,PGD,PGMの結線を入れ替えます.
//	ポート出力データ.
#define	PGM	 		LATBbits.LATB6		// RB6 (AVR-SCK) = PGM
#define	PGC	 		LATBbits.LATB4		// RB4 (AVR-MISO)= PGC
#define	PGD	 		LATCbits.LATC7		// RC7 (AVR-MOSI)= PGD
#define	MCLR 		LATCbits.LATC6		// RC6 (AVR-RST) = MCLR

//	ポート読み込み.
#define	inPGM	 	PORTBbits.RB6
#define	inPGC	 	PORTBbits.RB4
#define	inPGD	 	PORTCbits.RC7
#define	inMCLR 		PORTCbits.RC6

//	方向レジスタ (0=出力)
#define	dirPGM	 	TRISBbits.TRISB6
#define	dirPGC	 	TRISBbits.TRISB4
#define	dirPGD	 	TRISCbits.TRISC7
#define	dirMCLR 	TRISCbits.TRISC6

#else

// PGC,PGD,PGMの結線を入れ替えません.

//	ポート出力データ.    
#define	PGC	 		LATBbits.LATB6		// RB6 (AVR-SCK) = PGC
#define	PGD	 		LATBbits.LATB4		// RB4 (AVR-MISO)= PGD
#define	PGM	 		LATCbits.LATC7		// RC7 (AVR-MOSI)= PGM
#define	MCLR 		LATCbits.LATC6		// RC6 (AVR-RST) = MCLR

//	ポート読み込み.
#define	inPGC	 	PORTBbits.RB6
#define	inPGD	 	PORTBbits.RB4
#define	inPGM	 	PORTCbits.RC7
#define	inMCLR 		PORTCbits.RC6

//	方向レジスタ (0=出力)
#define	dirPGC	 	TRISBbits.TRISB6
#define	dirPGD	 	TRISBbits.TRISB4
#define	dirPGM	 	TRISCbits.TRISC7
#define	dirMCLR 	TRISCbits.TRISC6
#endif


#else
//	18F2550/4550

//	ポート出力データ.    
#define	PGC	 		LATBbits.LATB1		// RB1
#define	PGD	 		LATBbits.LATB0		// RB0
#define	PGM	 		LATCbits.LATC7		// RC7
#define	MCLR 		LATCbits.LATC6		// RC6

//	ポート読み込み.
#define	inPGC	 	PORTBbits.RB1
#define	inPGD	 	PORTBbits.RB0
#define	inPGM	 	PORTCbits.RC7
#define	inMCLR 		PORTCbits.RC6

//	方向レジスタ (0=出力)
#define	dirPGC	 	TRISBbits.TRISB1
#define	dirPGD	 	TRISBbits.TRISB0
#define	dirPGM	 	TRISCbits.TRISC7
#define	dirMCLR 	TRISCbits.TRISC6


#endif

/********************************************************************
 *	データ
 ********************************************************************
 */
//#pragma udata access accessram

//	コマンドの返信が必要なら1をセットする.
extern	uchar near ToPcRdy;

//#pragma udata SomeSectionName1
extern	Packet PacketFromPC;			//入力パケット 64byte

#if defined(__18F14K50)
extern	near Packet PacketToPC;				//出力パケット 64byte
#else
//#pragma udata SomeSectionName2
extern	Packet PacketToPC;				//出力パケット 64byte
#endif

#define	Cmd0	PacketFromPC.cmd
#define	set_bit(Port_,val_) \
				Port_ = val_ 

/********************************************************************
 *	24bitアドレスを8bit ３個に分解するunion.
 ********************************************************************
 */
typedef	union {
	dword adr24;
	uchar adr8[4];
} ADR24;

typedef	union {
	ushort adr16;
	uchar  adr8[2];
} ADR16;


/********************************************************************
 *	ISPモジュール インクルード.
 ********************************************************************
 */
#pragma code

/*********************************************************************
 *	１μ秒単位の時間待ち.
 *********************************************************************
 *	Windows側ではUSBフレーム時間の消費が発生するので、実際には不要.
 *	マイコンファーム側に移植するときはマイコン側でそれぞれ実装の必要がある.
 */
static	void wait_us(uchar us)
{
	do {
		_asm 
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		_endasm;
	} while(--us);
}
//static	
void wait_ms(uchar ms)
{
	do {
		wait_us(250);
		wait_us(250);
		wait_us(250);
		wait_us(250);
	}while(--ms);
}
//
//	0  ～200   : 0～200マイクロ秒待つ.
//	201～250   : 1～ 50ミリ秒待つ.
//
//
static	void wait_ums(uchar us)
{
	if(us) {
		if(us>200) 	wait_ms(us-200);
		else 		wait_us(us);
	}
}


/*********************************************************************
 *	1bit送信.
 *********************************************************************
        |50n|50n|
        +---+
  PGC --+   +---+

  PGD [ b=0ならL ]
        b=1ならH
 */
#if	0
inline void SetCmdb1(uchar b)
{
	if(b)	set_bit(PGD,1);
	else	set_bit(PGD,0);
	wait_us(1);
	set_bit(PGC,1);wait_us(1);
	set_bit(PGC,0);wait_us(1);
}

void SetCmdb1Long(uchar b,uchar ms)
{
	if(b)	set_bit(PGD,1);
	else	set_bit(PGD,0);

//	wait_us(1);
	set_bit(PGC,1);

	if(ms) wait_ms(ms);
	wait_us(1);

	set_bit(PGC,0);

//	wait_us(1);
}
#endif

/*********************************************************************
 *	1bit受信.
 *********************************************************************
        +---+
  PGC --+   +---

           ↑ ここで読む.

  PGD [ 入力    ]
 */
#if	0
inline uchar GetCmdb1(void)
{
	uchar b=0;
	set_bit(PGC,1);
	wait_us(1);
//	b = get_port(Pinb) & (1<<PGD);
	if(inPGD) b=1;
	set_bit(PGC,0);
	wait_us(1);
	return b;
}
#endif
/*********************************************************************
 *	n bit 送信.
 *********************************************************************
 */
void SetCmdN(ushort cmdn,uchar len)
{
	uchar i,b;
	for(i=0;i<len;i++) {
//		b = cmdn & 1;
//		SetCmdb1(b);
//		cmdn >>=1;
		{
			if(cmdn&1)	set_bit(PGD,1);
			else		set_bit(PGD,0);
			set_bit(PGC,1);
			cmdn >>=1;
			set_bit(PGC,0);
		}
	}
}

/*********************************************************************
 *	4 bit 送信 , 最後の PGC_H時間を指定.
 *********************************************************************
 */
void SetCmdNLong(uchar cmdn,uchar len,uchar ms)
{
	uchar i;	//,b;
	for(i=0;i<(len-1);i++) {
//		b = cmdn & 1;
//		SetCmdb1(b);
//		cmdn >>=1;
		{
			if(cmdn&1)	set_bit(PGD,1);
			else		set_bit(PGD,0);
			set_bit(PGC,1);
			cmdn >>=1;
			set_bit(PGC,0);
		}
	}
	{
//		SetCmdb1Long(cmdn & 1,ms);
		{
			if(cmdn&1)	set_bit(PGD,1);
			else		set_bit(PGD,0);
			set_bit(PGC,1);
			if(ms) wait_ms(ms);
			cmdn >>=1;
			set_bit(PGC,0);
		}
	}
}

/*********************************************************************
 *	PGDビットの入出力方向を切り替える.(0=入力)
 *********************************************************************
 *	f=0 ... PGD=in
 *	f=1 ... PGD=out
 */

void SetPGDDir(uchar f)
{
#if	0
	int dirb = (1<<PGC) | (1<<PGM) | (1<<MCLR);
	if(f) dirb |= (1<<PGD);
	set_port(Ddrb ,dirb);
#else

//	PIC
	if(f) {
		dirPGD=0;	// out
	}else{
		dirPGD=1;	// in
	}

/*	int dirc = (1<<PGM) | (1<<MCLR);

	int dirb = (1<<PGC) ;
	if(f) dirb |= (1<<PGD);

	set_port(Ddrb ,dirb ^ 0xff);
	set_port(Ddrc ,dirc ^ 0xff);
 */

#endif
}


/*********************************************************************
 *	4bitのコマンド と 16bitのデータを送信.
 *********************************************************************
 */
void SetCmd16(uchar cmd4,ushort data16,uchar ms)
{
	SetCmdNLong(cmd4,4,ms);
	SetCmdN(data16,16);
}

/*********************************************************************
 *	TBLPTR[U,H,L] のどれかに8bit値をセットする.
 *********************************************************************
 */
static	void SetAddress8x(uchar adr,uchar inst)
{
	SetCmd16(b_0000,0x0e00 |  adr,0);		// 0E xx  : MOVLW xx
	SetCmd16(b_0000,0x6e00 | inst,0);		// 6E Fx  : MOVWF TBLPTR[U,H,L]
}
/*********************************************************************
 *	TBLPTR に24bitアドレスをセットする.
 *********************************************************************
 */
void setaddress24(void)
{
	SetAddress8x(PacketFromPC.picadru,0xf8);	// TBLPTRU
	SetAddress8x(PacketFromPC.picadrh,0xf7);	// TBLPTRH
	SetAddress8x(PacketFromPC.picadrl,0xf6);	// TBLPTRL
}

/*********************************************************************
 *	TBLPTR から１バイト読み出し. TBLPTRはポスト・インクリメントされる.
 *********************************************************************
 */
static	uchar GetData8(void)
{
	uchar i,data8=0;

	SetCmdN(b_1001,4);
	SetCmdN(0 , 8);

//	return pic18_GetData8b();
/*********************************************************************
 *	8 bit 受信.	 LSBファースト.
 *********************************************************************
static	uchar pic18_GetData8b(void)
{
 */

	SetPGDDir(0);		// PGD=in
//	for(i=0;i<8;i++,mask<<=1) {
//		if( GetCmdb1() ) {
//			data8 |= mask;
//		}
	i=8;do {
		set_bit(PGC,1);
		data8>>=1;
		if(inPGD) {
			data8 |= 0x80;
		}
		set_bit(PGC,0);
	}while(--i);
	SetPGDDir(1);		// PGD=out
	return data8;
}


#if	0
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
#endif


#if	0
/*********************************************************************
 *	4bitのコマンド と 16bitのデータを送信.
 *********************************************************************
 */
void SetCmd16L(uchar cmd4,ushort *buf,uchar len,uchar ms)
{
	uchar i;
	ushort data16;
	for(i=0;i<len;i++) {
		data16 = *buf++;
		SetCmdNLong(cmd4,4,ms);
		SetCmdN(data16,16);
	}
}
#endif
/********************************************************************
 *	16bitデータ列を(size)ワード分書き込む.
 ********************************************************************
 *	cmd4 は、各16bitデータを書き込む前に与える4bitコマンド.
 *	ms   は、4bitコマンドの最終bit送出後に何ミリ秒待つべきかの値.
 *	ms=0 のときは 1uS 待つ.
 */
void pic_setcmd16L(void)
{
	uchar len = PacketFromPC.picsize;
	uchar cmd4= PacketFromPC.piccmd4;
	uchar ms  = PacketFromPC.picms;
	uchar i;
	ushort *s = (ushort *) PacketFromPC.picdata;
	ushort data16;
	for(i=0;i<len;i++) {
		data16 = *s++;
		SetCmdNLong(cmd4,4,ms);
		SetCmdN(data16,16);
	}
}
/********************************************************************
 *	24bitアドレスをセットして、 8bitデータ列を(size)バイト分取得する.
 ********************************************************************
 */
void pic_getdata8L(void)
{
	uchar i,len;

	setaddress24();

	len =PacketFromPC.picsize;
	for(i=0;i<len;i++) {
		PacketToPC.raw[i] = GetData8();
	}
}
/********************************************************************
 *	PICプログラムモード
 ********************************************************************
 */
#if	0
void pic_setpgm(void)
{
	uchar f=PacketFromPC.picsize;

	if(f) {
		//全ての信号をLにする.
		set_bit(PGM,0);
		set_bit(PGD,0);
		set_bit(PGC,0);
		set_bit(MCLR,0);

	dirPGM=0;
	dirPGC=0;
	dirMCLR=0;

	SetPGDDir(1);		// PGD=out

		wait_ms(1);
		set_bit(PGM,1);	// PGM=H
		wait_ms(1);
		set_bit(MCLR,1);// MCLR=H
		wait_ms(1);
	}else{
		set_bit(PGD,0);// MCLR=L
		set_bit(PGC,0);// MCLR=L
		wait_ms(1);
		set_bit(MCLR,0);// MCLR=L
		wait_ms(1);
		set_bit(PGM,0);	// PGM=L
		wait_ms(1);
		//全ての信号をLにする.
		set_bit(PGM,0);
		set_bit(PGD,0);
		set_bit(PGC,0);
		set_bit(MCLR,0);

	dirPGM=1;
	dirPGC=1;
	dirMCLR=1;

	SetPGDDir(0);		// PGD=out
	}
}
#endif
/********************************************************************
 *	PIC24プログラムモード
 ********************************************************************
 */
void pic_bitbang(void)
{
	uchar cnt =PacketFromPC.raw[1];
	uchar wait=PacketFromPC.raw[2];
	uchar *p =&PacketFromPC.raw[3];
	uchar *t =&PacketToPC.raw[0];
	uchar c;
	while(cnt) {
		c = *p++;
//	ポート出力データ.    
		if( (c & 0x08)) MCLR = 1;
		if(!(c & 0x08)) MCLR = 0;
		if( (c & 0x04)) PGM = 1;
		if(!(c & 0x04)) PGM = 0;
		if( (c & 0x02)) PGD = 1;
		if(!(c & 0x02)) PGD = 0;
		if( (c & 0x01)) PGC = 1;
		if(!(c & 0x01)) PGC = 0;

//	方向レジスタ (0=出力)
		if( (c & 0x80)) dirMCLR = 1;
		if(!(c & 0x80)) dirMCLR = 0;
		if( (c & 0x40)) dirPGM = 1;
		if(!(c & 0x40)) dirPGM = 0;
		if( (c & 0x20)) dirPGD = 1;
		if(!(c & 0x20)) dirPGD = 0;
		if( (c & 0x10)) dirPGC = 1;
		if(!(c & 0x10)) dirPGC = 0;
		
		if(Cmd0==PICSPX_BITREAD) {
			c = 0;
			if(inMCLR)c|=0x08;
			if(inPGM) c|=0x04;
			if(inPGD) c|=0x02;
			if(inPGC) c|=0x01;
			*t++ = c;
		}
		wait_ums(wait);
		cnt--;
	}
}
#if	SUPPORT_PIC24F	

/*********************************************************************
 *	１クロック分の PGD 送信
 *********************************************************************

   PGD <データ1bit>
   
   PGC _____|~~~~~~
 */
/********************************************************************
 *	PIC24F:任意bit(最大8bit)のPGDデータを送る.
 ********************************************************************
 */
void add_data(uchar c,uchar bits)
{
	uchar b;
	for(b=0;b<bits;b++) {	// LSBから bits ビット送る.
		PGC = 0;
		if( (c & 1)) PGD = 1;
		if(!(c & 1)) PGD = 0;
		c>>=1;
		PGC = 1;
	}
}
/********************************************************************
 *	PIC24F: 4bit分のPGDデータを送る.
 ********************************************************************
 */
void add_data4(uchar c)
{
	add_data(c,4);
}
/********************************************************************
 *	PIC24F: 8bit分のPGDデータを送る.
 ********************************************************************
 */
void add_data8(uchar c)
{
	add_data(c,8);
}
#if	0
/********************************************************************
 *	PIC24F: 3byte分のPGDデータを送る.
 ********************************************************************
 */
void add_data24(uchar u,uchar h,uchar l)
{
	add_data4(b_0000);	// send SIX command '0000'
	add_data8(l);
	add_data8(h);
	add_data8(u);
	PGC = 0;
}
#endif
/********************************************************************
 *	PIC24F: 24bit分のPGDデータを送る.
 ********************************************************************
 */
void sendData24(dword c)
{
	ADR24 adr;
	adr.adr24 = c;
	add_data4(b_0000);	// send SIX command '0000'
	add_data8(adr.adr8[0]);	// l
	add_data8(adr.adr8[1]);	// h
	add_data8(adr.adr8[2]);	// u
	PGC = 0;
}
void send_nop1()
{
	sendData24(0x000000);				// NOP
}
void send_nop2()
{
	send_nop1();
	send_nop1();
}
void send_goto200()
{
	sendData24(0x040200);				// GOTO 0x200
	send_nop1();
}

#define	W6	6
#define	W7	7

/********************************************************************
 *	PIC24F: レジスタペア (W0,reg) にホストPC指定のアドレス24bitを設定
 ********************************************************************
 *	ホストPC側で、あらかじめアドレスを 1/2にしておくこと(device adr化).
 */
void send_addr24(uchar reg)
{
	ADR24 adr;

	adr.adr8[0] = PacketFromPC.picadru;
	adr.adr8[1] = 0;
	adr.adr8[2] = 0;
	adr.adr8[3] = 0;

	sendData24(0x200000| ( adr.adr24 <<4) );	// MOV  #adr_h, W0
	sendData24(0x880190);				// MOV  W0, TBLPAG

	adr.adr8[0] = PacketFromPC.picadrl;
	adr.adr8[1] = PacketFromPC.picadrh;

	sendData24(0x200000|reg| ( adr.adr24 <<4) );	// MOV  #adr_l, W6
}

/*********************************************************************
 *	共通ルーチン・アドレスセットアップ
 *********************************************************************
 */
static	void setup_addr24()		//int adr)
{
	send_nop1();
	send_goto200();
	send_addr24(W6);
	sendData24(0x207847);				// MOV  #VLSI, W7
	send_nop1();
}
static	void setup_addr24write(void)	//int adr,int nvmcon)
{
	send_nop1();
	send_goto200();
	sendData24(0x24001A);				// MOV  #0x4001, W10
	sendData24(0x883B0A);				// MOV  W10, NVMCON
	send_addr24(W7);
}
static	void bset_nvmcon(void)
{
	send_nop2();
	sendData24(0xA8E761);					// BSET NVMCON
	send_nop2();
}
/*********************************************************************
 *	8bitデータの受信
 *********************************************************************
   
   PGC _____|~~~~~~

   PGD <データ1bit>
			 ^
			 |
		サンプリングpoint
 */
static	uchar pic24_getData8(void)
{
	uchar c=0;
	uchar b;

	PGC = 0;
	for(b=0;b<8;b++) {	// LSB から 8 ビット受け取る.
		
		PGC = 1;
		c>>=1;
		if(inPGD) {
			c|=0x80;
		}
		PGC = 0;
	}
	return c;
}
/*********************************************************************
 *	16bitデータの受信
 *********************************************************************
 */
static	int	pic24_getData16(void)
{
	ADR16 adr;
	adr.adr8[0]=pic24_getData8();
	adr.adr8[1]=pic24_getData8();
	return adr.adr16;
}
/*********************************************************************
 *	16bitデータの受信
 *********************************************************************
 */
int	recvData16(void)
{
	int rc;
	add_data4(b_0001);	// send SIX command '0000'
	add_data8(0x00  );	// send 1 byte '0000_0000'
	PGC=0;
	dirPGD=1;			// INPUT
//	addPGDdir(PGD_INPUT);
	//printf("\n\n==recvData16==\n");
	//flushBitBang();
	
	rc = pic24_getData16();
	
	dirPGD=0;			// OUTPUT
	//addPGDdir(PGD_OUTPUT);
	//sendData24(0x000000);
	send_nop1();
	return rc;
}


static	void WaitComplete(void)
{
	int r;
	bset_nvmcon();
	do {
		send_goto200();
		sendData24(0x803B00);	// MOV NVMCON, W0
		sendData24(0x883C20);	// MOV W0, VLSI
		send_nop1();
		r = recvData16();
	} while (r & 0x8000);
}
/********************************************************************
 *	PIC24F:Flash書き込み
 ********************************************************************
 */
void pic_write24f(void)
{
	uchar i;
	uchar cmd4=PacketFromPC.piccmd4;	// 0=address setupする. 1=継続書き込み 2=complete処理あり.
	uchar len =PacketFromPC.size;		// ループ回数. ＝データbyte数 / 6 と同じ.
	ushort *word=(ushort *)PacketFromPC.picdata;

	if(cmd4==0) {
		setup_addr24write();	//(adr>>1,0x4001);
	}

	for(i=0;i<len;i++) {
//		pack3word(t,&pd);	t+=8

		sendData24(0xEB0300);					// CLR W6
		sendData24(0x200000| ((dword)word[0]<<4) );	// MOV  #w0, W0
		sendData24(0x200001| ((dword)word[1]<<4) );	// MOV  #w1, W1
		sendData24(0x200002| ((dword)word[2]<<4) );	// MOV  #w2, W2
		sendData24(0xBB0BB6);					// TBLWTL [W6++], [W7]
		send_nop2();
		sendData24(0xBBDBB6);					// TBLWTH.B [W6++], [W7++]
		send_nop2();
		sendData24(0xBBEBB6);					// TBLWTH.B [W6++], [++W7]
		send_nop2();
		sendData24(0xBB1BB6);					// TBLWTL [W6++], [W7++]
		send_nop2();
		word+=3;
	}
	
	if(cmd4==2) {
		WaitComplete();
		send_goto200();
	}
}
/********************************************************************
 *	PIC24F:Flash読み出し
 ********************************************************************
 */
void pic_read24f(void)
{
	uchar i;
	uchar len=PacketFromPC.size;
	ushort *word=(ushort *)PacketToPC.raw;

	setup_addr24();			//(adr>>1);

	for(i=0;i<len;i++) {
		sendData24(0xBA0B96);				// TBLRDL [W6], [W7]
		send_nop2();
		word[0] = recvData16();

		sendData24(0xBADBB6);				// TBLRDH.B [W6++], [W7++]
		send_nop2();
		sendData24(0xBAD3D6);				// TBLRDH.B [++W6], [W7--]
		send_nop2();
		word[1] = recvData16();

		sendData24(0xBA0BB6);				// TBLRDL [W6++], [W7]
		send_nop2();
		word[2] = recvData16();

		word +=  3;
	}
}

#endif


/********************************************************************
 *	PICライター系コマンド受信と実行.
 ********************************************************************
 *	Cmd0 : 0x10～0x1F
 */
void cmd_picspx(void)
{
	if(		Cmd0==PICSPX_SETCMD16L) {pic_setcmd16L();}
	else if(Cmd0==PICSPX_SETADRS24) {setaddress24();}
	else if(Cmd0==PICSPX_GETDATA8L) {pic_getdata8L();}
	else if(Cmd0==PICSPX_BITBANG) 	{pic_bitbang();}
	else if(Cmd0==PICSPX_BITREAD) 	{pic_bitbang();}
#if	SUPPORT_PIC24F
	else if(Cmd0==PICSPX_WRITE24F) 	{pic_write24f();}
	else if(Cmd0==PICSPX_READ24F) 	{pic_read24f();}
#endif
//	else if(Cmd0==PICSPX_SETPGM) 	{pic_setpgm();}

	ToPcRdy = Cmd0 & 1;	// LSBがOnなら返答が必要.
}



/********************************************************************
 *	64バイトを最速でmemcpy
 ********************************************************************
 */
void memcpy64()		//(char *t,char *s)
{
	_asm
		movff  FSR2L, POSTINC1
		movff  FSR2H, POSTINC1

		// FSR2=t;
		movlw  0xfa
		movff  PLUSW1, FSR2L
		movlw  0xfb
		movff  PLUSW1, FSR2H

		// FSR0=s;
		movlw  0xfc
		movff  PLUSW1, FSR0L
		movlw  0xfd
		movff  PLUSW1, FSR0H

		movlw  4

memcpy64_loop:	// 16byte copy
		//		  *t++ = *s++  x 16
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0

		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0

		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0

		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0
		movff     POSTINC2, POSTINC0

		decfsz	WREG,1,0
		bra		memcpy64_loop


		movlw  0xff
		movff  PLUSW1, FSR2H
		movf   POSTDEC1,0,0
		movlw  0xff
		movff  PLUSW1, FSR2L
		movf   POSTDEC1,0,0
	_endasm
}
