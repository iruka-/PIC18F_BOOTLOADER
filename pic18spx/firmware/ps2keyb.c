/**********************************************************************
 *	PS2 Keyboard Library
 **********************************************************************
 *
void  kbd_init(void);			// : 初期化
uchar kbd_getchar(void);		// : ASCII コードを取得.
uchar kbd_peekcode(void);		// : FIFOキューのTOPを見る.
uchar kbd_getcode(void);		// : FIFOキューの取り出し操作.
uchar kbd_getpress(uchar c);	// : リアルタイム 任意キーの押下情報 の取得.

 */
#if	0

// AVR

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define	Near
#else

// PIC

#include "usb/typedefs.h"                   
#include "usb/usb.h"                         
#include "io_cfg.h"                     

#define	Near	near
#define	PROGMEM	rom
#define	__LPM
#endif

#include "config.h"
#include "ps2keyb.h"


#if	USE_PS2KEYBOARD		// PS/2キーボードI/F を使用する.



#pragma romdata

#if	PS2KBD_USE_GETCHAR
#include "keycode.h"
#endif

#include "keyname.h"



/**********************************************************************
 *	FIFOのサイズ.
 **********************************************************************
 */
#define	BUFF_SIZE 		32			//２のべき.
#define	BUFF_SIZE_MASK	(BUFF_SIZE-1)

#define	PREFIX_E0		0x1
#define	PREFIX_E1		0x2			// [Pause]=E1 14,77,E1 14,F0 77
#define	PREFIX_F0		0x4

#define	KBD_SHIFT		0x1



/**********************************************************************
 *	ワークエリア.
 **********************************************************************
 */
#pragma udata access accessram

static	uchar bitcount;				//	0..10 : start=0,data[8],oddparity,stop=1
static	uchar inptr, outptr;		// FIFO の取り出し、書き込みポインタ.
static  uchar kbd_data;				// Holds the received	scan code
static  uchar kbd_shift;			// Holds shift , ctrl , alt status
static  uchar prefix;				// Holds last scan code prefix
static  uchar clk_last;
static	uchar watchDog;				// 0.25秒 Watch Dog

#define	MAX_DOG	250

#pragma udata

static	uchar kbd_buffer[BUFF_SIZE];// FIFO.
#if	PS2KBD_USE_PRESSTABLE	
static	uchar kbd_stat[32];			// 128個のキーの押下情報 (bit table).
#endif


#pragma code
/**********************************************************************
 *	ps2kbd : 初期化
 **********************************************************************
 */
void kbd_init(void)
{
	//内部変数の初期化.
	inptr  =	0;
	outptr =	0;
	bitcount = 11;
	prefix =	0;
	kbd_shift =	0;
	watchDog = 0;
	clk_last = PS2KBD_CLOCK_MASK;	// 初期値 = H

#if	PS2KBD_USE_INTERRUPT		// use PCINT8

#ifdef	_AVR_CHIP_
	//ポートの初期化.
	PCICR  = 1<<1;	// PCINT1 (8..14) Enable.
	PCMSK1 = 1<<0;	// PCINT8 Mask off.
#else
	// PIC用.
	INTCON2bits.RBPU =0;	// PORTB PullUp
	INTCON2bits.INTEDG0=0;	// Falling Edge割り込み.

	INTCONbits.INT0IE=1;	// INT0割り込み許可.
#endif

#endif

}

#if	PS2KBD_USE_PRESSTABLE	
/**********************************************************************
 *	ps2kbd : 128個のキーの押下情報のmask bit生成.
 **********************************************************************
 */
static uchar get_press_mask(uchar c)
{
	return 1 << (c & 7);
}
static uchar get_press_idx(uchar c)
{
	return (c >> 3);
}

/**********************************************************************
 *	ps2kbd : 128個のキーの押下情報 のクリア.
 **********************************************************************
 */
void clr_press_table(void)
{
	uchar i;
	for(i=0;i<16;i++) {
		kbd_stat[i] = 0;
	}
}
/**********************************************************************
 *	ps2kbd : 128個のキーの押下情報 の更新.
 **********************************************************************
 */
