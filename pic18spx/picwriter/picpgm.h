#define	PIC_DEVICE_ADR	0x3ffffe
#define	PIC_CONFIG_ADR	0x300000

void	wait_us(int us);
void	SetPGDDir(int f);

// portb , pinb , ddrb のアドレスを解決.
void	PicInit(void);
// PGM,MCLRのセットアップ.
void	PicPgm(int f);

void	EraseCmd(int cmd05,int cmd04);
void	BulkErase(int cmd80);
void	WriteFlash(int adr,uchar *buf,int wlen);
void	WriteFuse(uchar *buf);
void	ReadFlash(int adr,uchar *buf,int len,int dumpf);
void	PicRead(char *buf,int cnt);

