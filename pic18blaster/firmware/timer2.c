/********************************************************************
 *	タイマー２ (SPIクロック生成)
 ********************************************************************
 */
#include "usb/typedefs.h" 
#include "usb/usb.h"      
#include "io_cfg.h"       

#include <timers.h>       

/**********************************************************************
 *	初期化.
 **********************************************************************
	t2config:
			bit ---- --xx = prescale
			bit -yyy y--- = postscale
			bit 7--- ---- = interrupt enable

	period: n (1～255)
			1/n
 **********************************************************************
	prescale xx:
			 00=  1/1
			 01=  1/4
			 1x=  1/16
    postscale yyyy:
    		  0000 = 1/1
    		  0001 = 1/2
			  0010 = 1/3
			   ・・・
			  1111 = 1/16
 **********************************************************************
	分周比	TMR2IF = 12MHz * prescale * period * postscale ;
 */
void timer2_init(uchar t2config,uchar period)
{
    OpenTimer2(t2config);
	PR2 = period;
}


/*------------------------------------------------------------------------
	SPI制御レジスタをマスターモードで初期化する.
    -d0 | fOSC/2  ... 6MHz
    -d1 | fOSC/4  ... 3MHz
    -d2 | fOSC/8  ... 1.5MHz
    -d3 | fOSC/16 ... 750kHz
    -d4 | fOSC/32 ... 375kHz
    -d5 | fOSC/64 ... 187kHz
    -d6 | fOSC/128...  93kHz
    -d7以降はリニアに遅くなります. 
 *------------------------------------------------------------------------
 */
void timer2_interval(uchar spi_delay)
{
	if	   (spi_delay==0) {timer2_init(0,2-1);}		//-d0 | fOSC/2  ... 6MHz
	else if(spi_delay==1) {timer2_init(0,4-1);}		//-d1 | fOSC/4  ... 3MHz
	else if(spi_delay==2) {timer2_init(0,8-1);}		//-d2 | fOSC/8  ... 1.5MHz
	else if(spi_delay==3) {timer2_init(1,4-1);}		//-d3 | fOSC/16 ... 750kHz
	else if(spi_delay==4) {timer2_init(1,8-1);}		//-d4 | fOSC/32 ... 375kHz
	else if(spi_delay==5) {timer2_init(2,4-1);}		//-d5 | fOSC/64 ... 187kHz
	else if(spi_delay>=6) {
		timer2_init(2,spi_delay);					//-d6～|fOSC/128 ... 93kHz
	}
}
/**********************************************************************
 *	
 **********************************************************************
 */


