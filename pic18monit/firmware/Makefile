##########################################
#  GNU Makefile for PIC MCC18 C Compiler
##########################################

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


TARGET = main-$(DEVICE)


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
	main.o 	\
	monit.o \
	usercmd.o \
	print.o \
	timer2.o 	\
	fuse.o 	\
	memcpy64.o \
	usb_generic.o 	\
	usb_descriptors.o	\
	usb/GenericDeviceDriver/usb_function_generic.o \
	usb/usb_device.o	\

#	usb/CDCDeviceDriver/usb_function_cdc.o	\
#	usb/usb_device.o	\

#	usb_cdc.o	\
#	eusart.o		\

#

##########################################
#  Compile Option
#
#
CFLAGS = -DRAM_SERIAL -DUSB_CDC_SUPPORT_HARDWARE_FLOW_CONTROL -p$(DEVICE) /i$(MCC18)/h /iinclude
#CFLAGS = -p=$(DEVICE) /i$(MCC18)/h /iinclude -w=1

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
	/u_CRUNTIME /z__MPLAB_BUILD=1 /m$(TARGET).map /i /o$(TARGET).cof

##########################################
#  Dependency
#

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
	-$(RM) *.cof *.map *.dmp *.cod *.lst *.src

##########################################
#  HEXファイルをHEXダンプに変換する.
dumphex : 
	$(EXE_PREFIX)srec2bin.exe $(TARGET).hex  nul > $(TARGET).dmp

##########################################
#  LSTファイルを少しだけ見やすくする.
dumplst : 
	$(EXE_PREFIX)mplistconv.exe -m80 $(TARGET).lst $(TARGET).src

#[EOF]
