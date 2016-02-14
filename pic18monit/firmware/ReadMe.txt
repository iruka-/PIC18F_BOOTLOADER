■ 概要

  これは PIC18F2550/4550/14K50 を使用した、USBバルク転送フレームワークです
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  ・MicroChipが提供している PIC18 のUSBアプリケーションフレームワークを元にしています。

  ・MPLABは使わずに、GNU Makefile を使用してビルドするようにしています。

    (wineなどの環境設定さえ行えば、Linuxのコマンドライン・シェル上でビルド可能です)


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
mkall.bat           .....	   サポートしている全品種用のHEXを作ります.
                              (18F2550用と18F4550用は共通HEXです)
srec2bin.exe        .....      HEXファイルをダンプ形式に変換するツールです.
mplistconv.exe      .....      LSTファイルを見やすくするツールです.
  上記ツールは make dumplst もしくは make dumphex で使用できます.
mplistcnv.pl        .....      LSTファイルを見やすくするツールのperl版.


★ソース:
main.c              .....      メイン処理だけを抜き出したもの.
usb_descriptors.c   .....      ディスクリプタテーブル.


★ヘッダーファイル:
HardwareProfile.h   .....      チップ品種による、ハードウェアプロファイルのセレクタ.
Hp_LowPinCount.h    .....	   18F14K50用のハードウェアプロファイル
Hp_2550.h           .....	   18F2550/4550用のハードウェアプロファイル
usb_config.h        .....	   USBの設定.


★品種別のリンカースクリプト:
rm18F14K50.lkr      .....	   18F14K50用のリンカースクリプト
rm18F2550.lkr       .....	   18F2550用のリンカースクリプト
rm18F4550.lkr       .....	   18F4550用のリンカースクリプト

★Windows用のinfファイル
driver_inf/

