/*********************************************************************
 *	P I C b o o t
 *********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef	_LINUX_
#include <windows.h>
#endif


#define MAIN_DEF	// for monit.h
#include "monit.h"
#include "hidasp.h"
#include "util.h"

#define CMD_NAME "picboot.exe"
#define DEFAULT_SERIAL	"*"

char HELP_USAGE[]={
	"* PICboot Ver 0.3 (" __DATE__ ")\n"
	"Usage is\n"
	"  " CMD_NAME " [Options] <hexfile.hex>\n"
	"Options\n"
	"  -p[:XXXX]   ...  Select serial number (default=" DEFAULT_SERIAL ").\n"
	"  -r          ...  Run after write.\n"
	"  -v          ...  Verify.\n"
	"  -E          ...  Erase.\n"
	"  -rp         ...  Read program area.\n"
	"  -nv         ...  No-Verify.\n"
	"  -B          ...  Allow Bootloader Area to Write / Read !.\n"
	"  -sXXXX      ...  Set program start address. (default=0800)\n"
};

#define	MAX_CMDBUF	4096

static	char lnbuf[1024];
static	char usb_serial[128]=DEFAULT_SERIAL;	/* 使用するHIDaspxのシリアル番号を格納 */
static	char verbose_mode = 0;	//1:デバイス情報をダンプ.
		int  hidmon_mode = 1;	/* picmon を実行中 */
static	uchar 	databuf[256];
static	int		dataidx;
static	int		adr_u = 0;		// hexrec mode 4


#define	FLASH_START	0x0800
#define	FLASH_SIZE	0x10000
#define	FLASH_STEP	32
#define	ERASE_STEP	256

#define	FLASH_OFFSET 0		// 0x800 = テスト.

static	uchar flash_buf[FLASH_SIZE];
static	int	  flash_start = FLASH_START;
static	int	  flash_end   = FLASH_SIZE;
static	int	  flash_end_for_read = 0x8000;

#define	CACHE_SIZE	64
static	uchar cache_buf[CACHE_SIZE];
static	int   cache_addr=(-1);

static	char  opt_r  = 0;		//	'-r '
static	char  opt_rp = 0;		//	'-rp'
static	char  opt_re = 0;		//	'-re'
static	char  opt_rf = 0;		//	'-rf'

static	char  opt_E  = 0;		//	erase
static	char  opt_v  = 0;		//	'-v'
static	char  opt_nv = 0;		//	'-nv'

//	ユーザーアプリケーション開始番地.
static	int   opt_s  = 0x800;	//	'-s1000 など'

#define	CHECKSUM_CALC	(1)


#define	DEVCAPS_BOOTLOADER	0x01

void hidasp_delay_ms(int dly);
/*********************************************************************
 *	使用法を表示
 *********************************************************************
 */
void usage(void)
{
	fputs( HELP_USAGE, stdout );
}
/*********************************************************************
 *	一行入力ユーティリティ
 *********************************************************************
 */
static	int getln(char *buf,FILE *fp)
{
	int c;
	int l;
	l=0;
	while(1) {
		c=getc(fp);
		if(c==EOF)  {
			*buf = 0;
			return(EOF);/* EOF */
		}
		if(c==0x0d) continue;
		if(c==0x0a) {
			*buf = 0;	/* EOL */
			return(0);	/* OK  */
		}
		*buf++ = c;l++;
		if(l>=255) {
			*buf = 0;
			return(1);	/* Too long line */
		}
	}
}
/**********************************************************************
 *	INTEL HEXレコード 形式01 を出力する.
 **********************************************************************
 */
