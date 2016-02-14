/*********************************************************
 *	Ｃ言語　簡略ヘッダー  Borland-C version. rev.2
 *********************************************************
 */
#include <stdio.h>

#define GETLN 1
#define ADDEXT 1
/*
 *	オプション文字列チェック
 */
char  *opt[128];	/* オプション文字が指定されていたらその文字に
			   続く文字列を格納、指定なければNULL	*/

#define Getopt(argc,argv)  \
 {int i;for(i=0;i<128;i++) opt[i]=NULL; \
   while( ( argc>1 )&&( *argv[1]=='-') ) \
    { opt[ argv[1][1] & 0x7f ] = &argv[1][2] ; argc--;argv++; } \
 }

#define IsOpt(c) ((opt[ c & 0x7f ])!=NULL)
#define   Opt(c)   opt[ c & 0x7f ]

void	mes_printf(char *fmt,...);

/*
 *	ズボラｍａｉｎ
 */
#define Main main(int argc,char **argv){ Getopt(argc,argv);
#define End  exit(0);}

#define Seek(fp,ptr) fseek(fp,ptr,0)

/*
 *	グローバル変数
 */
FILE  *ifp;
FILE  *ofp;
FILE  *ifp2;
FILE  *ofp2;

/*
 *	Ｕｓａｇｅ（"文字列"）
 */
#define Usage(string) void usage(){printf(string);exit(1);}

/*
 *	省略型ファイルアクセス
 */
#define Ropen(name) {ifp=fopen(name,"rb");if(ifp==NULL) \
{ printf("Fatal: can't open file:%s\n",name);exit(1);}  \
}

#define Wopen(name) {ofp=fopen(name,"wb");if(ofp==NULL) \
{ printf("Fatal: can't create file:%s\n",name);exit(1);}  \
}


#define Ropen2(name) {ifp2=fopen(name,"rb");if(ifp2==NULL) \
{ printf("Fatal: can't open file:%s\n",name);exit(1);}  \
}

#define Wopen2(name) {ofp2=fopen(name,"wb");if(ofp2==NULL) \
{ printf("Fatal: can't create file:%s\n",name);exit(1);}  \
}

#define Read(buf,siz)   fread (buf,1,siz,ifp)
#define Write(buf,siz)  fwrite(buf,1,siz,ofp)
#define Read2(buf,siz)  fread (buf,1,siz,ifp2)
#define Write2(buf,siz) fwrite(buf,1,siz,ofp2)

#define Rclose()  fclose(ifp)
#define Wclose()  fclose(ofp)

#define Rclose2()  fclose(ifp2)
#define Wclose2()  fclose(ofp2)

/*
 *	拡張子付加ユーティリティ
 *
 *	addext(name,"EXE") のようにして使う
 *	もし拡張子が付いていたらリネームになる
 */
#ifdef  ADDEXT
void addext(char *name,char *ext)
{
	char *p,*q;
	p=name;q=NULL;
	while( *p ) {
		if ( *p == '.' ) q=p;
		p++;
	}
	if(q==NULL) q=p;
	/*if( (p-q) >= 4) return;  なんかbugっている*/
	strcpy(q,".");
	strcpy(q+1,ext);
}
#endif



#ifdef GETLN
/*
 *	一行入力ユーティリティ
 */
int getln(char *buf,FILE *fp)
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
#endif
