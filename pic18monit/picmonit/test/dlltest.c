/* dlltest.c */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

//#include "hidmon.h" ... こちらはdllのスタティック・リンク用

// ダイナミックリンクの場合は、importする関数は全部関数ポインタを型定義する.
typedef int (__stdcall *_UsbInit)    (const char *string);
typedef int (__stdcall *_UsbExit)    (void);
typedef int (__stdcall *_PortAddress)(const char *string);
typedef int (__stdcall *_UsbPeek)    (int adr, int arena);
typedef int (__stdcall *_UsbPoke)    (int adr, int arena, int data, int mask);

// importする関数は、それぞれの関数ポインタを用意して、import時にポインタをセットする.
_UsbInit     UsbInit;
_UsbExit     UsbExit;
_PortAddress PortAddress;
_UsbPeek     UsbPeek;
_UsbPoke     UsbPoke;



#define	PINB	"portc"
#define	PORTB	"latc"
#define	DDRB	"trisc"

#define RAM_IO	0
#define ALL		0

#ifndef __GNUC__
#pragma comment(lib, "setupapi.lib")
#endif

HINSTANCE hDLL = NULL;		// pic18ctl.dll handle



char *check_poke(char *port, int data)
{
	int rc;

	rc = UsbPoke(PortAddress(port), RAM_IO, data, ALL);
	if (rc > 0) {
		return "[OK]";
	} else {
		return "[ERROR]";
	}
}

void test1(int f)
{
#if	0
	printf(" ddrd := 0x%02x, %s\n", f, check_poke("ddrd", f));
	printf("portd := 0x%02x, %s\n", f, check_poke("portd", f));
	printf(" pind := 0x%02x, %s\n", f, check_poke("pind", f));
#endif
}


////////////////////////////////////////////////////////////////////////
//             pic18ctl.dll をロード
static int LoadDLL(char *dllname)
{
	hDLL = LoadLibrary(dllname);
	if (!hDLL) {
#if 1
		fprintf(stderr, "Error at Load '%s'\n",dllname);
#else
		MessageBox(NULL, "Error at Load DLL", "ERR", MB_OK);
#endif
		return 0;
	}
	
// importする関数は、それぞれの関数ポインタを用意して、import時にポインタをセットする.
	UsbInit     = (_UsbInit) GetProcAddress(hDLL, "UsbInit" );
	UsbExit     = (_UsbExit) GetProcAddress(hDLL, "UsbExit" );
	PortAddress = (_PortAddress) GetProcAddress(hDLL, "PortAddress" );
	UsbPeek     = (_UsbPeek) GetProcAddress(hDLL, "UsbPeek" );
	UsbPoke     = (_UsbPoke) GetProcAddress(hDLL, "UsbPoke" );

	printf("UsbInit()=%x\n",(int) UsbInit );
	printf("PortAddress()=%x\n",(int) PortAddress );

	return 1;
}



unsigned char bits[] = {1,2,4,8,16,32,64,128,64,32,16,8,4,2,1};

int main(int argc, char*argv[])
{
	int i, adr, portb, rc;
	char *serial = "*";

	if (argc == 2) {
		serial = argv[1];
	}
	
	rc = LoadDLL("pic18ctl.dll");
	if(rc==0) {
		exit(1);
	}
	
	rc = UsbInit(serial);
	if (rc>=0) {
		printf("UsbInit(\"%s\") [OK]\n", serial);
	} else {
		printf("UsbInit(\"%s\") [ERROR], rc = %d\n", serial, rc);
		exit(1);
	}

//	printf("%5s = (0x%x)\n", "portb", PortAddress("portb"));

	adr   = PortAddress(PINB);
	portb = PortAddress(PORTB);

//	printf("%5s = (0x%x)\n", PINB, adr);

//	test1(0);
//	test1(20);		// テストのために、例外的な値を書き込む
//	test1(60);		// テストのために、例外的な値を書き込む
//	test1(32);
//	test1(0x32);

//	UsbPoke(PortAddress("ddrb"), RAM_IO, 0xff, ALL);
	for (i = 0; i < 10; i++) {
		int j;
		for (j = 0; j < sizeof(bits); j++) {
			// PORTB3 にRED LED
			UsbPoke(portb, RAM_IO, bits[j], ALL);
			Sleep(100);
		}
	}

	for (i = 0; i < 1000; i++) {
		printf("%5s == 0x%02x\r", PINB, UsbPeek(adr, 0));
	}
	printf("\n");

	/* HIDaspxの標準モードに戻す */
//	UsbPoke(PortAddress("portb"), RAM_IO, 0x38, ALL);
//	UsbPoke(PortAddress("portd"), RAM_IO, 0x67, ALL);
//	UsbPoke(PortAddress("ddrd"), RAM_IO, 0x20, ALL);

	UsbExit();

	printf("UsbExit\n");

	return 0;
}
