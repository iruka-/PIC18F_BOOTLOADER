■ 概要

   PIC18F2550/4550を使用して、赤外線リモコンの受光波形を観測することが出来ます。

■ 使い方


   （１）firmware/picmon-18f2550.hex を焼きます。
   （２）リモコン受信器(38kHz)の出力端子をPICのPortA.bit0に繋ぎます。
   （３）PICデバイスをUSB経由で接続した後、 picmonit.exe を起動します。
 
 C:picspx> picmon\picmonit.exe

 PIC> プロンプトが出たら、以下のコマンドを入力します。
 PIC> graph porta infra

   （４）リモコン受信器に向けて、家電協規格の赤外線リモコン信号を送ります。

 するとコンソールに、解析したビットを表示します。

 HDR:01000000_00000100_00000001_00000000_10001000_10001001_ HEX:40 04 01 00 88 89
 HDR:01000000_00000100_00000001_00000000_00001000_00001001_ HEX:40 04 01 00 08 09
 HDR:01000000_00000100_00000001_00000000_00001000_00001001_ HEX:40 04 01 00 08 09
 HDR:01000000_00000100_00000001_00000000_11001000_11001001_ HEX:40 04 01 00 c8 c9
 HDR:01000000_00000100_00000001_00000000_00101000_00101001_ HEX:40 04 01 00 28 29

■ HEXの内容について

 先頭２バイト＝メーカーコード
 次の３バイト＝リモコンボタンに応じた固有コード
 最後の１バイト＝ボタン固有コード３バイトのチェックサム（ＸＯＲ値???）


■ 家電協以外のリモコンは？

 現在のところ対応していません。（主にソニー、ＮＥＣ方式）
 対応方法としては、picmon/infra.c に追加機能的に実装することができると思います。




