  -------------------------------------------------------------------
         HIDaspx を使用して PIC18FシリーズのFlashを読み書き.

                               PICspx
  -------------------------------------------------------------------

■ 概要

   これは USB 接続の AVR ライター HIDaspx のハードウェアをそのまま利用して、PIC18F
   への書き込みを行おうという企画です。

■ 何に使うのですか？

   PIC 18F2550/18F4550/18F14K50 など USB I/F 内蔵の廉価な PIC シリーズのマイコン
   にブートローダーを書き込むのに使用できたらいいなということで制作を始めました。


■ 用意するハードウェアは？

   （１）HIDaspx  AVRライター.

   AVRマイコンやHIDaspxに関しては 千秋ゼミさんのサイトに膨大な情報が蓄積されています。
   http://www-ice.yamagata-cit.ac.jp/ken/senshu/sitedev/index.php?AVR%2FHIDaspx00


   （２）PIC 18F2550/18F4550/18F14K50 （どれか）

    LVP 書き込みがOFFになっていると読み出せません。(未使用のチップはONになっています)

    書き込み済みのチップで LVP=OFF の場合は、HVP 書き込み可能なライターで一度消去
    してから LVP=ON で書き込んでください.

    「FENG3's HOME PAGE」のPIC18F14k50に関する記事の中に、006P乾電池とLVPプログラマ
    を使って、初期化する方法が紹介されており、PICspxを使って初期化することも可能です。

     LVPビット(ONにする方法)⇒ http://wind.ap.teacup.com/feng3/452.html

    また LVP=ON 状態のチップを実際に使用するときは、常時 PGM 端子を PullDown (5～
    10k Ω程度で OK です) しなければなりません。

■ 接続:

 AVR用ISP 6PIN                  PIC18F2550/14K50

	1 MISO    ------------------   PGD
	2 Vcc     ------------------   Vcc
	3 SCK     ------------------   PGC
	4 MOSI    ------------------   PGM
	5 RESET   ------------------   MCLR
	6 GND     ------------------   GND


■ PIC 書き込みツール

   src/ ディレクトリにあります。

   PICspx.exe
-------------------------------------------------------
* PICspx Ver 0.2 (Aug 18 2009)
Usage is
  PICspx.exe [Options] hexfile.hex
Options
  -p[:XXXX]   ...  Select serial number (default=0000). HIDaspxファームのシリアル番号指定.
  -r          ...  Read Info.    HIDaspxに繋がっているデバイスの情報を見る.
  -v          ...  Verify.       書き込みは行わず、ベリファイのみ実行する.
  -e          ...  Erase.        消去のみ行う.
  -rp         ...  Read program area.  FlashROM領域を読み出して hexfile.hex に出力する.
  -rf         ...  Read fuse(config).  Fuseデータを読み出してコンソールに表示する.
  -d          ...  Dump ROM.           FlashROM領域を読み出して コンソールに表示する.
  -nv         ...  No-Verify.    書き込み後のベリファイを省略する.

-------------------------------------------------------

■ PIC 書き込みツールの使い方

  WindowsXP から DOS 窓を開いて
  C:> picspx  -r
      ~~~~~~~~~~
  のように入力すると、接続しているPIC18F の品種を表示します。


  C:> picspx  bootloader-0000.hex
      ~~~~~~~~~~~~~~~~~~~~~~~~~~~
  のように入力すると、「bootloader-0000.hex」をPICに書き込むことが出来ます。


  C:> picspx  -rf
      ~~~~~~~~~~~
  のように入力すると、Fuse(config)の内容を読み出して コンソール表示します。


  C:> picspx  -d read.hex
      ~~~~~~~~~~~~~~~~~~~
  のように入力すると、FlashROMの内容を読み出して コンソールにダンプ表示します。

  C:> picspx  -rp read.hex
      ~~~~~~~~~~~~~~~~~~~~
  のように入力すると、FlashROM の内容を読み出して カレントディレクトリに「read.hex」
  のファイル名で出力します。（非常に時間が掛かります）



■ テストツール( このツールをデバッグする場合のみに使用してください )

   test/ ディレクトリにあります。HIDmon を改造して pgm コマンドを追加したものです。
   a.bat を起動すると PIC の flash ROM の内容 (先頭 64byte) と fuse(config),ID を
   読み出して16進表示します。


