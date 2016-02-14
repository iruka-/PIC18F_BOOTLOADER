/**********************************************************************
	モトローラＳフォーマットをバイナリー化する.
 **********************************************************************
 */
#include "easy.h"
#include <malloc.h>
#include <string.h>

Usage(
	" ===== Motorola Srec to Binary Converter =====\n"
	"\n"
	"Usage:\n"
	"$ srec2bin infile outfile\n"
	"Option:\n"
	"  -b  byte dump\n"
	"\n"
);

static	char	infile[256];
static	char	outfile[256];

static	unsigned char 	databuf[256];
static	int				dataidx;

#define	BIN_MAX	0x1000000
static	char	bin[BIN_MAX];	// 16MB
static	int		start_adr=0;
static	int		end_adr=0;

static	int		adr_u = 0;		// hexrec mode 4
/**********************************************************************
 *		
 **********************************************************************
 */
void init_bin(void)
{
	int i;
	for(i=0;i<BIN_MAX;i++) bin[i]=0xff;
}

/**********************************************************************
 *		
 **********************************************************************
 */
int read_hex1(char *s)
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
 *		
 **********************************************************************
 */
int read_hex(char *s)
{
	int rc;

	dataidx = 0;
	while(*s) {
		rc = read_hex1(s);s+=2;
		if(rc<0) return rc;
		databuf[dataidx++]=rc;
	}
	return 0;
}

int	srec(int adrsize,unsigned char *buf,int length)
{
	int recsize;
	int adrs=0;
	int sum;
	int cnt;
	int i;
	recsize = *buf++;
	for(i=0;i<adrsize;i++) {
		adrs <<= 8;
		adrs |= *buf++;
	}
	cnt = recsize - adrsize - 1;
	
	if( recsize != (length-1) ) {
		printf("WARNING:record length mismatch:%d != %d\n",recsize,(length-1));
	}

	printf("%06x",adrs);
	for(i=0;i<cnt;i++) {
		printf(" %02x",buf[i]);
	}
	printf("\n");
}

int	write_hex(int scmd,unsigned char *buf,int length)
{
	switch(scmd) {
		case '0':
			break;
		case '1':	return srec(2,buf,length);
		case '2':	return srec(3,buf,length);
		case '3':	return srec(4,buf,length);

		case '7':
		case '8':
		case '9':
			break;
	}
}

/**********************************************************************
 *		
 **********************************************************************
 */
int	ihex(unsigned char *buf,int length)
{
	int adrs = (buf[1] << 8) | buf[2];
	int cnt  =  buf[0];
	int i;
	buf += 4;

	if(IsOpt('b')) {
	for(i=0;i<cnt;i++,adrs++) {
		printf("%04x",adrs|adr_u);
		printf(" %02x\n",buf[i]);
	}
		return;
	}

	printf("%04x",adrs|adr_u);
	for(i=0;i<cnt;i++) {
		printf(" %02x",buf[i]);
	}
	printf("\n");
}
/**********************************************************************
 *		
 **********************************************************************
 */
int	write_ihex(int scmd,unsigned char *buf,int length)
{
	switch(scmd) {
		case 0x00:	//データレコード:
					//データフィールドは書き込まれるべきデータである。
			return ihex(buf,length);
		case 0x01:	//エンドレコード:
					//HEXファイルの終了を示す.付加情報無し.
			return;
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
				return;
			break;
		case 0x05:	//スタートリニアアドレス：
					//例えば
					//:04000005FF000123D4
					//         ~~~~~~~~
					//となっていれば、FF000123h番地がスタートアドレスになる。

			break;
		
	}
	printf("scmd=%x\n",scmd);
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
	read_hex(s);
	c = databuf[3];
	write_ihex(c,databuf,dataidx);
}
/**********************************************************************
 *		
 **********************************************************************
 */
int	read_line(char *s)
{
	int c;
	c = *s++;
	if(c==0) return;
	if(c==':') {
//		printf(">%s<\n",s);
		return read_ihex(s);
	}
	if(c!='S') {
		printf("WARNING:garbled line:%s\n",s);
		return -1;
	}
	c = *s++;
	switch(c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '7':
		case '8':
		case '9':
			//printf("Srec %s\n",s);
			read_hex(s);
			write_hex(c,databuf,dataidx);
			return 0;
		default:
			printf("WARNING:garbled line:%s\n",s);
			return -1;
	}
}

/**********************************************************************
 *		
 **********************************************************************
 */
void read_srec(void)
{
	char buf[1024];
	int rc,eof;
	Ropen(infile);
	while(1) {
		eof = getln(buf,ifp);
		rc  = read_line(buf);
		if(eof == EOF) break;
	}	
	Rclose();
}

/**********************************************************************
 *		
 **********************************************************************
 */
int	main(int argc,char **argv)
{
	Getopt(argc,argv);
	if(argc<3) {
		usage();
	}
	strcpy(infile,argv[1]);
	strcpy(outfile,argv[2]);

	init_bin();
	read_srec();

	return 0;
}

/**********************************************************************
 *		
 **********************************************************************
 */