static void set_press_table(uchar c,uchar pre)
{
	uchar idx  = get_press_idx(c);
	uchar mask = get_press_mask(c);
	if(pre & PREFIX_F0) {	//離された情報,拡張シーケンスは無視する.
		kbd_stat[idx] &= ~mask;
	}else{
		kbd_stat[idx] |=  mask;
	}
}

/**********************************************************************
 *	ps2kbd : 128個のキーの押下情報 の取得.
 **********************************************************************
 *	引数  c : スキャンコード.
 *	戻り値  : 非ゼロの場合、そのスキャンコードのキーは「押された状態」にある.
 *			  ゼロの場合、そのスキャンコードのキーは「離された状態」にある.
 */
uchar kbd_getpress(uchar c)
{
	uchar idx  = get_press_idx(c);
	uchar mask = get_press_mask(c);
	return kbd_stat[idx] & mask;
}
#else

#endif
/**********************************************************************
 *	ps2kbd : FIFOキューの操作.
 **********************************************************************
 */
static	void put_que(uchar c)
{
	uchar p = inptr;

	if(c==0) return;			// 0 はFIFOに書き込まない.

	kbd_buffer[p]=c;
	p++; p &= BUFF_SIZE_MASK;	// 仮の p を進めてみる.

	if(p != outptr) {	// 仮の p が取り出しポインタに追いついていない場合のみ,
		inptr = p;		// ポインタを更新する.
	}

#if	0	// TEST
	mLED_1_Toggle();
#endif
}

/**********************************************************************
 *	ps2kbd : FIFOキューのTOPを見る.
 **********************************************************************
 */
uchar kbd_peekcode(void)
{
	uchar p = outptr;
	if(p == inptr) {			//キューが空と判断する.
		return 0;				//入力なし.
	}
	return kbd_buffer[p];		//キューTOPを見るだけ.
}

/**********************************************************************
 *	ps2kbd : FIFOキューの取り出し操作.
 **********************************************************************
 *	キー入力がないときは、0を返す.
 *	キー入力があったときは、スキャンコード(非ゼロ)を1バイト返す.
 *注意：
 *	ASCIIコードのみを取得する場合は、kbd_getcode()を一切呼ばずに、
 *	kbd_getchar()のほうを使用する。
 *	混在して呼び出した場合、kbd_getchar()の動作がおかしくなる。
 */
uchar kbd_getcode(void)
{
	uchar c;
	uchar p = outptr;
	if(p == inptr) {			//キューが空と判断する.
		return 0;				//入力なし.
	}
	c = kbd_buffer[p];			//キューから取り出す.
	p++; p &= BUFF_SIZE_MASK;	// 仮の p を進めてみる.
	outptr = p;
	return c;
}

#if	PS2KBD_USE_GETCHAR
/**********************************************************************
 *	ps2kbd : getchar ASCII コードを取得.
 **********************************************************************
 *	キー入力がないときは、0を返す.
 *	キーブレーク(離された)前置コード(0xF0)や拡張シーケンス(0xE0)
 *	を取得した場合は、 prefixというスタティック変数にそのフラグを
 *	残すが、戻り値は 0 になる.（次のコード取得時にフラグが参照される）
 */