static	int	out_ihex01(unsigned char *data,int adrs,int cnt,FILE *ofp)
{
	int length, i, sum;
	unsigned char buf[1024];

	buf[0] = cnt;
	buf[1] = adrs >> 8;
	buf[2] = adrs & 0xff;
	buf[3] = 0;

	length = cnt+5;
	sum=0;

	memcpy(buf+4,data,cnt);
	for(i=0;i<length-1;i++) {
		sum += buf[i];
	}
	sum = 256 - (sum&0xff);
	buf[length-1] = sum;

	fprintf(ofp,":");
	for(i=0;i<length;i++) {
		fprintf(ofp,"%02X",buf[i]);
	}
	fprintf(ofp,"\n");
	return 0;
}
/**********************************************************************
 *	INTEL HEXレコード 形式01 (バイナリー化済み) を解釈する.
 **********************************************************************
 */
#if	0
static void put_flash_buf(int adrs,int data)
{
	adrs = adrs - FLASH_BASE;
	if( (adrs >= 0) && (adrs < FLASH_SIZE ) ) {
		flash_buf[adrs] = data;
	}
}
#endif
/**********************************************************************
 *	INTEL HEXレコード 形式01 (バイナリー化済み) を解釈する.
 **********************************************************************
 */
static	int	parse_ihex01(unsigned char *buf,int length)
{

	int adrs = (buf[1] << 8) | buf[2];
	int cnt  =  buf[0];
	int i;
	int sum=0;

#if	CHECKSUM_CALC
	for(i=0;i<length;i++) {
		sum += buf[i];
	}
//	printf("checksum=%x\n",sum);
	if( (sum & 0xff) != 0 ) {
		fprintf(stderr,"%s: HEX RECORD Checksum Error!\n", CMD_NAME);
		exit(EXIT_FAILURE);
	}
#endif

	buf += 4;

#if	HEX_DUMP_TEST
	printf("%04x",adrs|adr_u);
	for(i=0;i<cnt;i++) {
		printf(" %02x",buf[i]);
	}
	printf("\n");
#endif


	if(adr_u != 0) return 1;	//上位アドレスが非ゼロ(64kB以外)

	for(i=0;i<cnt;i++) {
		flash_buf[adrs+i] = buf[i];
	}
	return 0;
}
/**********************************************************************
 *	INTEL HEXレコード(１行:バイナリー化済み) を解釈する.
 **********************************************************************
 */
int	parse_ihex(int scmd,unsigned char *buf,int length)
{
	switch(scmd) {
		case 0x00:	//データレコード:
					//データフィールドは書き込まれるべきデータである。
			return parse_ihex01(buf,length);

		case 0x01:	//エンドレコード:
					//HEXファイルの終了を示す.付加情報無し.
			return scmd;
			break;

		case 0x02:	//セグメントアドレスレコード:
					//データフィールドにはセグメントアドレスが入る。 たとえば、
					//:02000002E0100C
					//		   ~~~~
					//
			break;

		case 0x03:	//スタートセグメントアドレスレコード.
			break;

		case 0x04:	//拡張リニアアドレスレコード.
					//このレコードでは32ビットアドレスのうち上位16ビット
					//（ビット32～ビット16の）を与える。
				adr_u = (buf[4]<<24)|(buf[5]<<16);
//				printf("adr_u=%x\n",adr_u);
				return scmd;
			break;

		case 0x05:	//スタートリニアアドレス：
					//例えば
					//:04000005FF000123D4
					//         ~~~~~~~~
					//となっていれば、FF000123h番地がスタートアドレスになる。

			break;

	}
	fprintf(stderr,"Unsupported ihex cmd=%x\n",scmd);
	return 0;
}

/**********************************************************************
 *	16進2桁の文字列をバイナリーに変換する.
 **********************************************************************
 */
int read_hex_byte(char *s)
{
	char buf[16];
	int rc = -1;

	buf[0] = s[0];
	buf[1] = s[1];
	buf[2] = 0;
	sscanf(buf,"%x",&rc);
	return rc;
}

/**********************************************************************
 *	16進で書かれた文字列をバイナリーに変換する.
 **********************************************************************
 */
