/********************************************************************
 *	64バイトを最速でmemcpyする.
 *  void memcpy64(char *t,char *s)
 ********************************************************************
 *	１ｍ秒単位の時間待ち. （最大255ｍ秒まで）
 *	void wait_ms(uchar ms)
 ********************************************************************
 *	１μ秒単位の時間待ち.（最大255μ秒まで）
 *	void wait_us(uchar us)
 ********************************************************************
 */

#include "monit.h"

#pragma code

/********************************************************************
 *	64バイトを最速でmemcpy
 ********************************************************************
 *  void memcpy64(char *t,char *s)
 *	引数:
 *		char *t : 転送先アドレス
 *		char *s : 転送元アドレス
 *
 *  注意:
 *		すべてasm記述なので、引数取得(FSR1データスタックから)は自前で行っている.
 *		転送バイト数は64バイト固定.
 *
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


/*********************************************************************
 *	１μ秒単位の時間待ち.（最大255μ秒まで）
 *********************************************************************
 */
void wait_us(uchar us)
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


/*********************************************************************
 *	１ｍ秒単位の時間待ち. （最大255ｍ秒まで）
 *********************************************************************
 */
void wait_ms(uchar ms)
{
	do {
		wait_us(250);
		wait_us(250);
		wait_us(250);
		wait_us(250);
	}while(--ms);
}
