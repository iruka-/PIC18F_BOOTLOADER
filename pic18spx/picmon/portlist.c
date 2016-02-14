/* portlist.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "monit.h"
#include "hidasp.h"
#include "portlist.h"
#include "util.h"

/*********************************************************************
 *	ポート名称	,  アドレス , 属性（バイト幅、よく参照されるかどうか）
 *********************************************************************
 */
/*********************************************************************
 *	ポート名称	,  アドレス , 属性（バイト幅、よく参照されるかどうか）
 *********************************************************************
 */

#if	0
#include "portlist_2313.h"
#endif

#if	0
#include "portlist_mega88.h"
#endif

#if	1
#include "portlist_pic18.h"
#include "portlist_14k50.h"
#endif

PortList *portList = portList_pic18;	//書き換えあり.

#define	_PIC_COL	1

#if	_PIC_COL
#define	COLS 3
#else
#define	COLS 4
#endif

/*********************************************************************
 *	大文字小文字を区別しない文字列比較＋bit属性検出
 *********************************************************************
 */
int str_casecmpx(char *s,char *t,int *pmask)
{
	int bit;
	while(*t) {
		if(tolower(*s) != tolower(*t)) return 1;	//不一致.
		s++;
		t++;
	}
	if(*s==0) {
		*pmask = 0; return 0;	//一致.
	}
	if(*s=='.') {
		s++;
		if(s[1]==0) {
			bit = s[0] - '0';
			if((bit >= 0) && (bit < 8)) {
				*pmask = (1 << bit);
				return 0;		// port.bit (bitは0～7) 一致.
			}
		}
	}
	return 1;	//不一致.
}

/*********************************************************************
 *	ポート名称からアドレスを求める.
 *********************************************************************
 */
int	portAddress(char *s)
{
	PortList *p = portList;
	while(p->name) {
		if(stricmp(s,p->name)==0) return p->adrs;
		p++;
	}
	return 0;
}

/*********************************************************************
 *	ポート名称からアドレスとビットマスクを求める.
 *********************************************************************
 */
int	portAddress_b(char *s,int *pmask)
{
	PortList *p = portList;
	while(p->name) {
		if(str_casecmpx(s,p->name,pmask)==0) {
			return p->adrs;
		}
		p++;
	}
	return 0;
}

/*********************************************************************
 *	PortアドレスをPort名に翻訳する.
 *********************************************************************
 *	成功=1
 */
char *GetPortName(int adrs)
{
	PortList *p = portList;
	while(p->name) {
		if(p->adrs == adrs) return p->name;
		p++;
	}
	return NULL;
}

/*********************************************************************
 *	ポート一覧表を表示する.
 *********************************************************************
 */
void PrintPortNameList(void)
{
	PortList *p = portList;
	int      i,k;
	int      m = 0;
	char     buf[256][128];

	memset(buf,0,sizeof(buf));

	while(p->name) {
#if	_PIC_COL
		sprintf(buf[m],"%9s = 0x%02x ",p->name,p->adrs);
#else
		sprintf(buf[m],"%8s = 0x%02x ",p->name,p->adrs);
#endif
		m++;
		p++;
	}
	//段組みしたい.
	k = (m+COLS-1)/COLS;	//COLS段にした場合の行数.
	for(i=0;i<k;i++) {
#if	_PIC_COL
		printf("%s%s%s\n",buf[i],buf[i+k],buf[i+k*2]);
#else
		printf("%s%s%s%s\n",buf[i],buf[i+k],buf[i+k*2],buf[i+k*3]);
#endif
	}
}

/*********************************************************************
 *	数値 d を二進数文字列に変換し buf に格納する.
 *		bufと同じアドレスを返す.
 *********************************************************************
 */
char *radix2str(char *buf,int d)
{
	char *rc = buf;
	int i,m,c=' ';
	m = 0x80;
	for(i=0;i<8;i++) {
		if(d & m) c='1';
		else      c='0';
		*buf++ = c;
		m >>= 1;
		if(i==3) *buf++ = '_';
	}
	*buf = 0;
	return rc;
}

/*********************************************************************
 *	段組表示によるポート値出力.
 *********************************************************************
 */
void PrintPortColumn(void)
{
	PortList *p = portList;
	int      i,k;
	int      m = 0;
	static   char     buf[256][128];	//結果バッファ（段組）

	char     tmp[128];		//２進数化するバッファ.

#if	_AVR_PORT_
	uchar data[0x20 + 0x40];//ポート読み取り値.
	UsbRead(0x20,AREA_RAM,data+0x20,0x40);
//	memdump(data+0x20,0x40,0x20);return;
#else
	// PIC
	uchar data[256];//ポート読み取り値.
	UsbRead(0xf50,AREA_RAM,data+0x50,0x100-0x50);
#endif

	memset(buf,0,sizeof(buf));

	while(p->name) {
		if(p->attr & 1) {
#if	_PIC_COL
			sprintf(buf[m],"%9s = %s ",p->name,radix2str(tmp,data[p->adrs & 0xff]));
#else
			sprintf(buf[m],"%6s = %s ",p->name,radix2str(tmp,data[p->adrs & 0xff]));
#endif
			m++;
		}
		p++;
	}
	//段組みしたい.
	k = (m+COLS-1)/COLS;	//COLS段にした場合の行数.
	for(i=0;i<k;i++) {
#if	_PIC_COL
		printf("%s%s%s\n",buf[i],buf[i+k],buf[i+k*2]);
#else
		printf("%s%s%s%s\n",buf[i],buf[i+k],buf[i+k*2],buf[i+k*3]);
#endif
	}
}

/*********************************************************************
 *	ポート内容を全部ダンプする.
 *********************************************************************
 */
void PrintPortAll(int mask)
{
	PortList *p = portList;
#if	_AVR_PORT_
	uchar data[0x20 + 0x40];//ポート読み取り値.
	UsbRead(0x20,AREA_RAM,data+0x20,0x40);
//	memdump(data+0x20,0x40,0x20);return;
#else
	// PIC
	uchar data[256];//ポート読み取り値.
	UsbRead(0xf50,AREA_RAM,data+0x50,0x100-0x50);

#endif
	while(p->name) {
		if(p->attr & 1) {
			if((mask==0)||(p->attr & QQ)) {
				cmdPortPrintOne(p->name,p->adrs,data[p->adrs & 0xff]);
			}
		}
		p++;
	}
}

/*********************************************************************
 *	
 *********************************************************************
 */
void delete_ExPort(void)
{
	PortList *p = portList;
	while(p->name) {
		if(	p->attr & EX_PORT) {
			p->attr &= (~1);
		}
		p++;
	}
}
/*********************************************************************
 *	PORTEの存在チェック.
 *********************************************************************
 */
int	InitPortList(void)
{
#define	LATE	0xf8d
#define	LATE_2	(1<<2)
	int late;
	late = hidPeekMem( LATE );
	hidPokeMem (LATE, late | LATE_2 , 0);
	late = hidPeekMem( LATE );
	if(late & LATE_2) {
		hidPokeMem (LATE, late & (~LATE_2) , 0);
		return 1;
	}else{
		delete_ExPort();
		return 0;
	}
}

/*********************************************************************
 *	PortListを14k50用に切り替える.
 *********************************************************************
 */
void ChangePortList14K(void)
{
	portList = portList_14k50;
}
/*********************************************************************
 *
 *********************************************************************
 */
