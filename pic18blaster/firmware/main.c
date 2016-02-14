/********************************************************************
 *	メイン関数	初期化	割り込みベクターの設定.
 ********************************************************************

タイマー２割り込みの実験:

（１）TIMER2を使って 10kHz周期のインターバル割り込みを行う。
（２）割り込み処理内で10000回カウントを行い、10000回になると（１秒毎）
      LED1を反転させる.


この状態でも picmonit.exe は使用できるので、ホストＰＣからポートを触ったり
ワークを見たりすることが出来る.




 */
#include "usb/typedefs.h"
#include "usb/usb.h"
#include "io_cfg.h"
#include "timer2.h"
#include "monit.h"
#include "config.h"
#include "ps2keyb.h"

//	FUSE設定.
#include "fuse-config.h"

//	選択スイッチ.
//---------------------------------------------------------------------
#define	TIMER2_INTERRUPT	0		// タイマー２割り込みを使用する.
#define	TIMER2_INT_BLINK	0		// タイマー２割り込みでLEDブリンク.
//---------------------------------------------------------------------

void YourHighPriorityISRCode();
void YourLowPriorityISRCode();
void timer2_int_handler(void);

/**	VECTOR REMAPPING ***********************************************/
#if	0
	#define	REMAPPED_RESET_VECTOR_ADDRESS			0x1000
	#define	REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1008
	#define	REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1018
#endif

#if	1
	#define	REMAPPED_RESET_VECTOR_ADDRESS			0x800
	#define	REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x808
	#define	REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x818
#endif

#if	0
	#define	REMAPPED_RESET_VECTOR_ADDRESS			0x00
	#define	REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x08
	#define	REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x18
#endif

/********************************************************************
 *	0x800～のジャンプベクターを設定する.
 ********************************************************************
 *	0x800 goto _startup
 *	0x808 goto YourHighPriorityISRCode
 *	0x818 goto YourLowPriorityISRCode
 */

extern void	_startup (void);		// See c018i.c in your C18 compiler	dir
#pragma	code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS
void _reset	(void)
{
	_asm goto _startup _endasm
}


#pragma	code REMAPPED_HIGH_INTERRUPT_VECTOR	= REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS
void Remapped_High_ISR (void)
{
	 _asm goto YourHighPriorityISRCode _endasm
}


#pragma	code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS
void Remapped_Low_ISR (void)
{
	 _asm goto YourLowPriorityISRCode _endasm
}

/********************************************************************
 *	0x008～のジャンプベクターを設定する.
 ********************************************************************
 *	0x008 goto 0x808
 *	0x018 goto 0x818
 */
#pragma	code HIGH_INTERRUPT_VECTOR = 0x08
void High_ISR (void)
{
	 _asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
}


#pragma	code LOW_INTERRUPT_VECTOR =	0x18
void Low_ISR (void)
{
	 _asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
}

/********************************************************************
 *	ジャンプベクター設定はここまで.
 ********************************************************************
 */


#pragma	code
/********************************************************************
 *	High割り込み処理関数.
 ********************************************************************
 */
#pragma	interrupt YourHighPriorityISRCode
void YourHighPriorityISRCode()
{
	#if	defined(USB_INTERRUPT)
		USBDeviceTasks();
	#endif

#if	USE_PS2KEYBOARD		// PS/2キーボードI/F を使用する.
	#if	PS2KBD_USE_INTERRUPT
		// INT0割り込みは Highプライオリティのみ.
		kbd_handler();
	#endif
#endif
}	//This return will be a	"retfie fast", since this is in	a #pragma interrupt	section

/********************************************************************
 *	Low	割り込み処理関数.
 ********************************************************************
 */
//#pragma	interruptlow YourLowPriorityISRCode
#pragma	interruptlow YourLowPriorityISRCode nosave=TBLPTR,TBLPTRU,TABLAT,PCLATH,PCLATU,PROD,section("MATH_DATA")
void YourLowPriorityISRCode()
{
	//Check	which interrupt	flag caused	the	interrupt.
	//Service the interrupt
	//Clear	the	interrupt flag
	//Etc.

	#if	TIMER2_INTERRUPT
		timer2_int_handler();
	#endif

}	//This return will be a	"retfie", since	this is	in a #pragma interruptlow section




