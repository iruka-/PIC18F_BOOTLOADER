/*********************************************************************
 *	DLL main
 *********************************************************************
 *
 */

#include <windows.h>
#include <stdio.h>


/*********************************************************************
 *	メイン
 *********************************************************************
 */

/*********************************************************************
 *
 *********************************************************************
 */

/*
 * DLLエントリポイント関数の型と典型的なスケルトンコード
 */
BOOL APIENTRY					/* int __stdcall */
DllMain(HINSTANCE hInstance, DWORD ul_reason_for_call, LPVOID pParam)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		/* ここにグローバル変数等の初期化コードを書く */
		/* ※loaddll でDLLをロードした時はここが実行される */
		break;

	case DLL_PROCESS_DETACH:
		/* ここにグローバル変数等の後始末コードを書く */
		/* ※freedll でDLLをアンロードした時はここが実行される */
		break;

	case DLL_THREAD_ATTACH:
		/* ここにスレッド毎に必要な変数等の初期化コードを書く */
		break;

	case DLL_THREAD_DETACH:
		/* ここにスレッド毎に必要な変数等の後始末コードを書く */
		break;
	}
	return TRUE;
}


/*********************************************************************
 *	以下、dummy関数.
 *********************************************************************
 */

int disasm_print(unsigned char *buf,int size,int adr)
{
	return 0;
}

void cmdPortPrintOne(char *name,int adrs,int val)
{
//	char tmp[32];
//	printf("%8s(%02x) %02x %s\n",name,adrs,val,radix2str(tmp,val));
}

