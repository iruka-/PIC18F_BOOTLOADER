/*********************************************************************
 *	アナログテスター風の表示.
 *********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "gr.h"

#include "analog.h"
#include "monit.h"

#define	LOWPASS_NEEDLE	10
#define	LOWPASS_DIGITAL	4

int	getAnalogVolt(double *val,int f);
int	getRegistance(double *val,int f);

int	needle_val=-1;
static	int blue = 0xff000080;

/*********************************************************************
 *	値から座標の素を作る.
 *********************************************************************
 *	引数:
 *		value : 0(0.0V) ～ 5000(5.0V)
 *	戻り値:
 *		*px:  cos(rad);
 *		*py:  sin(rad);
 *	処理内容 0～5000の値を 45度～(45+90)度に変換し、三角関数を適用する.
 */
void calc_sincos(int value,float *px,float *py)
{
	float degree, rad;

	degree = 45.0 + (value * 90.0 / 5000.0);
	if(	degree >= (45+90+10)) {
		degree  = (45+90+10);
	}
	rad = degree * M_PI * 2 / 360;
	*px =  cos(rad);
	*py =  sin(rad);
}
/*********************************************************************
 *	抵抗値ＫΩを電圧計スケール(0～5000)に変換.
 *********************************************************************
          E
 	I = -----   ,  R = Rin + Rconst  , Rconst = 3.3k
          R
	Imin = 0, Imax = 5000
 */
int reg_scaled(double Kohm,int *density)
{
	double E = 5000.0 * 3.3;
	double I,I2;
	double Rconst = 3.3;
	if(Kohm<0) return 0;		// Open

	I = E / (Kohm + Rconst);

	if(density) {
		I2 = E / (Kohm + Rconst + 0.01);
		*density = (I - I2)* 100;
	}
	return (int) I;
}
/*********************************************************************
 *	目盛りを描く.
 *********************************************************************
 *	引数:
 *		n : 0(0.0V) ～ 5000(5.0V)
 *	 flag : 0:普通の目盛り / 1:0.5V毎の目盛り / 2:1.0V毎の目盛り
 */
void draw_scale_sub(int n,int flag,char *str)
{
	int x1,y1,x2,y2;
	int cx = AIN_CENTER_X;
	int cy = AIN_CENTER_Y;
	int r1 = AIN_CENTER_R-8;
	int r2;

	float x,y;
	int color = 0x000040;
	int value = n;
	calc_sincos(value,&x,&y);

	switch(flag) {	//目盛りの長さを選択.
		default:
		case 0:	r2 = AIN_CENTER_R;break;
		case 1:	r2 = AIN_CENTER_R+8;break;
		case 2:	r2 = AIN_CENTER_R+16;break;
	}
	// 線分の両端の座標を計算.
	x1 = cx - (x*r1);
	y1 = cy - (y*r1);
	x2 = cx - (x*r2);
	y2 = cy - (y*r2);
	gr_line(x1,y1,x2,y2,color);	//線分を描く.

	if(str) {		//数字を書く.
		gr_puts(x2-8,y2-16,str,color,BKCOL,12);
	}
}
/*********************************************************************
 *	目盛りを描く.
 *********************************************************************
 * 	 i : 0(0.0V) ～ 50(5.0V)
 *flag : 0:普通の目盛り / 1:0.5V毎の目盛り / 2:1.0V毎の目盛り
 */
void draw_scale(void)
{
	char buf[256];
	char *str;
	int i,f;
	for(i=0;i<=50;i++) {
		f = 0;
		if((i%5)==0) f=1;
		if((i%10)==0) f=2;
		if(f==2) {	// 1.0V単位のところには、文字を書く.
			sprintf(buf,"%d.0",i/10);
			str = buf;
		}else{
			str = NULL;
		}
		draw_scale_sub(i * 100,f,str);
	}
}

/*********************************************************************
 *	抵抗専用 目盛りを描く.
 *********************************************************************
 */
void draw_reg_scale_sub(int ohm,int step1,int step2)
{
	double r;
	int	scl,dens,flag;
	char buf[256];
	char *str=NULL;
	r = (double) ohm / 1000.0;
	scl = reg_scaled(r,&dens);
	flag = 0;
	if((ohm % step1)==0) {
		flag = 1;
	}
	if((ohm % step2)==0) {
		flag = 2;
		if(ohm >= 3000) {
			sprintf(buf,"%gk",(double)ohm/1000.0);
		}else{
			sprintf(buf,"  %gk",(double)ohm/1000.0);
		}
		str = buf;
	}
	draw_scale_sub(scl,flag,str);
}
void draw_reg_scale(void)
{
	int i;
	for(i=0;i<1000;i+=100) {
		draw_reg_scale_sub(i,500,500);
	}
	for(i=1000;i<=7000;i+=100) {
		draw_reg_scale_sub(i,500,1000);
	}
	for(i=7000;i<10000;i+=1000) {
		draw_reg_scale_sub(i,5000,5000);
	}
	// 10K
	for(i=10000;i<=20000;i+=1000) {
		draw_reg_scale_sub(i,5000,5000);
	}
	for(i=20000;i<100000;i+=10000) {
		draw_reg_scale_sub(i,50000,50000);
	}
	for(i=100000;i<1000000;i+=100000) {
		draw_reg_scale_sub(i,500000,500000);
	}
	draw_reg_scale_sub(99999999,100,100);
}

