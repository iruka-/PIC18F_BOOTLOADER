□　LinuxおよびMacOS用のhidspxについて

　　　　　　　　　　　　　　　　　　　　　　　　　　　2009年 1月22日（初版）
　　　　　　　　　　　　　　　　　　　　　　　　　　　2010年 1月22日（改訂）

　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　千秋広幸

□　これは何か

 これは、Linux/MacOSの両方に対応したHIDaspx用の書き込みツール(hidspx)のソース
です。瓶詰堂さんが公開されているソースを元に編成しました。

□　関連資料

　このアーカイブは、実行ファイルを作成に必要な最小限のソースのみを提供します。
したがって、別途、Windows向けのアーカイブを入手してください。そちらには、回路
図や説明書、ファームウェアが同梱されています。(hidspx-2009-****.zip)

□　インストール方法

Linux用の実行ファイルを作成するには

srcディレクトリに移動し、

 $ make
 $ sudo make install

でインストールできます。

MacOSでmakeする場合には、以下のように操作します。

 $ make -f Makefile.macos
 $ sudo make install

インストール先は、/usr/local/bin となっていますが、他を希望する場合はMakefile
のINSTALL_DIRを修正してください。HIDSPXを変更すれば、hidspxのコマンド名も変更
可能です。

fuse.txtの文字コードは現在はUTF8としていますが、他の文字コードを利用している
場合には、利用環境に合わせて、コード変換を行ってください。コード変換には、
iconvやnkfコマンドが利用できます。

□　特徴

 Windows版のhidspxにほぼ同様の使い方が可能ですが、Windows版 よりもサポートし
ているプログラマは限られます。しかし、必要な機能はすべて備えており、AVRマイコ
ンのプログラマとして利用できます。

□　改良点
 HIDSPX環境変数に /usr/local/bin （あるいは hidspx.ini, fuse.txt の設置
場所）を指定すれば、そのファイルの内容を参照し、各種の設定を行うことができます。

□　HIDaspxのファームウェアについて
 2008年11月27日～2009年1月21日までのファームウェアでは、このソフトウェアは機能
しません。 最新版のソースでお使いください。

□　確認済みの点（改良が望まれる点）

* HIDaspxを接続時、dmesgに以下のメッセージが出力される

usb 1-2: configuration #1 chosen from 1 choice
/build/buildd/linux-2.6.24/drivers/hid/usbhid/hid-core.c: \
　couldn't find an input interrupt endpoint

実害はありませんのでこのままお使いください。なお、これが気になる方は、HIDaspx
のファームウェアをmain_libusb.hexに変更すれば、このメッセージは回避できます。
利用時には、-phu を指定します。しかし、この変更を実施するとWindowsでは使えな
くなります。できるだけ「main-12.hex」ファームでご利用ください。

※ 2009-0123版以降で、-phu （HIDaspxのlibusb対応オプション）を追加しました。

------------------------------------------------------
□　一般利用者権限でhidspx,hidmonを使う方法
------------------------------------------------------

■ Vine Linux 4.2の場合
      root 権限で、/etc/udev/rules.d/95-my-permissions.rules に以下の内容を
      書き込みます。

SUBSYSTEM=="usb_device",SYSFS{idVendor}=="16c0",SYSFS{idProduct}=="05dc",MODE="0666"
SUBSYSTEM=="usb_device",SYSFS{idVendor}=="16c0",SYSFS{idProduct}=="05df",MODE="0666"

      その後、HIDaspxデバイスを差し込んでAVRの読み出しに成功しました。

■ Ubuntu 8.10の場合

      Ubuntu 8.10で動作させるには、好みのエディタ（たとえば gedit）で

      sudo gedit /etc/udev/rules.d/95-my-permissions.rules

      以下の内容を書き込みます。（参考までに AVR マイコン関連の udev 情報を
      書いておきます。利用したいものがあれば、追加してください）

# ADD by senshu

