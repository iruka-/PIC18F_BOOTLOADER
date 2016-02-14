/*********************************************************************
 *	赤外線リモコンパルスを解析して表示する.
 *********************************************************************
      SYNC             DATA0           DATA1           STOP    
AHEA  ON(8T) +OFF(4T)  ON(1T)+OFF(1T)  ON(1T)+OFF(3T)  ON(1T)  
NEC   ON(16T)+OFF(8T)  ON(1T)+OFF(1T)  ON(1T)+OFF(3T)  ON(1T)  
SONY  ON(4T) +OFF(1T)  ON(1T)+OFF(1T)  ON(2T)+OFF(1T)  -       
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef	_LINUX_
#include <conio.h>		// getch() kbhit()
#endif

#include "monit.h"
#include "hidasp.h"
#include "util.h"

#define	DUMP_PULSE	(0)

enum {
	 NONE,
	 AHEA,
	 SONY,
	 NEC,
};
/*********************************************************************
 *
 *********************************************************************
 */
#define	DATA_BIT	0x01

#define	T1			4		//
#define	MARK_0		4		//
#define	MARK_1		9		//
#define	SPACE_0		4		//
#define	SPACE_1		12		//

#define	MARK_HD		36		//
#define	SPACE_HD	18		//
#define	SONY_HDR	29		//
#define	AHEA_HDR	44		//


#define	MAX_BIT		1024

/*********************************************************************
 *	ビットパターンを保存.
 *********************************************************************
 */
uchar bit_buf[MAX_BIT];
int	  bit_ptr;
int	  bit_status;
int	  mode;
/*********************************************************************
 *	文字列を保存.
 *********************************************************************
 */
char  puts_buf[MAX_BIT];
char *puts_ptr;

/*********************************************************************
 *	ビットパターンに1bit追加する.
 *********************************************************************
 */
void add_bit(int b)
{
	if(	bit_ptr < MAX_BIT) {
		bit_buf[bit_ptr++]=b;
	}
}

/*********************************************************************
 *	1bit分の信号(low,highの組)を解析しつつコンソール表示.
 *********************************************************************
 */
void print_infra_aeha(int mark,int space)
{
	static	int   bits=0;
	char *mode_s="AHEA";
	if(mode==NEC) mode_s="NEC";

	if( mark >= (MARK_HD - 10) ) {
		puts_ptr += sprintf(puts_ptr," %s:",mode_s);
		bits=0;
		bit_ptr=0;
	}else{
		if( ( mark >= 2 ) && ( mark <= (MARK_0 + 3) ) ) {
			if(space < SPACE_0 * 2) {
				puts_ptr += sprintf(puts_ptr,"0");add_bit(0);
			}else{
				puts_ptr += sprintf(puts_ptr,"1");add_bit(1);
			}
		}else{
			puts_ptr += sprintf(puts_ptr,"?");add_bit(0);
		}
		bits++;
		if(	bits>=8 ) {
			bits=0;
			puts_ptr += sprintf(puts_ptr,"_");
		}
	}
}

/*********************************************************************
 *	1bit分の信号(low,highの組)を解析しつつコンソール表示.
 *********************************************************************
 */
void print_infra_sony(int mark,int space)
{
	static	int   bits=0;
	if( mark > (MARK_1 * 2) ) {
		puts_ptr += sprintf(puts_ptr," SONY:");
		bits=0;
		bit_ptr=0;
	}else{
		if( ( mark >= 2 ) && ( mark <= (MARK_1 * 2) ) ) {
			if(mark < MARK_1) {
				puts_ptr += sprintf(puts_ptr,"0");add_bit(0);
			}else{
				puts_ptr += sprintf(puts_ptr,"1");add_bit(1);
			}
		}else{
			puts_ptr += sprintf(puts_ptr,"?");add_bit(0);
		}
		bits++;
		if(	bits>=8 ) {
			bits=0;
			puts_ptr += sprintf(puts_ptr,"_");
		}
	}
}

/*********************************************************************
 *	1bit分の信号(low,highの組)を解析しつつコンソール表示.
 *********************************************************************
 */
void print_infra_1(int mark,int space)
{
	if(bit_status==(-1)) return;

	switch(bit_status) {
	 case NONE:
		if( (mark > AHEA_HDR) ) {
			bit_status = NEC;
			print_infra_aeha(mark,space);
		}else if(mark >= SONY_HDR) {
			bit_status = AHEA;
			print_infra_aeha(mark,space);
		}else if(mark >= (SONY_HDR/2)) {
			bit_status = SONY;
			print_infra_sony(mark,space);
		}
		mode = bit_status;
		break;
	 case AHEA:
	 case NEC:
		print_infra_aeha(mark,space);
		break;
	 case SONY:
		print_infra_sony(mark,space);
		break;
	}

	if( space >= (MARK_HD + SPACE_HD)*2 ) {
		bit_status=(-1);
		return;
	}
	
}
/*********************************************************************
 *	1bit分の信号(low,high個別)を解析しつつコンソール表示.
 *********************************************************************
 */
void print_infra(int c,int len)
{
	static int mark_len=0;
	if(c) {
		mark_len = len;
	}else{
		print_infra_1(mark_len,len);
		mark_len = 0;
	}
}

/*********************************************************************
 *	ビットパターン表示を16進表示に直す.
 *********************************************************************
 */
void print_hex_code(uchar *s,int len)
{
	int i,byte=0,m=0x01;

	printf("%s HEX:",puts_buf);
	puts_ptr = puts_buf;

	if(mode==SONY) {
		len = (len+7)&(-8);
	}
	for(i=0;i<len;i++) {
		if((i&7)==0) {	// bit 0
			m=0x01;		// LSBから.
			byte=0;
		}
		if(s[i]) {
			byte|=m;	//ビットパターンを1byteのデータに反映.
		}
		m<<=1;
		if((i&7)==7) {	//8bit集まった.
			printf("%02x ",byte);
		}
	}
	printf("\n");
}

/*********************************************************************
 *	赤外線リモコンパルスを解析して表示する.
 *********************************************************************
 *	引数
 *	 buf[] : 10kHzサンプルされた porta のロジアナデータ. (LSBのみ使用)
 *	 cnt   : 有効サンプル数.
 */
void analize_infra(uchar *buf,int cnt)
{
	int i,c,len,f;

	bit_ptr=0;
	memset(bit_buf,0,MAX_BIT);

	bit_status=NONE;
	puts_ptr = puts_buf;


	len = 0;	//連続数.
	f = buf[0] & DATA_BIT;	//直前の値.

	for(i=0;i<cnt;i++) {
		c = buf[i] & DATA_BIT;
		if( c != f ) {
#if	DUMP_PULSE
			if(f) {
				printf("*H:%3d\n",len);
			}else{
				printf("*L:%3d\n",len);
			}
#endif
			print_infra(c,len);
			f = c;
			len=0;
		}else{
			len++;
		}
	}
	if(len) print_infra(c ^ DATA_BIT,len);

	print_hex_code(bit_buf,bit_ptr);
}


/*********************************************************************
 *
 *********************************************************************
 */