//	====================================================
//	ＢＯＸ(x1,y1) - (x2,y2),c
//	====================================================
void box_sub(int x1,int y1,int x2,int y2,int c1,int c2,int c3,int c4)
{
	gr_hline(x1,y1,x2,y1,c1);
	gr_hline(x1,y2,x2,y2,c3);
	gr_vline(x1,y1,x1,y2,c2);
	gr_vline(x2,y1,x2,y2,c4);
}
/*********************************************************************
 *	色つきの枠
 *********************************************************************
 */
void box_arrange(int x,int y,int w,int h,int c1,int c2,int c3,int c4)
{
	int i;
	for(i=0;i<3;i++) {
		box_sub(x,y,x+w-1,y+h-1,c1,c2,c3,c4);
		x++;y++;
		w-=2;
		h-=2;
	}
}

/*********************************************************************
 *	パネルを描く.
 *********************************************************************
 */
void draw_panel(void)
{
	int i, c1, c2, c3, c4;
	int r = AIN_CENTER_R;
	int r1 = 6;
	gr_boxfill(AIN_SCREEN_X0,AIN_SCREEN_X0,AIN_SCREEN_W-AIN_SCREEN_X0-1,AIN_CENTER_H,BKCOL);
	gr_boxfill(0,AIN_CENTER_H+2,AIN_SCREEN_W,AIN_CENTER_H+60,BKCOL2);

	for(i=0;i<1;i++) {
		gr_circle_arc(AIN_CENTER_X,AIN_CENTER_Y+i,r,r,       0 ,45,90+45);
		gr_circle_arc(AIN_CENTER_X,AIN_CENTER_Y+i,r-r1,r-r1, 0 ,45,90+45);
	}
//				 R G B
	c1=0x00c0c090;
	c2=0x00807060;
	c3=0x007050a0;
	c4=0x00504090;
	box_arrange(0,0,AIN_SCREEN_W,AIN_CENTER_H+5,c1,c2,c3,c4);
}

/*********************************************************************
 *	ワイヤーを描く.
 *********************************************************************
 */
void draw_wireframe(void)
{
	int x0,y0,x1,y1;
	x0 = 8;
	x1 = AIN_SCREEN_W-8;
	y1 = AIN_SCREEN_H-8;
	y0 = y1 - AIN_GRAPH_H;
	gr_box(x0-1,y0-1,x1+1,y1+1,blue);
}
/*********************************************************************
 *	針を描く. subr()
 *********************************************************************
 */
void draw_needle_sub(int value,int color)
{
	int x1,y1,x2,y2,dx,dy,dy1,dy2;
	int cx = AIN_CENTER_X;
	int cy = AIN_CENTER_Y;
	int r1 = AIN_CENTER_R-9;
	int r2 = 60;
	float x,y;
	calc_sincos(value,&x,&y);

	x1 = cx - (x*r1);
	y1 = cy - (y*r1);
	x2 = cx - (x*r2);
	y2 = cy - (y*r2);
	// softclip
	if(	y2>AIN_CENTER_H) {
		dy = y2-y1;
		dx = x2-x1;
		dy2 = y2 - AIN_CENTER_H;
		dy1 = AIN_CENTER_H - y1;
		y2 = AIN_CENTER_H;
		//中点を補間.
		x2 = (x1*dy2 + x2*dy1) / dy;
	}

	gr_line(x1,y1,x2-1,y2,color);
	gr_line(x1,y1,x2,y2,color);
	gr_line(x1,y1,x2+1,y2,color);
}

/*********************************************************************
 *	針を描く.
 *********************************************************************
 */
void draw_needle(int value)
{
	if(	needle_val != value) {
		draw_needle_sub(needle_val,BKCOL);
		draw_needle_sub(value , REDCOL);
		needle_val = value;
	}
}
/*********************************************************************
 *	グラフ表示内の文字を書く.
 *********************************************************************
 */
void draw_AnalogFrame(void)
{
	draw_panel();
	draw_scale();
	draw_wireframe();
}
void draw_RegistanceFrame(void)
{
	draw_panel();
	draw_reg_scale();
	draw_wireframe();
}

#define	SCREEN_W	768
#define	SCREEN_X0	32
#define	SCREEN_H	480

#define	SIG_COLOR	0x00f000
#define	SIG_COLOR2	0x004000
#define	BACK_COLOR	0x000000
#define	FRAME_COLOR	0x504000

/*********************************************************************
 *	1サンプルの1ビット分を描画
 *********************************************************************
 *	v    : 0 か 非0   信号レベル.
 *  diff : 0 か 非0   信号反転が起きた.
 */