static	int read_hex_string(char *s)
{
	int rc;

	dataidx = 0;
	while(*s) {
		rc = read_hex_byte(s);s+=2;
		if(rc<0) return rc;
		databuf[dataidx++]=rc;
	}
	return 0;
}

/**********************************************************************
 *
 **********************************************************************
 0 1 2 3 4
:1000100084C083C082C081C080C07FC07EC07DC0DC
 ~~ データ長
   ~~~~ アドレス
       ~~ レコードタイプ
         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~データ
                                         ~~チェックサム.
 */
int	read_ihex(char *s)
{
	int c;
	if(s[0] == ':') {
		read_hex_string(s+1);
		c = databuf[3];
		parse_ihex(c,databuf,dataidx);
	}
	return 0;
}
/*********************************************************************
 *	HEX file を読み込んで flash_buf[] に格納する.
 *********************************************************************
 */
int	read_hexfile(char *fname)
{
	FILE *ifp;
	int rc;
	Ropen(fname);
	while(1) {
		rc = getln(lnbuf,ifp);
		read_ihex(lnbuf);
		if(rc == EOF) break;
	}
	Rclose();
	return 0;
}

/*********************************************************************
 *	flash_buf[]の指定エリアが 0xffで埋まっていることを確認する.
 *********************************************************************
 *	埋まっていれば 0 を返す.
 */
static	int	check_prog_ff(int start,int size)
{
	int i;
	for(i=0;i<size;i++) {
		if(flash_buf[start+i]!=0xff) return 1;
	}
	return 0;
}

/*********************************************************************
 *	flash_buf[]の指定エリアが 0xffで埋まっていることを確認する.
 *********************************************************************
 *	埋まっていれば 0 を返す.
 */
static	void modify_jmp( int addr , int dst )
{
	dst >>= 1;
	flash_buf[addr+0] = dst;	// lo(dst)
	flash_buf[addr+1] = 0xef;	// GOTO inst

	flash_buf[addr+2] = dst>>8;	// hi(dst)
	flash_buf[addr+3] = 0xf0;	// bit15:12 = '1'
}

/*********************************************************************
 *	必要なら、flash_buf[] の 0x800,0x808,0x818番地を書き換える.
 *********************************************************************
 */
void modify_start_addr(int start)
{
	if(start == 0x800) return;	//デフォルト値なので何もしない.

	if(start < 0x800) {			// ユーザープログラム開始番地が0x800以前にあるのはおかしい.
		fprintf(stderr,"WARNING: start address %x < 0x800 ?\n",start);
		return;
	}

	if( check_prog_ff(0x800,0x20) ) {
		fprintf(stderr,"WARNING: Can't write JMP instruction.\n");
		return;
	}

	modify_jmp( 0x800 , start );
	modify_jmp( 0x808 , start + 0x08 );
	modify_jmp( 0x818 , start + 0x18 );

}

/*********************************************************************
 *	ターゲットの再起動.
 *********************************************************************
 */
int reboot_target(void)
{
//	UsbBootTarget(0,1);
	UsbBootTarget(0x800,1);
	return 0;
}

/*********************************************************************
 *	flash_buf[] の内容を ターゲット PIC に書き込む.
 *********************************************************************
 */
int	write_block_for_test(int addr,int size)
{
	int i,f=0;
	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if(f) {
		printf("%04x",addr);
		for(i=0;i<size;i++) {
			printf(" %02x",flash_buf[addr+i]);
		}
		printf("\n");
	}
	return 0;
}
/*********************************************************************
 *	64byteを消去.
 *********************************************************************
 */
int	erase_block(int addr,int size)
{
	int i,f=0;
	int pages = size / 64;

	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if((f) || (opt_E)) {
		UsbEraseTargetROM(addr + FLASH_OFFSET,pages);
		Sleep(20);
//		hidasp_delay_ms(20);
	}
	return 0;
}

/*********************************************************************
 *	flash_buf[] の内容を ターゲット PIC に書き込む.
 *********************************************************************
 */