# USBasp(OLD)
SUBSYSTEM=="usb",SYSFS{idVendor}=="03eb",SYSFS{idProduct}=="c7b4",MODE="0666"
# USBasp(NEW), HIDaspx (libusb mode)
SUBSYSTEM=="usb",SYSFS{idVendor}=="16c0",SYSFS{idProduct}=="05dc",MODE="0666"

# HIDaspx (HID mode)
SUBSYSTEM=="usb",SYSFS{idVendor}=="16c0",SYSFS{idProduct}=="05df",MODE="0666"

###
#AVRISP mkII
SUBSYSTEM=="usb",SYSFS{idVendor}=="03eb",SYSFS{idProduct}=="2104",MODE="0666"
# AVR Dragon
SUBSYSTEM=="usb",SYSFS{idVendor}=="03eb",SYSFS{idProduct}=="2107",MODE="0666"

      この設定後、

      sudo /etc/init.d/udev restart

      でデバイスを再認識させます（rule ファイルを修正時のみ必要）。利用者権
      限で hidspx, hidmon コマンドが正常に動作することを確認しました。

■ Ubuntu 9.10の場合

　利用者から、「指定しなくとも、sudo make install するだけで利用が可能」との
報告をいただいています。お試しください。

------------------------------------------------------
□　制限事項と使い方
------------------------------------------------------

Linux版では、現在のところ、シリアル番号の指定ができません。hidmonでは指定可能です
から、AVRライタとして利用するHIDaspxを'0000'にしておくことで、とりあえずは混乱無く
利用できます。

 hidspx -ph:XXXX

□　お願い
 独自の改良を行われた方は、ぜひご一報ください。

□　改訂履歴

☆2009-0122 初版リリース

☆2009-0123 make install 時の権限を修正、libusbモードへの対応を追加

☆2009-0124 以下のメッセージ出力を抑止（谷岡さんの作成したパッチを元にしています）
　「usb 1-2: usbfs: process 9811 (hidspx) did not claim interface 0 before use」

☆2009-0126 Linux版でも、-ri, -rd, -rF, -rl, --new-mode, --show-options を追加
 利用するWebブラウザは、ターミナルからの使用を想定し、URLを標準出力に
 表示します。w3m, lynx などに渡すことでブラウザで表示可能です。
 gnome端末を利用している場合には、右クリックで「URLを開く」を選ぶことで、
 firefoxなどを利用することができます。

☆2009-0127 特権モードでの実行を必要な部分に限定する修正を実施

☆2009-0128 実行時間の表示を追加（コード提供してくれた谷岡さんに感謝します）

 ◇hidspxの使用例と結果表示

 $ hidspx read.hex -d1 (実行コマンド)
 #=> hidspx -d4 -ph --new-mode read.hex -d1
 Detected device is ATtiny2313.
 Erase Flash memory.
 Flash memory...
 Writing   [##################################################]   2016,   1.26s
 Verifying [##################################################]   2016,   0.99s
 Passed.
 Total read/write size = 4032 B / 2.70 s (1.46 kB/s)

この表示により、ReadよりもWriteに時間が必要なことや、(1.26+0.99) != 2.70 の理由ですが、
Eraseやハードウェアの認識にも若干の時間が必要なこともわかります。

☆2009-0202  udev rules への登録方法の説明を追加

☆2009-0317 -rIオプションの追加（FUSE Calcの新バージョンに対応）

☆2009-0318 -oオプションの追加（出力ファイル名の指定、Windowsに合わせました）

☆2009-0514 -! 、ユーザーブックマークオプションを追加(hidspx-GUIからの利用も可能)
※　hidspx-GUIからの利用では、一時ファイルの削除に失敗する不具合があります。


☆2009-0527 うえまさんからの報告を受け、MacOSでコンパイルした時のエラーを修正しました。

☆2009-0818 FUSEビット（RSTDISBLとDWENの保護）を行うように、修正しました。

☆2009-1210 fuse.txt, fuse_en.txtの修正(ATmega328Pなどへの対応)
            --show-spec オプションの追加

☆2010-0125 エラーメッセージのミスタイプ(fonud => foundに修正)

