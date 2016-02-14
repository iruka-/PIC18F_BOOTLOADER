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
#DEVICE	= 18F2550
DEVICE	= 18F14K50

#######################################################
#	bootload の常駐番地
#   本番設定=0000   
#   他のloaderを使用してデバッグ=0800

VECTOR=0000
#VECTOR=0800
#VECTOR=1000

.PHONY: all clean print

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


CC = $(EXEC_PREFIX)mcc18.exe
LD = $(EXEC_PREFIX)mplink.exe
AR = $(EXEC_PREFIX)mplib.exe
ASM		= $(EXEC_PREFIX)mpasmwin.exe
LINK	= $(EXEC_PREFIX)mplink.exe
MP2HEX	= $(EXEC_PREFIX)mp2hex.exe

RM = rm

TARGET = loader-$(DEVICE)



##########################################
#  Set mcc18 INSTALL PATH
#
ifndef MCC18
MCC18 = C:/MCC18
endif

SRCS		= boot.asm usb.asm usb_desc.asm vectors.asm flash.asm hidmon.asm
INCS		= boot_if.inc boot.inc io_cfg.inc usb_defs.inc usb_desc.inc usb.inc 
OBJS		= $(foreach f, $(SRCS), $(basename $(f)).o )

COF			= $(TARGET).cof
HEX			= $(TARGET).hex
MAP			= $(TARGET).map

#MPLAB_DIR	= "C:\Program Files\Microchip\MPASM Suite\"
LKR			= sd$(DEVICE)-$(VECTOR).lkr

ASMFLAGS	=

all: $(HEX)

$(HEX): $(COF)
	$(MP2HEX) -q $(COF)

$(COF): $(OBJS)
	$(LINK) -p$(DEVICE) -q $(LKR) $(OBJS) -m$(MAP) -o$@

%.o: %.asm $(INCS)
	$(ASM) -q -p$(DEVICE) -dVECT=0x$(VECTOR) $< -e -l -o$@ $(ASMFLAGS)

clean:
	rm -f *.o *.ERR *.LST *.map *.cod *.cof
# *.hex

print:
	@echo "SRCS = $(SRCS)"
	@echo "INCS = $(INCS)"
	@echo "OBJS = $(OBJS)"

dis:
	gpdasm -y -p 18f2550 $(TARGET).hex >1
	-tabconv -d8 1 2
#	 $(TARGET).dis

dump:
	srec2bin $(TARGET).hex 1 >2

#
