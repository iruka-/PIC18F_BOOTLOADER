#ifndef	_util_h_
#define	_util_h_

#ifdef	_MSDOS_
#define	DLL_int	__declspec(dllexport) int __stdcall
#else
//		Linux
#define	DLL_int	int
#endif

/* util.c */
DLL_int UsbInit (const char *string);
DLL_int PortAddress (const char *string);
DLL_int UsbPeek (int adr, int arena);
DLL_int UsbPoke (int adr, int arena, int data, int mask);
DLL_int UsbExit (void);


int UsbSetPoll (int adr, int arena);
int UsbPoll (uchar *buf);

void	memdump_print(void *ptr,int len,int off);
int		dumpmem(int adr,int arena,int size,unsigned char *buf);
int		UsbBootTarget(int adr,int boot);
int		UsbExecUser(int arg);
int		UsbReadString(uchar *buf);
int		UsbPutKeyInput(int key);
int		pokemem(int adr,int arena,int data0,int data1);
void	hid_read_mode(int mode,int adr);
int		hid_read(int cnt,int nBytes);
int		hid_ping(int i);
void	UsbBench(int cnt,int psize);
void	UsbDump(int adr,int arena,int cnt);
int		UsbDisasm (int adr, int arena, int cnt);
int		UsbRead(int adr,int arena,uchar *buf,int size);
int		UsbSetPoll_slow(int adr,int arena);
int		UsbPoll_slow();
int		UsbReadPacket(int cnt);
int		UsbSyncCmd(int cnt);
int		UsbAnalogPoll(int mode,unsigned char *abuf);
int		UsbEraseTargetROM(int adr,int size);
int		UsbFlash(int adr,int arena,uchar *buf,int size);
void	UsbPoke_b(int adr,int arena,int bit,int mask);
void	local_init(void);
void	UsbCheckPollCmd(void);



#endif

