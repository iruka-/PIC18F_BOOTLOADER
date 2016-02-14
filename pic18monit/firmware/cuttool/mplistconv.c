/**********************************************************************
	MPASMが出力した .lstファイルを整形する.
 **********************************************************************
 */
#include "easy.h"
#include <malloc.h>
#include <string.h>

Usage(
	" ===== MPLINK .LST format converter =====\n"
	"Usage:\n"
	"$ mplistconv [-m80] infile.lst outfile.txt\n"
	"\n"
	"Option:\n"
	"  -m<MAXLENGTH>   set line max length\n"
	"\n"
);

#define CRLF "\r\n"

static	char	infile[256];
static	char	outfile[256];

static	char 	curfile[256];

static	int		start_adr=0;
static	int		end_adr=0;

static	int		adr_u = 0;		// hexrec mode 4
static	unsigned char	maxlength=0;

/**********************************************************************
 *		文字(c)がSJIS漢字の１文字目該当するなら1
 **********************************************************************
 */
static	int	iskanji1(int c)
{
	c &= 0xff;
	if( (c >= 0x81) && (c < 0xa0) ) return 1;
	if( (c >= 0xe0) && (c < 0xfc) ) return 1;
	return 0;
}

/**********************************************************************
 *		文字列(s)の(n)文字目がSJIS漢字の１文字目該当するなら1
							  SJIS漢字の２文字目該当するなら2
 それ以外なら0
 **********************************************************************
 */
static	int	iskanji_str(char *s,int n)
{
	int i,c,f=0;
	i=0;
	while(*s) {
		c = *s;
		if(f == 1) {
			f=2;				//SJIS漢字２文字目
		}else{
			if(iskanji1(c)) f=1;	//SJIS漢字１文字目
			else f=0;				//ASCII
		}
		if(i>=n) return f;
		i++;
		s++;
	}
	return 0;
}

/**********************************************************************
 *		文字列 *s に含まれる文字cの位置を右端からサーチする.
 **********************************************************************
 *	マッチしない場合は (-1)を返す.
 *  マッチした場合は、左端からの文字(c)の位置（文字数）を返す.
 */
static int rindex(char *s,int c)
{
	int n = strlen(s);
	int i;
	for(i=n-1;i>=0;i--) {
		if(s[i]==c) return i;
	}
	return -1;
}

/**********************************************************************
 *		文字列 *s の右端にある空白文字を全て削除する.
 **********************************************************************
 */
void chop_sp(char *s)
{
	int n = strlen(s);
	int i;
	int c = ' ';
	for(i=n-1;i>=0;i--) {
		if(	s[i] != c) {
			s[i+1]=0;
			return;
		}
	}
	
}

/**********************************************************************
 *		文字列 *s を文字数maxlength でカットする.
 **********************************************************************
 */
void cut_string(char *s,int maxlength)
{
	if( iskanji_str(s,maxlength)==1 ) {
		s[maxlength]=0;
	}else{
		s[maxlength+1]=0;
	}
}
/**********************************************************************
 *		大文字アルファベットなら1を返す.
 **********************************************************************
 */
static	int	isALPHA(int c)
{
	if((c>='A')&&(c<='Z'))return 1;
	return 0;
}
/**********************************************************************
 *		.lstファイルを１行単位で処理する.
 **********************************************************************
 */
int	read_line(char *s)
{
	int n = strlen(s);
	int m = rindex(s,' ');
	char *t;
	if(m>=0) {
		t = s+m+1;
		if(isALPHA(t[0]) && (t[1]==':')) {
			s[m]=0;
			if(	strcmp(curfile,t)!=0) {
				strcpy(curfile,t);
				fprintf(ofp,"FILE:%s" CRLF,curfile);
			}
		}
	}
	
	chop_sp(s);
	if(	maxlength>=32 ) {
		cut_string(s,maxlength);
	}
	chop_sp(s);
	fprintf(ofp,"%s" CRLF,s);
}

/**********************************************************************
 *		.lstファイルを読み込んで処理.
 **********************************************************************
 */
void read_lst(void)
{
	char buf[4096];
	int rc,eof;
	Ropen(infile);
	Wopen(outfile);
	while(1) {
		eof = getln(buf,ifp);
		rc  = read_line(buf);
		if(eof == EOF) break;
	}
	Wclose();
	Rclose();
}

/**********************************************************************
 *		メイン処理
 **********************************************************************
 */
int	main(int argc,char **argv)
{
	int m=80;
	Getopt(argc,argv);
	if(argc<3) {
		usage();
	}
	strcpy(infile,argv[1]);
	strcpy(outfile,argv[2]);

	if(IsOpt('m')) {
		sscanf(Opt('m'),"%d",&m);
		maxlength=m;
	}
	read_lst();

	return 0;
}

/**********************************************************************
 *		
 **********************************************************************
 */
