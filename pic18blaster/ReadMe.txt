■ 概要

   これは、OpenOCD の DLL ハック（実験）です。
   即ち、外部に DLL を置いて、JTAGアダプターのドライバーの分離実装を試みています。

■ 現在のステータス

   とりあえずPIC18F14K50を使用したJTAGアダプターが動いています。


■ 試し方

   WindowsXPを用います。

   下記PIC18spxファームを焼いたPIC18F14K50(もしくはPIC18F2550)基板と、
   JTAGが使用できる適当なARM基板を以下のような対応でJTAG接続しておきます。

   pic18blaster/firmware/picmon-18F14K50.hex

 結線は、
   (PICライター機能の)         ARM基板 JTAG端子
           MCLR  ------------- TMS
           PGM   ------------- TDI
           PGD   ------------- TDO
           PGC   ------------- TCK
 となります。これ以外のnTRSTピンなどはまだサポートしていません。(USB-Blasterと同様)

 ARM側は3.3Vなので、PIC側の電圧に注意してください。(5Vは危険です)
 PIC側にPIC18F2550を用いるときは、電圧変換の必要が生じます。
 参考例としては、USB-Blaster（もどき）の実装における、抵抗分圧を利用する方法と、
 あまりお勧めしませんが、PIC18F2550を3.3V駆動する方法などがあります。(18LF2550を使えば規格内です)

   hidblast/ ディレクトリの ocd.bat あるいは、 ocd2.bat を起動して、openocd.exeの吐き出すメッセージ
   を確認することが出来ます。

   正常に接続出来ているようでしたら、telnetで localhost:4444 番に接続して、OpenOCDコマンドを実行して
   みてください。

   STM8S-Discovery用のファームを焼いてみるテスト用に a.bat とmain-0000.hex を含めています。


■ ディレクトリ構成

 pic18blast-+- ソース.
            |
            +--helper\   ヘッダーファイル.
            +--jtag\     ヘッダーファイル.
            |
            +--openocd_patch\  openocd本体側作成用の改造点
            |
            +--firmware\       PIC18F14K50用ファームウェア.
                (安定性向上のためpic18spxファームから、割り込み関連をDisableしたものです)



■ プログラムの再ビルド方法

   WindowsXP上のMinGW-gccコンパイラを用いてmakeしてください。
   makeすると、hidblast.dll が作成されます。

   openocd.exe本体を再ビルドする方法は、以下のURLを参照してください。

-http://hp.vector.co.jp/authors/VA000177/html/2010-09.html
   
   今回の改造部分ソースはopenocd_patch/ ディレクトリに置いています。

   Linux上でのビルドオプションは、こんな感じです。
   $ ./configure \
       --build=i686-pc-linux-gnu \
       --host=i586-mingw32msvc \
       --enable-dummy

   出来上がった openocd.exe 本体は、ドライバーとして、同一ディレクトリに存在する hidblast.dll を
   起動時に呼び出します。(存在しなければ、dummyドライバーのみが組み込まれます)


■ 現状の問題点

   多少不安定です。--->ファームを更新することで安定化しました。
   PIC18F14K50で不安定な場合は、PIC18F2550用をお試しください。

■ ライセンス

   OpenOCDの配布ライセンスに準じます。


■ 展望

   hidblast.dll ファイルを(自力で)差し替えるだけで、自作デバイスがサポート可能になります。

   （たとえばATtiny2313を使用したJTAGアダプターなどをサポート出来る可能性があります）

   hidblast.dll のエントリーポイントは、
      DLL_int get_if_spec(struct jtag_command **q);
   だけです。引数のstruct jtag_command **qのqには、openocd本体のjtag_command_queueという
   グローバル変数のアドレスを渡します。
   戻り値は、(intになっていますが) ドライバー記述構造体のアドレスになります。


