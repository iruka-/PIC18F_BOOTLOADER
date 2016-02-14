#ifndef	picwrt_h_
#define	picwrt_h_

//	機能スイッチ.
#define	SUPPORT_PIC24F			1	// PIC24ライター機能を含みます.
#define	INCLUDE_JTAG_CMD		1	// JTAG ライター機能を含みます.

#define	ALTERNATE_PGxPIN_ASSIGN	1	// PGC,PGD,PGMの結線を入れ替えます.


/* picwrt.c */
uchar GetCmdb1 (void);
void SetCmdb1 (uchar b);
int recvData16 (void);
void GetData8L (int adr, uchar *buf, int len);
void SetCmd16 (uchar cmd4, ushort data16, uchar ms);
void SetCmd16L (uchar cmd4, ushort *buf, uchar len, uchar ms);
void SetCmdN (ushort cmdn, uchar len);
void SetCmdNLong (uchar cmdn, uchar len, uchar ms);
void SetCmdb1Long (uchar b, uchar ms);
void SetPGDDir (uchar f);
void add_data (uchar c, uchar bits);
void add_data24 (uchar u, uchar h, uchar l);
void add_data4 (uchar c);
void add_data8 (uchar c);
void cmd_picspx(void);
//void memcpy64 (char *t,char *s);
void pic_bitbang (void);
void pic_getdata8L (void);
void pic_read24f (void);
void pic_setcmd16L (void);
void pic_setpgm (void);
void pic_write24f (void);
void sendData24 (dword c);
void send_addr24 (uchar reg);
void send_goto200 (void);
void send_nop1 (void);
void send_nop2 (void);
void setaddress24 (void);
void wait_ms (uchar ms);


#endif

