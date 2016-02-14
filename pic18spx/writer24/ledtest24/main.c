#include "p24Fxxxx.h"

#define F_CPU	32000000UL	// 32MHz

_CONFIG1(JTAGEN_OFF & GCP_OFF & GWRP_OFF 
		& BKBUG_OFF & COE_OFF & ICS_PGx1 & FWDTEN_OFF )

_CONFIG2(IESO_OFF & FNOSC_FRCPLL & FCKSM_CSDCMD & OSCIOFNC_OFF 
	   & IOL1WAY_OFF & I2C1SEL_PRI & POSCMOD_NONE)


void delay_1us(void)
{
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");

	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");

	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");

}

void delay_us(unsigned long us)
{
	while(us--){
		delay_1us();
	}
}

void delay_ms(unsigned long ms)
{
	while(ms--){
		delay_us(1000);
	}
}

int main(void)
{
    AD1PCFG = 0xffff;	// すべてディジタルポートとする
	_RCDIV  = 0;		// PostScaler ( 0:32MHz 1:16MHz )
	TRISB   = 0xfffe;	// PORTB bit0 のみ OUTPUT
	{
		char  c = 0;
		while(1){
			c ^= 0x01;
			PORTB = c;
			delay_ms(100);
		}
	}
}
