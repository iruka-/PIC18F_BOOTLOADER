■ 概要

  これは、MicroChipが提供している PIC18 のUSBアプリケーションフレームワーク
  に含まれているHID-Bootloader をもとにして作成した PIC18F専用の簡易モニタです。

■ 現在のステータス

  （１）PIC 18F2550/18F4550/18F14K50 上で動作します。
  （２）picmonit.exe が使えます。
  （３）hidaspx.exe の仮バージョンpicspx-gccが動きます。
		現在は書き込みエラーが多発します。---> ハードウェアSPIを使うとデータ化けが発生
		するようです。現在はソフトウェアSPIで回避しています。
  （４）picwriter の仮バージョンが動きます。
        picwriterは LVP書き込みのできるPICマイコンにファームを書き込みことが出来ます。
  （５）PIC24F用のwriter24.exe (仮バージョン)が動きます。

■ 試し方

  まず、picboot を使用してファームウェアを書き込みます。
  C:> picboot -r firmware/picmon-18F2550.hex

  次に、picmonitor を起動します。
  C:> cd picmon
  C:picmon> picmonit.exe 

  現在実装されているコマンドの主な使い方です。

TARGET DEV_ID=25

メモリーダンプ(SRAM)
PIC> d
000000 00 00 40 00 20 78 02 00
000008 08 41 22 68 05 00 07 20
000010 bf 04 02 80 ac 40 84 41
000018 5d 44 88 83 05 03 c4 41
000020 fe 20 85 84 0c e9 24 cb
000028 36 50 8c 22 22 f8 51 21
000030 04 44 25 dc 02 c0 31 14
000038 81 09 48 d4 43 10 d6 35

逆アセンブル(FLASH)
PIC> l 0
0000 6af7        clrf   0xf7, 0
0002 6af8        clrf   0xf8, 0
0004 d054        bra    0xae
0006 d330        bra    0x668
0008 ef04 f004   goto   0x808
000c d05d        bra    0xc8
000e d27b        bra    0x506
0010 d328        bra    0x662
0012 d32c        bra    0x66c
0014 d32f        bra    0x674
0016 d345        bra    0x6a2
0018 ef0c f004   goto   0x818
001c 0112        movlb  0x12
001e 0200        mulwf  0, 0
0020 0000        nop
0022 4000        rrncf  0, 0, 0
0024 04d8        decf   0xd8, 0, 0
0026 003c        dw     0x3c  ;unknown opcode
PIC> q
Bye.
C:>

その他のコマンド
PIC> ?
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
 q                           Quit to DOS
------
Topics:

bootコマンドにアドレス引数を与えることが出来ます。

例えば、bootloaderに戻りたい場合は boot 0
0x2c00～に焼いたテストプログラムを実行したい場合 boot 2c00
とします。

runコマンドとの違いは、USBバスを一旦リセットすることです。

runコマンドでの実行は、USB接続のままで、goto命令が発行されます。

graphコマンドは、監視ポート名を第一引数で与えます。
graphコマンドの第二引数は以下のようなものを用意しています。
 PIC> graph porta trig 0   ＜＝＝＝ portaのbit0 をトリガーにする。
 PIC> graph porta infra    ＜＝＝＝ porta bit0に赤外線リモコン受信素子が
                　接続されている前提で、家電協コードを解析して表示します。
 PIC> graph analog         ＜＝＝＝ AINをアナログ表示します。

graphコマンドのサンプリングレートは、PIC18F14K50の場合およそ1mS固定になります。
PIC18F2550/4550の場合は10kHz割り込みサンプリングを行います。
（14K50で割り込みサンプリング出来ないのは、バッファメモリー容量の事情です）

サンプリングレートの変更は PR2レジスタの書き換えで行ってみてください。
 PIC> p pr2 17   など.



■ ディレクトリ構成

 +- picspx/ -+-- picspx/     AVRライター    (gcc)
             |
             +-- picwriter/  PIC18Fライター (gcc)
             |
             +-- writer24/   PIC24Fライター (gcc)
             |
             +-- picmon/     簡易モニター   (gcc)
             |
             +-- firmware/   ファームウェア (mcc18)



■ 回路図 (PIC 18F4550)

circuit.txt を参照してください



■ AVRライターとしての試し方

18F2550/18F4550の４本のSPI信号を書き込み先のAVRチップに接続します。

18F4550上のpin番号:信号名  = SPI端子(AVRチップ上のピン名称)
			-----------------------------
  				25:RC6=TX  = Reset
 				33:RB0=SDI = MISO
  				34:RB1=SCK = SCK
  				26:RC7=SDO = MOSI
			-----------------------------

C:> cd picspx
C:picspx> picspx-gcc.exe  -r

のようにして使います。