int	write_block(int addr,int size)
{
	int i,f=0;
	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if(f) {
		UsbFlash(addr + FLASH_OFFSET,AREA_PGMEM,flash_buf+addr,size);
		Sleep(3);
//		hidasp_delay_ms(3);
		fprintf(stderr,"Writing ... %04x\r",addr);
#if	0
		for(i=0;i<size;i++) {
			printf(" %02x",flash_buf[addr+i]);
		}
		printf("\n");
#endif
	}
	return 0;
}
/*********************************************************************
 *	 ターゲット PIC の内容を flash_buf[] に読み込む.
 *********************************************************************
 */
int	read_block(int addr,uchar *read_buf,int size)
{
	int errcnt=0;
#if	0
	UsbRead(addr+ FLASH_OFFSET,AREA_PGMEM,read_buf,size);
#else
	// Cached.
	if((size==32)&&(addr == (cache_addr+32))) {
		memcpy(read_buf,cache_buf+32,size);
	}else{
		// Uncached.
		UsbRead( addr+ FLASH_OFFSET,AREA_PGMEM,cache_buf,CACHE_SIZE);
		cache_addr = addr;
		memcpy(read_buf,cache_buf,size);
	}
#endif
	return errcnt;
}

#if	0
int	read_block(int addr,int size)
{
	int errcnt=0;
	uchar *read_buf;

	read_buf = &flash_buf[addr] ;
	UsbRead(addr + FLASH_OFFSET,AREA_PGMEM,read_buf,size);
	return errcnt;
}
#endif
/*********************************************************************
 *	flash_buf[] の内容を ターゲット PIC に書き込む.
 *********************************************************************
 */
int	verify_block(int addr,int size)
{
	int i,f=0;
	int errcnt=0;
	uchar 	verify_buf[256];
	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if(f) {
//		UsbRead(addr + FLASH_OFFSET,AREA_PGMEM,verify_buf,size);
		read_block(addr,verify_buf,size);
		for(i=0;i<size;i++) {
			if(flash_buf[addr+i] != verify_buf[i]) {
				errcnt++;
				fprintf(stderr,"Verify Error in %x : write %02x != read %02x\n"
					,addr+i,flash_buf[addr+i],verify_buf[i]);
			}
		}
		fprintf(stderr,"Verifying ... %04x\r",addr);
	}
	return errcnt;
}
/*********************************************************************
 *	FLashの消去.
 *********************************************************************
 */
int	erase_flash(void)
{
	int i;
	fprintf(stderr,"Erase ... \n");
	for(i=flash_start;i<flash_end;i+= ERASE_STEP) {
		erase_block(i,ERASE_STEP);
	}
	return 0;
}
/*********************************************************************
 *	ダミー.
 *********************************************************************
 */
int disasm_print(unsigned char *buf,int size,int adr)
{
	unsigned short *inst = (unsigned short *)buf;
	printf("%04x %04x\n",adr,inst[0]);
	return 2;
}
/*********************************************************************
 *	flash_buf[] の内容を ターゲット PIC に書き込む.
 *********************************************************************
 */
int	write_hexdata(void)
{
	int i;
	for(i=flash_start;i<flash_end;i+= FLASH_STEP) {
		write_block(i,FLASH_STEP);
	}
	return 0;
}
/*********************************************************************
 *	flash_buf[] の内容を ターゲット PIC に書き込む.
 *********************************************************************
 */
int	verify_hexdata(void)
{
	int i;
	int errcnt = 0;
	cache_addr = (-1);

	fprintf(stderr,"\n");
	for(i=flash_start;i<flash_end;i+= FLASH_STEP) {
		errcnt += verify_block(i,FLASH_STEP);
	}
	return errcnt;
}
/*********************************************************************
 *	ポート名称からアドレスを求める.（ダミー処理）
 *********************************************************************
 */
int	portAddress(char *s)
{
	return 0;
}