■ 現在のステータス

   消去済みのPIC 18F14K50に対して、
   HIDmon-14k のファームウェア bootloader-0000.hex を以下の手順
   C:> picspx  bootloader-0000.hex
       ~~~~~~~~~~~~~~~~~~~~~~~~~~~
   で書き込んで、HIDmon/HIDboot として動作することを確認しています。



   ポートの上げ下げに USB の 1 フレーム (1mS もしくは 4mS(UHCI)) の時間が掛かりま
   すので読み出しや書き込みは非常に遅いです。

   ブートローダーを書き込むのが目的なので、速度については目をつぶることにします。

   パソコン側の USB が 2.0(480Mbps) をサポートしている場合は、パソコンと HIDaspx
   ライターの間に USB 2.0 Hub(480Mbps) を挟むことで書き込みを高速化することが出来
   ます。

   あるいは、Intel/VIA の USB Host コントローラ (UHCI) を使う代わりに、SiS/AMD/NEC
   製の USB Host コントローラ (OHCI) （マザーボード、もしくは PCI カード）を使用
   することでも書き込みを高速化することが出来ます。

   高速化される理由は、USB 2.0(480Mbps) 規格で拡張されたマイクロフレーム (1mS の
   1/8 の時間での USB 応答サイクル) を使用できるためです。（通常は USB のフレーム
   時間は 1mS であり、UHCI ホストではコントロール転送に 4 フレーム (4mS) 消費する
   ので、毎秒 250 回しかポートコントロール出来ないのが遅くなる理由です）


----------------
2009/09/02 8:54:18

senshuの作業

1. 18F14k50で利用するEraseCommandのコマンド値を以下のように変更

	EraseCmd(0x0f0f,0x8787);break;
		↓
	EraseCmd(0x0f0f,0x8f8f);break;	// mod by senshu

2. PICspxの実行ファイルにアイコンを追加

3. binディレクトリを追加し、実行ファイルをここに配置

4. Sleep関数の精度を向上させる修正を追加

5. アイコンのデザインを微修正

6. PCtestフォルダを追加（Sleep関数の精度を確認するプログラム）
   test-pc.exe は、PICspx用に特化し、10m秒以下の待ちを500回繰り返し、
   Sleep関数が実際に要した時間（最小、最大、平均）を表示する。

   この版のPICspxでは、wait_ms関数経由で、以下の値でSleep関数を呼び出し
   ている。

   1mSec   ... PGM, PGCの操作
   2mSec   ... Flashの書き込み完了待ち（1mSecから変更、マージン確保のため）
   100mSec ... Config書き込み完了待ち
   300mSec ... チップ消去了待ち

   test-pcをコマンドプロンプト上で実行すると、約20秒後に以下のような
   結果を表示する。（実行環境により、表示する数値には違いがある）

   結果の見方は、Sleep functionに注目し、avgの値が左側の数値に近いことを
   確認する。（この結果程度のずれは許容範囲）

-----
 delay_ms function:
  10 -> min =   9, max =  29, avg = 10.17 [mSec]
   5 -> min =   5, max =  24, avg =  5.11 [mSec]
   2 -> min =   2, max =  19, avg =  2.03 [mSec] <--- ???
   1 -> min =   1, max =   1, avg =  1.00 [mSec]
   0 -> min =   0, max =   0, avg =  0.00 [mSec]

 Sleep function:
  10 -> min =   9, max =  22, avg = 10.07 [mSec]
   5 -> min =   4, max =  23, avg =  5.10 [mSec]
   2 -> min =   2, max =  20, avg =  2.04 [mSec] <--- ???
   1 -> min =   1, max =  19, avg =  1.04 [mSec] <--- ???
   0 -> min =   0, max =   0, avg =  0.00 [mSec]


7. LVP=ON に初期化する方法（リンク）を追加

8. testフォルダ内の hidmon.exe にも、wait_ms関数の精度向上パッチを適用

9. binフォルダにPIC18F14k50用のBootloaderを同梱


http://hp.vector.co.jp/authors/VA000177/html/hidmon-14kA4CEBBC8A4A4CAFD.html

から入手できるソースに、

以下の修正を加えてビルドしたHEXファイル(bootloader-mod-0000.hex)を追加

	CONFIG  PWRTEN = ON			; (OFF) PowerUp Timer

詳細は、↑のURLをご覧ください。

修正の理由は、リセット機能の安定化のため（未チェックです）。

