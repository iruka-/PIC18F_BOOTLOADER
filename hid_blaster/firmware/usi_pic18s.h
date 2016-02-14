/*------------------------------------------------------------------------
 *		PIC18F用 usi_trans()関数.	SOFTWARE実装.
 *------------------------------------------------------------------------
関数一覧:

 void  portInit(void)			起動時Port初期化;
 void  ispConnect()             SPI接続;
 void  ispDisconnect()          SPI開放;
 uchar usi_trans(char cData)	SPI転送1バイト実行;
 
 *------------------------------------------------------------------------
25:RC6=TX  = Rset
33:RB0=SDI = MISO
34:RB1=SCK = SCK
26:RC7=SDO = MOSI
 */

//	PORTB PIN Setting
//#define	ISP_DDR		DDRB
//#define	ISP_OUT		PORTB

//	PORTB PIN ASSIGN


#if defined(__18F14K50)
//	ポート出力データ.    
#define	ISP_SCK		LATBbits.LATB6			/* Target SCK */
#define	ISP_MISO	LATBbits.LATB4			/* Target MISO */
#define	ISP_MOSI	LATCbits.LATC7			/* Target MOSI */
#define	ISP_RST		LATCbits.LATC6			/* Target RESET */

//	方向レジスタ (0=出力)
#define	DDR_SCK		TRISBbits.TRISB6			/* Target SCK */
#define	DDR_MISO	TRISBbits.TRISB4			/* Target MISO */
#define	DDR_MOSI	TRISCbits.TRISC7			/* Target MOSI */
#define	DDR_RST		TRISCbits.TRISC6			/* Target RESET */

//	ポート読み込み.
#define	PORT_SCK	PORTBbits.RB6			/* Target SCK */
#define	PORT_MISO	PORTBbits.RB4			/* Target MISO */
#define	PORT_MOSI	PORTCbits.RC7			/* Target MOSI */
#define	PORT_RST	PORTCbits.RC6			/* Target RESET */

#else
//	18F2550/4550
//	ポート出力データ.    
#define	ISP_SCK		LATBbits.LATB1			/* Target SCK */
#define	ISP_MISO	LATBbits.LATB0			/* Target MISO */
#define	ISP_MOSI	LATCbits.LATC7			/* Target MOSI */
#define	ISP_RST		LATCbits.LATC6			/* Target RESET */

//	方向レジスタ (0=出力)
#define	DDR_SCK		TRISBbits.TRISB1			/* Target SCK */
#define	DDR_MISO	TRISBbits.TRISB0			/* Target MISO */
#define	DDR_MOSI	TRISCbits.TRISC7			/* Target MOSI */
#define	DDR_RST		TRISCbits.TRISC6			/* Target RESET */

//	ポート読み込み.
#define	PORT_SCK	PORTBbits.RB1			/* Target SCK */
#define	PORT_MISO	PORTBbits.RB0			/* Target MISO */
#define	PORT_MOSI	PORTCbits.RC7			/* Target MOSI */
#define	PORT_RST	PORTCbits.RC6			/* Target RESET */


#endif

static uchar usi_delay = 5;

//------------------------------------------------------------------------
void ispDelay(uchar d)
{
	uchar i;
	for(i=2;i<d;i++) ;
}
//------------------------------------------------------------------------
void SPI_MasterInit(void)
{
}

//------------------------------------------------------------------------
void SPI_MasterExit(void)
{
}

//------------------------------------------------------------------------
//uchar SPI_MasterTransmit(char cData)
//	SPI転送を１バイト分実行する.
//
//  SCK    _____ __~~ __~~ __~~ __~~ __~~ __~~ __~~ __~~  _____
//  MOSI   _____ < D7>< D6>< D5>< D4>< D3>< D2>< D1>< D0>

#define	MOSI_OUT(dat,bit,rd) \
	if(  dat & (1<<bit))  ISP_MOSI=1;	\
	if(!(dat & (1<<bit))) ISP_MOSI=0;	\
	ISP_SCK = 1;						\
	if(PORT_MISO) { rd |= (1<<bit); }	\
	ISP_SCK = 0;						\


//
//	1nop分のwait入りバリエーション.
//
#define	MOSI_OUT1(dat,bit,rd) \
	if(  dat & (1<<bit))  ISP_MOSI=1;	\
	if(!(dat & (1<<bit))) ISP_MOSI=0;	\
	ISP_SCK = 1;						\
	_asm nop _endasm;					\
	_asm nop _endasm;					\
	if(PORT_MISO) { rd |= (1<<bit); }	\
	ISP_SCK = 0;						\
	_asm nop _endasm;					\
	_asm nop _endasm;

//
//	最高速
//
uchar usi_trans0(uchar data)
{
	uchar r=0;
	MOSI_OUT(data,7,r);
	MOSI_OUT(data,6,r);
	MOSI_OUT(data,5,r);
	MOSI_OUT(data,4,r);
	MOSI_OUT(data,3,r);
	MOSI_OUT(data,2,r);
	MOSI_OUT(data,1,r);
	MOSI_OUT(data,0,r);
	return r;
}

//
//	1nop入り.
//
uchar usi_trans1(uchar data)
{
	uchar r=0;
	MOSI_OUT1(data,7,r);
	MOSI_OUT1(data,6,r);
	MOSI_OUT1(data,5,r);
	MOSI_OUT1(data,4,r);
	MOSI_OUT1(data,3,r);
	MOSI_OUT1(data,2,r);
	MOSI_OUT1(data,1,r);
	MOSI_OUT1(data,0,r);
	return r;
}


uchar usi_trans(uchar data)
{
	uchar i;
	uchar r;

	if(usi_delay==0) return usi_trans0(data);
	if(usi_delay==1) return usi_trans1(data);

	ISP_SCK = 0;
	for(i=0;i<8;i++) {

		if(  data & 0x80)  ISP_MOSI = 1;
		if(!(data & 0x80)) ISP_MOSI = 0;

		r += r;			// r<<=1;

		ispDelay(usi_delay);
		
		if(PORT_MISO) { r|=1;}
		ISP_SCK = 1;

		data += data;	// data <<=1;
		ispDelay(usi_delay);

		ISP_SCK = 0;
	}
	return r;
}


//------------------------------------------------------------------------
void ispConnect(void) {
  /* reset device */
	ISP_RST=0;	/* RST low */
	ISP_SCK=0;  /* SCK low */

  /* now set output pins */
	DDR_RST=0;
	DDR_SCK=0;
	DDR_MOSI=0;
	DDR_MISO=1;

  /* positive reset pulse > 2 SCK (target) */
  	ispDelay(100);	// ISP_OUT |= (1 << ISP_RST);    /* RST high */
	ISP_RST = 1;
  	ispDelay(100);	// ISP_OUT &= ~(1 << ISP_RST);   /* RST low */
	ISP_RST = 0;
}

//------------------------------------------------------------------------
static void ispDisconnect(void) {
  
  /* set all ISP pins inputs */
	DDR_RST=1;
	DDR_SCK=1;
	DDR_MOSI=1;
  /* switch pullups off */
	ISP_RST=0;
	ISP_SCK=0;
	ISP_MOSI=0;
}

//------------------------------------------------------------------------
static	void ispSckPulse(void)
{
/* SCK high */
	ISP_SCK = 1;
    ispDelay(100);
/* SCK Low */
	ISP_SCK = 0;
    ispDelay(100);
}

void usi_set_delay(uchar delay)
{
	usi_delay = delay;	// '-dN'
}
