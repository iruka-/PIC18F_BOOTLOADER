
// PGM,MCLRのセットアップ.
void	PicPgm(int f);

void	EraseCmd(int cmd05,int cmd04);
void	BulkErase(int cmd80);
void	WriteFlash(int adr,uchar *buf,int wlen);
void	WriteFuse(int adr,uchar *buf);
void	ReadFlash(int adr,uchar *buf,int len,int dumpf);
void	ReadFuse( int adr,uchar *buf,int len,int dumpf);
void	PicRead(char *buf,int cnt);

int	    DeviceCheck(void);
int     GetDeviceID(void);

void	wait_us(int us);