#pragma	code

/**********************************************************************
 *	timer : 割り込みハンドラー.
 **********************************************************************
 */
void timer2_int_handler(void)
{
    if(	PIR1bits.TMR2IF){		// タイマ2割り込み？
#if	TIMER2_INT_SAMPLE			// タイマー２割り込みでPORTサンプル.
		mon_int_handler();
#endif

#if	TIMER2_INT_BLINK			// タイマー２割り込みでLEDブリンク.
		static ushort cnt=0;
		cnt++;
		if(cnt >= 1000) {
			cnt = 0;
			mLED_1 = !mLED_1;	// LEDを反転.
		}
#endif

#if	USE_PS2KEYBOARD		// PS/2キーボードI/F を使用する.
		kbd_int_handler();
#endif
        PIR1bits.TMR2IF=0;		// タイマ2割り込みフラグを0にする
	}
}

/**********************************************************************
 *	LED2 blink (生存反応)
 **********************************************************************
 */
void LED2_blink(void)
{
	static	ushort cnt=0;
	cnt++;
	if(cnt & 0x8000) mLED_2 = 1;
	else             mLED_2 = 0;
}

/********************************************************************
 *	初期化関数
 ********************************************************************
 */
static void	InitializeSystem(void)
{
#if RAM_SERIAL
	extern void set_serial_number(void);

	set_serial_number();
#endif
#if defined(__18F14K50)
	// 入力ピンをデジタルモードにする.
	ANSEL=0;
	ANSELH=0;
#endif
    ADCON1 = 0x0F;			//Need to make sure RB4 can be used as a digital input pin

#if	1
	// HIDaspx , PICwriter用 portb,portcを全入力にする.
	TRISB = 0xFF;
	TRISC = 0xFF;
#endif

#if	defined(USE_USB_BUS_SENSE_IO)
	tris_usb_bus_sense = INPUT_PIN;
#endif

#if	defined(USE_SELF_POWER_SENSE_IO)
	tris_self_power	= INPUT_PIN;
#endif
	mInitializeUSBDriver();
	UserInit();

#if	USE_PS2KEYBOARD		// PS/2キーボードI/F を使用する.
	kbd_init();
#endif

#if	TIMER2_INTERRUPT

//	timer2_init(0x80 |(15<<3)| 2,255);	// 割り込みON,postscale 1/16,prescale 1/16,1/256 = 183.10 H
//	timer2_init(0x80 |(14<<3)| 2,249);	// 割り込みON,postscale 1/15,prescale 1/16,1/250 = 200Hz
//	timer2_init(0x80 |(14<<3)| 2, 49);	// 割り込みON,postscale 1/15,prescale 1/16,1/50 = 1000Hz
	timer2_init(0x80 |(14<<3)| 2,  4);	// 割り込みON,postscale 1/15,prescale 1/16,1/5  =  10kHz

	// 割り込み許可.
	InitTimer2InterruptLow();			// Timer2割り込みを low Priorityに設定する.

#else

	INTCONbits.GIEL = 0; 		// Low  Priority 割り込みを禁止.
	INTCONbits.GIEH = 0; 		// High Priority 割り込みを禁止.
#endif
}

void USBtask(void)
{
	USBCheckBusStatus();					// Must	use	polling	method
	USBDriverService();						// Interrupt or	polling	method
	if((usb_device_state ==	CONFIGURED_STATE) && (UCONbits.SUSPND != 1)) {
		ProcessIO();
	}
}
/********************************************************************
 *	メイン関数
 ********************************************************************
 */
void main(void)
{
	InitializeSystem();
	while(1){
		USBtask();
//		LED2_blink();
	}
}
/********************************************************************
 *
 ********************************************************************
 */
