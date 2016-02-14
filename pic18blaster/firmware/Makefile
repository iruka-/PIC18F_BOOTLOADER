#
##########################################
#  GNU Makefile for PIC MCC18 C Compiler
##########################################
#  使い方:
#  ・WinAVR でおなじみの GNU Makeを使ってビルドします.
#    (make.exe : 通常は C:/WinAVR/utils/bin/ にあります.)
#
#  ・mcc18は MPLAB-C18-Std-Eval-v3_32.zip を使用しました.
#    (インストール先: 通常は C:/mcc18/bin/mcc18.exe )
#
#  ・ビルド環境がLinuxの場合も GNU Makeによりビルド出来ます.
#    Linuxの場合はwineをあらかじめインストールしてください.
#    $ wine ./srec2bin.exe などとやって、 ~/.wine/ディレクトリを作成した
#    あとで、~/.wine/drive_c/mcc18/ のようにディレクトリを掘って、そこに
#    Windows側の C:\mcc18 をそのままコピーして使います.
#
#  ・パスの通し方:
#    PATH %PATH%;C:\mcc18\bin;C:\mcc18\mpasm;C:\WinAVR\utils\bin;
#                ~~~~~~~~~    ~~~~~~~~ 下線部はmcc18のインストール先に応じて読み替えてください.
#    Linuxの場合はパスを通す必要はありません.
#
#  ・MPLABのほうは特にインストールする必要はありません.
#    (mcc18にmpasmが含まれています)
#
#  ・Windowsで、mcc18のインストール先が異なる場合は、下記
#    MCC18 = C:/MCC18
#   を書き換えるか、環境変数 「MCC18」 にインストール先を記述してください.
#

#######################################################
# デバイス種別:
DEVICE=18F14K50
#DEVICE=18F2550
#DEVICE=18F4550


#######################################################
# 動作環境判別: Linux / Windows
#
 ifdef ComSpec
EXEC_PREFIX=
EXE_PREFIX=
 else

 ifdef COMSPEC
EXEC_PREFIX=
EXE_PREFIX=
 else
EXEC_PREFIX=wine ~/.wine/drive_c/mcc18/bin/
EXE_PREFIX=wine ./
 endif

 endif

AS = $(EXEC_PREFIX)MPASMWIN.exe
CC = $(EXEC_PREFIX)mcc18.exe
LD = $(EXEC_PREFIX)mplink.exe
AR = $(EXEC_PREFIX)mplib.exe
RM = rm


TARGET = picmon-$(DEVICE)


##########################################
#  Set mcc18 INSTALL PATH
#
ifndef MCC18
MCC18 = C:/MCC18
endif

##########################################
#  Object List
#
OBJS = \
	main.o		\
	monit.o		\
	picwrt.o	\
	timer2.o	\
	print.o		\
	usercmd.o	\
	usbdsc.o	\
	usb/hid.o	\
	usb/usb9.o	\
	usb/usbctrltrf.o	\
	usb/usbdrv.o		\
	usb/usbmmap.o		\

#	ps2keyb.o	\
#	keybtest.o	\


##########################################
#  Compile Option
#
# EEPROMにシリアル番号を設定する場合
CFLAGS = -DRAM_SERIAL -p=$(DEVICE) /i$(MCC18)/h /iusb

# ROMにシリアル番号を埋め込む場合
#CFLAGS = -p=$(DEVICE) /i$(MCC18)/h /iusb
#CFLAGS = -DRAM_SERIAL -p=$(DEVICE) /i$(MCC18)/h /iusb -w=1
#CFLAGS = -DRAM_SERIAL -p=$(DEVICE) /i$(MCC18)/h /iusb -w=2

# ↑: 注意  警告レベルを1に下げていますが、元に戻すほうが望ましい.


##########################################
#  Rules
#
%.o : %.c
	$(CC) $(CFLAGS) $< -fo=$@


##########################################
#  Build Target
#
$(TARGET).cof : $(OBJS)
	$(LD) -p$(DEVICE) /l$(MCC18)/lib /k$(MCC18)/lkr rm$(DEVICE).lkr $(OBJS) \
	/u_CRUNTIME /z__MPLAB_BUILD=1 /i /m$(TARGET).map /o$(TARGET).cof



# v3.34まではこちら.
#	$(LD) 			  /l$(MCC18)/lib /k$(MCC18)/lkr rm$(DEVICE).lkr $(OBJS) \
#	/u_CRUNTIME /z__MPLAB_BUILD=1 /i /m$(TARGET).map /o$(TARGET).cof

##########################################
#  Dependency
#
monit.o : monit.c usi_pic18.h usi_pic18s.h


##########################################
#  Phony Target
#
all:
	$(MAKE) clean
	$(MAKE) DEVICE=18F14K50
	$(MAKE) clean
	$(MAKE) DEVICE=18F2550

clean : 
	-$(RM) $(OBJS) $(TARGET).cof $(TARGET).map
	-$(RM) *.cof *.map *.dmp *.cod *.lst *.err *.src

##########################################
#  HEXファイルをHEXダンプに変換する.
dumphex : 
	$(EXE_PREFIX)srec2bin.exe $(TARGET).hex  nul > $(TARGET).dmp

##########################################
#  LSTファイルを少しだけ見やすくする.
dumplst : 
	$(EXE_PREFIX)mplistconv.exe -m80 $(TARGET).lst $(TARGET).src

#[EOF]