static void plot_signals(int x,int y,int val,int old)
{
	int col0,col1,col2,i,d;
	col0 = 0;
	col1 = BACK_COLOR ;
	col2 = SIG_COLOR2 ;

	if((x % 40)==0) col0=blue;

	if(x==0) return;


	x += 8;

	gr_vline(x+1,y,x+1,y-AIN_GRAPH_H,col2);	//縦の線(先行線).
	gr_vline(x,y,x,y-AIN_GRAPH_H,col0);		//縦の線(現在線).

	for(i=1;i<5;i++) {
		d = AIN_GRAPH_H * i / 5;
		gr_pset(x,y-d , blue);		// ベースライン赤.
	}
	gr_line(x-1,y-old,x,y-val,SIG_COLOR);

}

#define	VOLT_SCALE	( AIN_GRAPH_H / 5.0 )
/*********************************************************************
 *	グラフを毎回表示する.
 *********************************************************************
 */
static void draw_graphic(int val)
{
	static int old=0;
	static int x=0;
	int y = AIN_SCREEN_H-8;
	plot_signals(x,y,val,old);old=val;
	x++;if(x>=AIN_SCREEN_W-16) {
		x=0;
	}
}
/*********************************************************************
 *	電圧を毎回表示する.
 *********************************************************************
 */
void draw_AnalogInput(void)
{
	static double vavg_n=(-1);
	static double vavg_d=(-1);
	char buf[256];
	double volt;
	int vcol, vsize;

	int dl = LOWPASS_DIGITAL;
	int nl = LOWPASS_NEEDLE;

	int f = getAnalogVolt(&volt,0);

#if	1
	if(f>=0) {
		if(	vavg_n < 0) {
			vavg_n = volt;
		}else{
			vavg_n = (vavg_n * (nl-1) + volt ) / nl;
		}

		if(	vavg_d < 0) {
			vavg_d = volt;
		}else{
			vavg_d = (vavg_d * (dl-1) + volt ) / dl;
		}
	}
#endif


	if(f<0) {
		sprintf(buf,"ERROR");
	}else{
		sprintf(buf,"%6.2f   ",vavg_d);
	}

	//アナログ針で表示.
	draw_needle( (int) (vavg_n*1000) );

	vcol  = 0xaa40e040;
	vsize = 36;
	//数値で表示.
	gr_puts(AIN_CENTER_X-80,AIN_CENTER_H+12,buf   ,vcol,BKCOL2,vsize);
	gr_puts(AIN_CENTER_X+32,AIN_CENTER_H+12,"V   ",vcol,BKCOL2,vsize);

	draw_graphic((int) (volt * VOLT_SCALE));
}


/*********************************************************************
 *	抵抗値を毎回表示する.
 *********************************************************************
 */
void draw_Registance(void)
{
	static double vavg_n=(-1);
	static double vavg_d=(-1);
	char buf[256];
	double Kohm;
	int vcol,  vsize;

	int dl = LOWPASS_DIGITAL;
	int nl = LOWPASS_NEEDLE;

	int f = getRegistance(&Kohm,0);

	int scale = reg_scaled(Kohm,0);

#if	1
	if(f>=0) {
		if( Kohm>=0 ) {
			//ＫΩの値を平滑.
			if(	vavg_d < 0) {
				vavg_d = Kohm;
			}else{
				vavg_d = (vavg_d * (dl-1) + Kohm ) / dl;
			}
		}else{
			vavg_d = -1;
		}
	}
#endif

	//針の位置を平滑.
	if(	vavg_n < 0) {
		vavg_n = scale;
	}else{
		vavg_n = (vavg_n * (nl-1) + scale ) / nl;
	}


	//ＫΩの値を表示.
	if(f<0) {
		sprintf(buf,"ERROR");
	}else{
		if(Kohm<0) {
			sprintf(buf,"  R = OPEN");
		}else{
			sprintf(buf,"%12.3f   ",vavg_d);
		}
	}


	vcol  = 0xaa40e040;
	vsize = 36;
	//数値で表示.
	gr_puts(AIN_CENTER_X-120,AIN_CENTER_H+12,buf   ,vcol,BKCOL2,vsize);
	gr_puts(AIN_CENTER_X+62 ,AIN_CENTER_H+12," KΩ",vcol,BKCOL2,vsize);

	//アナログ針を表示.
	draw_needle( vavg_n );

//	draw_graphic( reg_scaled(vavg_n) );
}


/*********************************************************************
 *	抵抗値 グラフ表示.(仮)
 *********************************************************************
 */
void cmdRegGraph(char *buf)
{
	gr_init(AIN_SCREEN_W,AIN_SCREEN_H,32,0);
	draw_RegistanceFrame();
	do {
		draw_Registance();
		Sleep(32);
	}while(gr_break()==0);
	gr_close();
}
/*********************************************************************
 *	アナログ入力グラフ表示.(仮)
 *********************************************************************
 */
void cmdAinGraph(char *buf)
{
	gr_init(AIN_SCREEN_W,AIN_SCREEN_H,32,0);
	draw_AnalogFrame();
	do {
		draw_AnalogInput();
		Sleep(32);
	}while(gr_break()==0);
	gr_close();
}
/*********************************************************************
 *
 *********************************************************************
 */

