■■■ 概要 ■■■

  これは、MicroChipが提供している PIC18 のUSBアプリケーションフレームワークに
  含まれているUSB Generic device をもとにして作成した PIC18F専用の簡易モニタです。



■■■ ディレクトリ構成 ■■■ 

pic18monit --+-- firmware /        main-18F14K50.hex もしくは main-18F2550.hex
             |                     を対応するPICに焼きます。
             |
             +-- driver_inf /      WindowsXP用のUSB Generic デバイス用 inf ファイル
             |
             +-- picmonit /        WindowsXP側のコマンドラインツール picmonit.exe
             |
             +-- libusb /          picmonit.exe をビルドする場合に使用するlibusb-win32
                                   ライブラリです。
             |
             +-- picmonit / test   WindowsXP用: pic18ctl.dll の呼び出し例サンプル.



■■■ ライセンス ■■■ 

 MicroChip提供USBフレームワークの著作権は MicroChip に帰属されています。
 
 それを除いた部分、Windows側ユーティリティはGPLライセンスです。

 PIC18用逆アセンブラのソース はGNU gputilsに由来しています。




■■■ 詳細 ■■■

（１）Windows上での使い方:

 D:> picmonit.exe      ・・・起動
 PIC> help             ・・・HELP表示
 PIC> q                ・・・終了


-------------
* HID_Monit Ver 0.1
Command List
 d  <ADDRESS1> <ADDRESS2>    Dump Memory(RAM)
 dr <ADDRESS1> <ADDRESS2>    Dump Memory(EEPROM)
 dp <ADDRESS1> <ADDRESS2>    Dump Memory(PGMEM)
 e  <ADDRESS1> <DATA>        Edit Memory
 f  <ADDRESS1> <ADDRESS2> <DATA> Fill Memory
 l  <ADDRESS1> <ADDRESS2>    List (Disassemble) PGMEM
 p ?                         Print PortName-List
 p .                         Print All Port (column format)
 p *                         Print All Port (dump format)
 p <PortName>                Print PortAddress and data
 p <PortName> <DATA>         Write Data to PortName
 p <PortName>.<bit> <DATA>   Write Data to PortName.bit
 sleep <n>                   sleep <n> mSec
 bench <CNT>                 HID Write Speed Test
 boot                        Start user program
 run  <address>              Run user program at <address>
 user <arg>                  Run user defined function (usercmd.c)
 poll <portName>             Continuous polling port
 poll <portName> <CNT>       continuous polling port
 poll  *  <CNT>              continuous polling port A,B,D
 graph <portName>            Graphic display
 ain   <CNT>                 Analog input
 aingraph                    Analog input (Graphical)
 aindebug <mode>             Analog input debug
 reg   <CNT>                 Registance Meter
 reggraph                    Registance Meter (Graphical)
 q                           Quit to DOS
PIC>
-------------


-------------------------------------------------------------------------------------
（２）ビルド環境

-Windows上のコマンドラインツールは MinGW-gcc を使用してビルドします。
-PIC18Fファームウェアは MicroChip mcc18コンパイラを使用してビルドします。
--どちらもビルドには make を使用します。



-------------------------------------------------------------------------------------
（３）カスタマイズ

-PIC18Fファームウェアにユーザー専用の機能を簡単に追加するには、 usercmd.c
を書き換える方法があります。

-picmonit.exe から user <arg> コマンドにて実行されます。



-------------------------------------------------------------------------------------
（４）I/Oポート監視

 PIC> p  コマンドでI/Oポートの状態監視と変更が可能です。
 
例： port状態一覧
 PIC> p
 
例： LEDの点灯
 PIC> p latc 3
 
例： LEDの消灯
 PIC> p latc 0
 
   PORT[ABC] LAT[ABC] TRIS[ABC] の意味はデータシートを確認してください。
 
 
-------------------------------------------------------------------------------------
（５）モニタースクリプト
 PIC> プロンプト状態でのコマンド投入をバッチファイルで自動化できます。
 コマンドシーケンスを適当なテキストファイル(SCRIPT.TXT)に書き込んで、

 D:>  picmonit.exe -iSCRIPT.TXT

 を実行すると、一連のシーケンスを実行します。


-------------------------------------------------------------------------------------
（６）アナログオシロ

  PIC> graph analog　　コマンドによって、低速アナログオシロになります。



-------------------------------------------------------------------------------------
（７）回路図

      pic18spx互換です。


-------------------------------------------------------------------------------------
（８）Linux版

      ubuntu,debian などの場合 libusb0-dev ( 或いはlibusb-dev )パッケージが必要です。
      firmware は DOS/Linux共通です。
      picmonit は 再コンパイルしてください。
      picmonit の実行はルート権限で行うか、あるいは udev で
      該当USBデバイスのアクセス権限をユーザーに許可するようにします。

      Linux版はグラフィック部分が使用できません。

-------------------------------------------------------------------------------------
（９）その他

      picmonit/test/ は、pic18ctl.dll （旧hidmon.dll相当） の呼び出しサンプルです。
      今のところＣ言語のみでテスト中です。
      pic18ctl.dll は picmonit/ ディレクトリに生成されたものを test/ にコピーして
      動作試験を行ってください。


-------------------------------------------------------------------------------------
（１０）その他

      赤外線リモコン解析機能も（たぶん）使えるはずです。pic18spxのドキュメントを
　　　参照してください。




-------------------------------------------------------------------------------------