/*********************************************************************
 *	ROM読み出し結果をHEXで出力する.
 *********************************************************************
 */
#define	HEX_DUMP_STEP	16

void print_hex_block(FILE *ofp,int addr,int size)
{
	int i,j,f;
	for(i=0;i<size;i+=HEX_DUMP_STEP,addr+=HEX_DUMP_STEP) {
		f = 0;
		for(j=0;j<HEX_DUMP_STEP;j++) {
			if( flash_buf[addr+j] != 0xff ) {
				f = 1;
			}
		}
		if(f) {

#if	0
			printf(":%04x",addr);
			for(j=0;j<HEX_DUMP_STEP;j++) {
				printf(" %02x",flash_buf[addr+j]);
			}
			printf("\n");
#endif
			out_ihex01(&flash_buf[addr],addr,HEX_DUMP_STEP,ofp);


		}
	}


}
/*********************************************************************
 *	ROM読み出し.
 *********************************************************************
 */
void read_from_pic(char *file)
{
	int i, progress_on;
	FILE *ofp;
	uchar *read_buf;
	cache_addr = (-1);

//	fprintf(stderr, "Reading...\n");
	 progress_on = 1;
#if 1	/* by senshu */
	if(file != NULL && strcmp(file, "con")==0) {
		ofp=stdout;
		progress_on = 0;
	} else {
		if (file == NULL) {
			 file = "NUL";
		}
		ofp=fopen(file, "wb");
		if(ofp==NULL) {
			fprintf(stderr, "%s: Can't create file:%s\n", CMD_NAME, file);
			exit(1);
		}
	}
#else
	Wopen(file);
#endif
	fprintf(ofp,":020000040000FA\n");
	for(i=flash_start;i<flash_end_for_read;i+= FLASH_STEP) {
#if 1
		if (progress_on)
			fprintf(stderr,"Reading ... %04x\r",i);
#else
		fprintf(stderr,"Reading ... %04x\r",i);
#endif
		read_buf = &flash_buf[i] ;
		read_block(i,read_buf,FLASH_STEP);
		print_hex_block(ofp,i,FLASH_STEP);
		fflush(ofp);
	}
	fprintf(ofp,":00000001FF\n");
#if 1
	if (progress_on)
		fprintf(stderr,"\nRead end address = %04x\n", i-1);
#endif
	Wclose();
}
/*********************************************************************
 *	メイン
 *********************************************************************
 */
void getopt_p(char *s)
{
		if (s) {
			if (*s == ':') s++;
			if (*s == '?') {
				hidasp_list("picmon");
				exit(EXIT_SUCCESS);
			} else if (isdigit(*s)) {
				if (s) {
					int n, l;
					l = strlen(s);
					if (l < 4 && isdigit(*s)) {
						n = atoi(s);
						if ((0 <= n) && (n <= 999)) {
							sprintf(usb_serial, "%04d", n);
						} else {
							if (l == 1) {
								usb_serial[3] = s[0];
							} else if  (l == 2) {
								usb_serial[2] = s[0];
								usb_serial[3] = s[1];
							} else if  (l == 3) {
								usb_serial[1] = s[0];
								usb_serial[2] = s[1];
								usb_serial[3] = s[2];
							}
						}
					} else {
						strncpy(usb_serial, s, 4);
					}
				}
			} else if (*s == '\0'){
				strcpy(usb_serial, DEFAULT_SERIAL);		// -pのみの時
			} else {
				strncpy(usb_serial, s, 4);
			}
			strupr(usb_serial);
		}
}
/*********************************************************************
 *	メイン
 *********************************************************************
 */
