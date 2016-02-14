#ifndef	_linuxdef_h
#define	_linuxdef_h

//------------------------------------------------------
#ifdef	_LINUX_

#define	MAX_PATH	262
#define	_MAX_PATH	262

#define	TRUE		1
#define	FALSE		0


typedef int HANDLE;

//	MSDOSに存在する関数のダミー版をプロトタイプ宣言する:
void Sleep(int mSec);			// wait mSec.
void strupr(char *s);			// 文字列を全部大文字化.
int	stricmp(char *s1,char *s2);	// 大文字小文字の区別をしない文字列比較.
int getch(void);
int kbhit(void);

//--------dummy--------------------------------
#define	INVALID_HANDLE_VALUE	(-1)
typedef int SC_HANDLE;
typedef int BOOL;

#define	SC_MANAGER_ALL_ACCESS	0
#define	SERVICE_ALL_ACCESS		0
#define	OPEN_EXISTING			0

typedef unsigned long  DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef char 		 CHAR;
typedef	long 		 LONG;
typedef	long long	 LONGLONG;


//-------from winnt.h ------------------------
#define CLRDTR 6
#define CLRRTS 4
#define SETDTR 5
#define SETRTS 3
#define SETXOFF 1
#define SETXON 2
#define SETBREAK 8
#define CLRBREAK 9
#define MS_CTS_ON 16
#define MS_DSR_ON 32
#define MS_RING_ON 64
#define MS_RLSD_ON 128
#define NOPARITY	0
#define ODDPARITY	1
#define EVENPARITY	2
#define MARKPARITY	3
#define SPACEPARITY	4
#define ONESTOPBIT	0
#define DTR_CONTROL_DISABLE 0
#define DTR_CONTROL_ENABLE 1
#define DTR_CONTROL_HANDSHAKE 2
#define RTS_CONTROL_DISABLE 0
#define RTS_CONTROL_ENABLE 1
#define RTS_CONTROL_HANDSHAKE 2
#define RTS_CONTROL_TOGGLE 3

typedef struct _COMMTIMEOUTS {
	DWORD ReadIntervalTimeout;
	DWORD ReadTotalTimeoutMultiplier;
	DWORD ReadTotalTimeoutConstant;
	DWORD WriteTotalTimeoutMultiplier;
	DWORD WriteTotalTimeoutConstant;
} COMMTIMEOUTS,*LPCOMMTIMEOUTS;

typedef struct _DCB {
	DWORD DCBlength;
	DWORD BaudRate;
	DWORD fBinary:1;
	DWORD fParity:1;
	DWORD fOutxCtsFlow:1;
	DWORD fOutxDsrFlow:1;
	DWORD fDtrControl:2;
	DWORD fDsrSensitivity:1;
	DWORD fTXContinueOnXoff:1;
	DWORD fOutX:1;
	DWORD fInX:1;
	DWORD fErrorChar:1;
	DWORD fNull:1;
	DWORD fRtsControl:2;
	DWORD fAbortOnError:1;
	DWORD fDummy2:17;
	WORD wReserved;
	WORD XonLim;
	WORD XoffLim;
	BYTE ByteSize;
	BYTE Parity;
	BYTE StopBits;
	char XonChar;
	char XoffChar;
	char ErrorChar;
	char EofChar;
	char EvtChar;
	WORD wReserved1;
} DCB,*LPDCB;

#define VER_PLATFORM_WIN32s 0
#define VER_PLATFORM_WIN32_WINDOWS 1
#define VER_PLATFORM_WIN32_NT 2
#define VER_NT_WORKSTATION 1

#define SERVICE_DEMAND_START 3
#define SERVICE_KERNEL_DRIVER 1

#define GENERIC_READ	0x80000000
#define GENERIC_WRITE	0x40000000
#define GENERIC_EXECUTE	0x20000000

#define FILE_ATTRIBUTE_READONLY			0x00000001
#define FILE_ATTRIBUTE_HIDDEN			0x00000002
#define FILE_ATTRIBUTE_SYSTEM			0x00000004
#define FILE_ATTRIBUTE_DIRECTORY		0x00000010
#define FILE_ATTRIBUTE_ARCHIVE			0x00000020
#define FILE_ATTRIBUTE_DEVICE			0x00000040
#define FILE_ATTRIBUTE_NORMAL			0x00000080
#define FILE_ATTRIBUTE_TEMPORARY		0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE		0x00000200

#define SERVICE_ERROR_IGNORE 0
#define SERVICE_ERROR_NORMAL 1
#define SERVICE_ERROR_SEVERE 2

typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } u;
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;


typedef struct _OSVERSIONINFOA {
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	CHAR szCSDVersion[128];
} OSVERSIONINFOA,*POSVERSIONINFOA,*LPOSVERSIONINFOA;
typedef OSVERSIONINFOA OSVERSIONINFO;

typedef struct _COMSTAT {
	DWORD fCtsHold:1;
	DWORD fDsrHold:1;
	DWORD fRlsdHold:1;
	DWORD fXoffHold:1;
	DWORD fXoffSent:1;
	DWORD fEof:1;
	DWORD fTxim:1;
	DWORD fReserved:25;
	DWORD cbInQue;
	DWORD cbOutQue;
} COMSTAT,*LPCOMSTAT;

#endif

#endif