uchar kbd_getchar(void)
{
	uchar pre = prefix;			//一つ前のコードが拡張コードを持っている?
	uchar c = kbd_getcode();
	uchar idx;

	if(c==0) return c;			//入力無し.

	// ２バイト以上のシーケンスが来る場合は、前置コードをここで処理:
	if(c == 0xe0) {				//拡張シーケンス E0.
		prefix |= PREFIX_E0;
		return 0;
	}else if(c == 0xe1) {		//拡張シーケンス E1.
		prefix |= PREFIX_E1;
		return 0;
	}else if(c == 0xf0) {		//離された情報が後続.
		prefix |= PREFIX_F0;
		return 0;
	}

	// ２バイト以上のシーケンスなら、最後の１バイトを処理:
	// もしくは単発キーの場合もここに来る:

	prefix = 0;
	// [SHIFT] 状態のフラグ更新.
	if((c == SKEY_LSHIFT)||(c == SKEY_RSHIFT)) {
		if(pre & PREFIX_F0) {
			kbd_shift &= KBD_SHIFT;
		}else{
			kbd_shift |= KBD_SHIFT;
		}
	}
	set_press_table(c,pre);

	if(pre & PREFIX_E0) {	//拡張シーケンスはSCAN CODEのMSBを立てて区別する.
		c |= SKEY_EXTEND;
	}

	if(pre & PREFIX_F0) {	//離された情報は無視する.
		return 0;
	}

	idx = c*2;
	if(kbd_shift & KBD_SHIFT) idx++;	// [SHIFT]が押されている?

	// SCAN CODE --> ASCII CODE
	return __LPM( &keycode_table[idx] );
}
#endif


#if	PS2KBD_USE_INTERRUPT

/**********************************************************************
 *	ps2kbd : 割り込みハンドラー.
 **********************************************************************
 */

//  PIC

//  use INT0 エッジ割り込み hard handler

void 	kbd_handler(void)
{
	if(INTCONbits.INT0IF) {
#if	1
		// ノイズ対策. CLKノイズによって、ビットずれが起きてもリセットしたい.
		if(	watchDog >= MAX_DOG) {	// 最後の INT0エッジ割り込みから0.25秒経過した.
			bitcount = 11;			// 番犬発動. ビットカウンタを初期化する.
		}
		watchDog=0;					// watchDogをリセット.


		// 当然 falling edgeを仮定する.
											//	start and stop bits	are	ignored.	
											// Routine entered at	falling	edge
		if(bitcount	< 11 &&	bitcount > 2) {	//	Bit	3 to 10	is data. Parity	bit,
			kbd_data >>= 1;
			if(PS2KBD_PIN & PS2KBD_DATA_MASK) {
				kbd_data |= 0x80;			// Store	a '1'
			}
		}

		// rising edgeの処理もついでにやる.
		if(--bitcount == 0)	{				// All bits received
			put_que(kbd_data);
			bitcount = 11;
		}
#endif
		INTCONbits.INT0IF=0;
#if	1	// TEST
	mLED_2_Toggle();
#endif
	}
}

//	定時割り込み(1000Hz)も併用.

void 	kbd_int_handler(void)
{
	if( watchDog < MAX_DOG ) {
		watchDog++;
	}
#if	1	// TEST
	mLED_1_Toggle();
#endif

}

#else


/**********************************************************************
 *	PCINT8 をTIMER1 割り込みで エミュレート（代用）する.
 **********************************************************************
 */
void 	kbd_handler(void)
{
											//	start and stop bits	are	ignored.
	if(!(PS2KBD_PIN & PS2KBD_CLOCK_MASK)) {	// Routine entered at	falling	edge
		if(bitcount	< 11 &&	bitcount > 2) {	//	Bit	3 to 10	is data. Parity	bit,
			kbd_data >>= 1;
			if(PS2KBD_PIN & PS2KBD_DATA_MASK) {
				kbd_data |= 0x80;			// Store	a '1'
			}
		}
	} else {								// Routine entered at rising edge
		if(--bitcount == 0)	{				// All bits received
			put_que(kbd_data);
			bitcount = 11;
		}
	}
}


//	use TIMER1 softhandler
void 	kbd_int_handler(void)
{
	uchar clk_edge = PS2KBD_PIN & PS2KBD_CLOCK_MASK;	//現在値 KBD_CLK

	// KBD_CLKの電位に変化があったか?.(前回と異なっているか)
	if(	clk_last != clk_edge ) {
		clk_last  = clk_edge;
		kbd_handler();					// PCINT8 のエントリーをコールする.
	}
}
#endif
/**********************************************************************
 *	
 **********************************************************************
 */


#endif	//USE_PS2KEYBOARD		// PS/2キーボードI/F を使用する.