int main(int argc,char **argv)
{
	int errcnt, ret_val,retry;



	//オプション解析.
	Getopt(argc,argv,"i");
	if(IsOpt('h') || IsOpt('?') || IsOpt('/')) {
		usage();
		exit(EXIT_SUCCESS);
	}
	if((argc<2)&& (!IsOpt('r')) && (!IsOpt('E')) && (!IsOpt('p')) ) {
		usage();
		exit(EXIT_SUCCESS);
	}


	if(IsOpt('B')) {					// Bootエリアの強制読み書き.
		flash_start = 0;
		flash_end   = FLASH_START;
	}
	if(IsOpt('E')) {
		opt_E = 1;				//強制消去あり.
	}
	if(IsOpt('p')) {
		getopt_p(Opt('p'));		//シリアル番号指定.
	}
	if(IsOpt('s')) {
		sscanf(Opt('s'),"%x",&opt_s);	//アプリケーションの開始番地指定.
	}
	if(IsOpt('r')) {
		char *r = Opt('r');
		if(r[0]==0  ) opt_r = 1;		//書き込み後リセット動作あり.
		if(r[0]=='p') opt_rp = 1;		//'-rp' flash領域の読み込み.
		if(r[0]=='e') opt_re = 1;		//'-re'
		if(r[0]=='f') opt_rf = 1;		//'-rf'
	}
	if(IsOpt('n')) {
		char *n = Opt('n');
		if(n[0]=='v') opt_nv = 1;		//'-nv' 
	}
	if(IsOpt('v')) {
		opt_v = 1;						//'-v' 
	}


  for(retry=2;retry>=0;retry--) {
	//初期化.
   if( UsbInit(verbose_mode, 0, usb_serial) == 0) {
		fprintf(stderr, "Try, UsbInit(\"%s\").\n", usb_serial);
		if(retry==0) {
			fprintf(stderr, "%s: UsbInit failure.\n", CMD_NAME);
			exit(EXIT_FAILURE);
		}
   }else{

	if( UsbGetDevID() == DEV_ID_PIC18F14K) {	// 14K50のFlash ROMサイズ設定.
		flash_end_for_read = 0x3fff;
	}else{
		flash_end_for_read = 0x7fff;	// 0x5fff ... PIC18F2455(24kB)の場合
	}
	if(IsOpt('B')) {					// Bootエリアの強制読み書き.
		break;		// Rebootしない.
	}
	if((UsbGetDevCaps() & DEVCAPS_BOOTLOADER)) {
		break;		// BOOTLOADER機能あり.
	}else{
		fprintf(stderr, "Reboot firmware ...\n");
		UsbBootTarget(0x000c,1);
		Sleep(10);
		UsbExit();
	}
   }
   Sleep(2500);
  }

//	Flash_Unlock();


	if((opt_E) && (argc < 2)) {					// 強制消去の実行.
		erase_flash();							//  Flash消去.
	}

	memset(flash_buf,0xff,FLASH_SIZE);
	ret_val = EXIT_SUCCESS;

	if(argc>=2) {
		if(opt_rp||opt_re||opt_rf) {	// ROM読み出し.
			read_from_pic(argv[1]);
		}else{
			read_hexfile(argv[1]);		//	HEXファイルの読み込み.
			modify_start_addr(opt_s);

			if(IsOpt('v')==0) {			// ベリファイのときは書き込みをしない.
				erase_flash();				//  Flash消去.
				write_hexdata();			//  書き込み.
			}

		  if(opt_nv==0) {
			errcnt = verify_hexdata();	//  ベリファイ.
			if(errcnt==0) {
				fprintf(stderr,"\nVerify Ok.\n");
			}else{
				fprintf(stderr,"\nVerify Error. errors=%d\n", errcnt);
				ret_val = EXIT_FAILURE;
			}
		  }
		}
	}
#if 1
	else if (argc==1) {
		if(opt_rp||opt_re||opt_rf) {	// ROM読み出し.
			read_from_pic(NULL);
		}
	}
#endif

//	Flash_Lock();

	if(opt_r) {
		reboot_target();			//  ターゲット再起動.
	}
	UsbExit();
	return ret_val;
}
/*********************************************************************
 *
 *********************************************************************
 */

