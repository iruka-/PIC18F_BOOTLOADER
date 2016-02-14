/********************************************************************
 *	タイマー０
 ********************************************************************
 */
#include "usb/typedefs.h" 
#include "usb/usb.h"      
#include "io_cfg.h"       

#include <timers.h>       

#include "timer0.h"       

volatile int   timer_count;			//起動してからの時間.(tick)
volatile int   timer_count_high;	//起動してからの時間.(tick/65536)

volatile uchar timer_tick;			//タイマー割り込み発生フラグ.

#define	TIMER_DA_OUT	0			//タイマー割り込みのタイミングでD/A出力を有効化.

#if		TIMER_DA_OUT				//タイマー割り込みのタイミングでD/A出力を有効化.
static	 uchar timer_daout;			//タイマー割り込みのタイミングでD/A出力.
#endif


void kbd_int_handler(void);

/**********************************************************************
 *	初期化.
 **********************************************************************
 */
void timer0_init(int cnt)
{
	timer_count = 0;

//    OpenTimer0(TIMER_INT_ON & T0_8BIT & T0_SOURCE_INT & T0_PS_1_256);
    OpenTimer0(TIMER_INT_ON & T0_8BIT & T0_SOURCE_INT & T0_PS_1_1);	//46kHz
                                // タイマ0の設定, 8ビットモード, 割込み使う 
                                //内部クロック、1:256プリスケーラ
	//**** 割込みの許可
    INTCONbits.GIE=1;           // 割り込みをイネーブルにする
}

/**********************************************************************
 *	次に割り込むまで待つ.
 **********************************************************************
 */
uchar timer0_wait(void)
{
	while(timer_tick == 0) { /* nothing */ };
	timer_tick --;
	return timer_tick;
}
/**********************************************************************
 *
 **********************************************************************
 */
int timer0_gettime(void)
{
	return timer_count;
}
/**********************************************************************
 *	timer : 割り込みハンドラー.
 **********************************************************************
 */
void timer0_int_handler(void)
{
    if(	INTCONbits.T0IF){		// タイマ0割り込み？
        INTCONbits.T0IF=0;		// タイマ0割り込みフラグを0にする

		timer_count ++;
		if(	timer_count==0) {
			timer_count_high++;
		}
		timer_tick ++;
	}

#if	PS2KBD_USE_INTERRUPT
//  use PCINT8 hardhandler
#else
//	use TIMER1 softhandler
	kbd_int_handler();
#endif

}
/**********************************************************************
 *	
 **********************************************************************
 */
#if		TIMER_DA_OUT				//タイマー割り込みのタイミングでD/A出力を有効化.
void sound_out(uchar x)
{
	timer_daout = x;
}
#endif
/**********************************************************************
 *	
 **********************************************************************
 */