C:picspx> picspx-gcc.exe だけを入力すると使い方の表示が出ます。

picspx-gcc.ini が同じディレクトリにない場合は、少なくとも -ph
オプションを手動で指定する必要があります。





■ PICライターとしての試し方

  接続:

 AVR用ISP 6PIN          書き込み対象 PIC18F2550/14K50
------------------------------------------------------------
	1 MISO    ------------------>   PGD
	2 Vcc     ------------------>   Vcc
	3 SCK     ------------------>   PGC
	4 MOSI    ------------------>   PGM
	5 RESET   ------------------>   MCLR
	6 GND     ------------------>   GND

 ・picwriter/picwrite.exe -r 
 　を実行して、PICのデバイスIDが表示できれば、接続はうまくいっています。
 ・LVP書き込みモードのないPICや、既にHVP(高電圧)書き込み済みのPICは
   このPICライターでは読み書きできません。
 ・LVP書き込み後のマイコン使用時は常時PGM端子を「プルダウン」して使用する
　 必要があります。

==================================================
■ hidmon-14k50 や hidmon-2550との違い

 ・一応、Ｃ言語で書かれています。(mcc18)

 ・コードサイズが大きいです。(8kB程度)

 ・USB のPIDが違います。

 ・プロトコルも違います。

 ・HID Reportの転送方法が異なります。
   hidmonでは、全てコントロール転送(HidD_SetFeature/HidD_GetFeature)
   で行っていますが、picmonit.exeでは、EndPoint1に対するインタラプト転送
   によりデータの受け渡しを行っています。

   このため、インタラプト転送パケットがデバイスより送出されない限りは
   HID Reportを受け取ることができません。（ホストがタイムアウトになります）
   送出された場合は、必ず受け取っておかないと、次パケットの受信時に、バッファ
　 にたまった古いパケットを受け取って混乱することになります。


■ 何に使えますか？

 ・picmonit.exe 上のコマンドと、firmware上のコマンド受け取り処理の対を
 　自分で追加することにより、いろいろな機能を追加することが出来ます。


■ 応用１
   -------------------------------------------------------------------------
   PS/2 タイプのキーボードを繋いで、打鍵されたキーコードを１６進ダンプします
   -------------------------------------------------------------------------

   接続：
     
     PORTB.bit0 <---- PS/2 KeyBoard CLK
     PORTB.bit1 <---- PS/2 KeyBoard DATA

   使い方：
     （１）firmware/picmon-18F2550.hex を焼きます。
     （２）ＰＣ上にて、picmon/picmonit.exe を起動します。

     普通にメモリーダンプ('D'コマンド)や逆アセンブル('L'コマンド)が使えることを
     確認します。
     
     （３）userコマンドを入力します。

     PIC> user
     
       そして、繋いだPS/2キーボードを打鍵すると、コンソールに１６進でコード表示が
       出ます。
       
       ８０個のコードが表示されるか、もしくはパソコン側から[ESC]キーを押したら終了です。


■ 応用２
   -------------------------------------------------------------------------
   PIC上からprintfやputsを試せます。
   -------------------------------------------------------------------------

   firmware/Makefile に記述している OBJ項の keybtest.o を usercmd.o と置き換えます。
   firmwareディレクトリで make clean して make します。
   firmware/picmon-18F2550.hex を焼きます。
   ＰＣ上にて、picmon/picmonit.exe を起動します。
   そしてuserコマンドを入力します。
     PIC> user
   これで、PIC上からprintされた文字をＰＣ上で見ることが出来ます。
   
   usercmd.c をいろいろ書き換えて試すことが出来ます。


■ 応用３
   -------------------------------------------------------------------------
   赤外線リモコンの受光波形などを観測することが出来ます。
   -------------------------------------------------------------------------
   リモコン受信器(38kHz)の出力端子をPortA.bit0に繋いで、

 PIC> graph porta
フリーランモードで観測します。

 PIC> graph porta trig 0
porta bit0の変化によるトリガーで観測します。

 PIC> graph porta infra
porta bit0の変化によるトリガーで観測したのち、解析結果をコンソール表示します。



■ 応用４
   -------------------------------------------------------------------------
   PIC24Fデバイスのファームウェア書き込みが出来ます。
   -------------------------------------------------------------------------
   PIC18F14K50を3.3V動作で使用します。
   PIC24F専用の書き込みツールは writer24/ ディレクトリにあります。
   ターゲットのPIC24Fを接続したら、

   C:picspx> writer24\writer24.exe -r

   これにより、PIC24Fデバイス名とデバイスID,リビジョンNoが表示されれば接続はOKです。
   
   ＬＥＤチカチカサンプル(実際にはPGD1をToggleしています)
      writer24/ledtest24/main.hex
   これをビルドするにはMICROCHIPの C30コンパイラが必要です。


