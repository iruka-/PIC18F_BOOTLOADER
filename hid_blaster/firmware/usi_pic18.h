/*------------------------------------------------------------------------
 *		PIC18F用 usi_trans()関数.
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


/*------------------------------------------------------------------------
	SPI制御レジスタをマスターモードで初期化する.
 *------------------------------------------------------------------------
	
 */
extern void timer2_init(uchar prescale,uchar period);

uchar delay1(void)
{
	static uchar i;
	i++;
	return i;
}

void SPI_MasterInit(void)
{
	SSPSTAT=0b01000000;	// SMP=0 , CKE=1;
//	SSPCON1=0b00000000;	// SCK=12MHz/1 , CKP(Clock Polarity)=0
//	SSPCON1=0b00000001;	// SCK=12MHz/16
//	SSPCON1=0b00000010;	// SCK=12MHz/64
	SSPCON1=0b00000011;	// SCK=TMR2/2

	DDR_SCK=1;			// Hi-Z
	SSPCON1bits.SSPEN=1;// Enable SSP
	DDR_SCK=0;			// OUT
}

void SPI_MasterExit(void)
{
	SSPCON1bits.SSPEN=0;// Disable SSP
}

//------------------------------------------------------------------------
//uchar SPI_MasterTransmit(char cData)
//	SPI転送を１バイト分実行する.
uchar usi_trans(uchar data)
{
	SSPBUF = data;
	while(!SSPSTATbits.BF) {};  // BF = SSP Buffer Full . 1=Full 0=Empty 
	return SSPBUF;
}


//------------------------------------------------------------------------
void ispDelay(void)
{
	uchar i;
	for(i=0;i<100;i++) ;
//	delay_10us(1);
}
//------------------------------------------------------------------------
void ispConnect(void) {
  /* all ISP pins are inputs before */

  /* reset device */
//  ISP_OUT &= ~(1 << ISP_RST);   /* RST low */
//  ISP_OUT &= ~(1 << ISP_SCK);   /* SCK low */
	ISP_RST=0;
	ISP_SCK=0;

  /* now set output pins */
//  ISP_DDR |= (1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI);
	DDR_RST=0;
	DDR_SCK=0;
	DDR_MOSI=0;
	DDR_MISO=1;


  	SPI_MasterInit();

  /* positive reset pulse > 2 SCK (target) */
  	ispDelay();	// ISP_OUT |= (1 << ISP_RST);    /* RST high */
	ISP_RST = 1;
  	ispDelay();	// ISP_OUT &= ~(1 << ISP_RST);   /* RST low */
	ISP_RST = 0;
}

//------------------------------------------------------------------------
static void ispDisconnect(void) {
  
  /* set all ISP pins inputs */
//  ISP_DDR &= ~((1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI));
	DDR_RST=1;
	DDR_SCK=1;
	DDR_MOSI=1;
  /* switch pullups off */
//  ISP_OUT &= ~((1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI));
	ISP_RST=0;
	ISP_SCK=0;
	ISP_MOSI=0;

  /* disable hardware SPI */
//  SPCR = 0;
  SPI_MasterExit();
}

static	void ispSckPulse(void)
{
//	ISP_DDR=(1<<ISP_MOSI)|(1<<ISP_SCK);  /*MOSI,SCK=出力、他は入力に設定 */
  /* disable hardware SPI */
//  SPCR = 0;
  	SPI_MasterExit();

//	ISP_OUT |= (1 << ISP_SCK);		/* SCK high */
	ISP_SCK = 1;
    ispDelay();
//	ISP_OUT &= ~(1 << ISP_SCK);		/* SCK Low */
	ISP_SCK = 0;
    ispDelay();

	SPI_MasterInit();
}

void usi_set_delay(uchar delay)
{
	timer2_interval(delay);	// '-dN'
}
