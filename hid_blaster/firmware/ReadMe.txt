■ 概要

  これは、MicroChipが提供している PIC18 のUSBアプリケーションフレームワーク
  に含まれているHID-Bootloader をもとにして作成した PIC18F専用の簡易モニタです。



■ ビルド環境の構築の手引き

（１）
  まず、WinAVR(GNU Make) をインストールします。
  http://sourceforge.net/projects/winavr/
  インストール先は、通常なら C:\WinAVR\ になります。
  WinAVR/utils/bin には、make.exe 以外にもunixシェルでよく使用する ls や cat , rm
  などといったお馴染みのツールのWin32バイナリーが含まれているので、非常に便利です。
  
（２）
  次に、mcc18(Compiler)をインストールします。Standard-Eval VersionでOKです。
  http://www.microchip.com/
  インストール先は、通常なら C:\mcc18\ になります。
  
（３）
  続いて、MicroChipのサイトから、下記USBアプリケーションライブラリ、
  MicrochipApplicationLibrariesv2009-07-10.zip
  もしくはそれより新しいバージョンを入手してください。


  本ディレクトリは、
  MicrochipApplicationLibrariesv2009-07-10.zip
  の内容の一部を、GNU Makeビルドを通す目的で再配置、再構成したものです。


  準備が出来たら、パスを通します。
#  ・パスの通し方:
#    PATH %PATH%;C:\mcc18\bin;C:\mcc18\mpasm;C:\WinAVR\utils\bin;
#                ~~~~~~~~~    ~~~~~~~~ 下線部はmcc18のインストール先に応じて読み替えてください.
  適当なバッチファイルを用意して呼び出すか、あるいはWindowsのシステムプロパティ
  -->詳細-->環境変数-->ユーザーの環境変数
  PATH に C:\mcc18\bin;C:\mcc18\mpasm;C:\WinAVR\utils\bin
  を記述します。
  インストール時にパスを通してしまった場合は、上記確認のみで結構です。


■ ファイル一覧

ReadMe.txt			.....      このファイル.
Makefile            .....      GNU Makeが使用します.
                              (18F2550用と18F4550用は共通HEXです)
srec2bin.exe        .....      HEXファイルをダンプ形式に変換するツールです.
mplistconv.exe      .....      LSTファイルを見やすくするツールです.


★品種別のリンカースクリプト:
rm18F14K50.lkr      .....	   18F14K50用のリンカースクリプト
rm18F2550.lkr       .....	   18F2550用のリンカースクリプト
rm18F4550.lkr       .....	   18F4550用のリンカースクリプト


★ソース:
main.c              .....      
monit.c             .....      
picwrt.c            .....      
timer2.c            .....      
ps2keyb.c           .....      
print.c             .....      
keybtest.c          .....      
usbdsc.c            .....      
usb/usbmmap.c       .....      
usb/usbctrltrf.c    .....      
usb/usbdrv.c        .....      
usb/hid.c           .....      
usb/usb9.c          .....      

★ヘッダーファイル:
fuse-config.h
io_cfg.h
hidcmd.h
monit.h
usbdsc.h
usb/usbcfg.h
usb/usbdrv.h
usb/usbmmap.h
usb/typedefs.h
usb/usbdefs_ep0_buff.h
usb/usbdefs_std_dsc.h
usb/usb.h
usb/usb9.h
usb/hid.h
usb/usbctrltrf.h

★使用しないファイル
mchip-src/bootmain.c
mchip-src/BootPIC18NonJ.c
mchip-src/BootPIC18NonJ.h

■ 応用編

★PIC18F14K50 / PIC18F2550 両対応.

  元々MicroChipの cdc-serial emulatorは多品種対応ですが、今回もMakefileの
    DEVICE=18F2550 
  の行を書き換えることで、PIC18F14K50にも対応させることが出来ます。
  
  makeall.bat をコマンドラインで実行すると、両方のHEXが得られます。

  また、18F4550 は 18F2550のHEXをそのまま流用できるようです。

■ 何に使えますか？

 ・picmonit.exe 上のコマンドと、firmware上のコマンド受け取り処理の対を
 　自分で追加することにより、いろいろな機能を追加することが出来ます。


 ・main.c の以下のスイッチを有効(1)にして、
	#define	TIMER0_INTERRUPT	0		// タイマー０割り込みを使用する.
	#define	USE_PS2KEYBOARD		0		// PS/2キーボードI/F を使用する.
   
   MakefileのOBJSに下記コメントアウトされた３行を有効にすると、一応PS/2
   キーボードの打鍵入力のハンドリングを確認することができるようです。
　　（動作中にLEDが点きます。また、動作中にキーボード入力バッファ内容を
　　　ダンプして確認することも出来ます）

OBJS = \
	main.o		\
	monit.o		\
	usbdsc.o	\
	usb/hid.o	\
	usb/usb9.o	\
	usb/usbctrltrf.o	\
	usb/usbdrv.o		\
	usb/usbmmap.o		\

#	timer0.o	\
#	task.o		\
#	ps2keyb.o	\

