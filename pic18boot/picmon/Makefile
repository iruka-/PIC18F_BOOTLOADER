#################################################################
#	Windows用です.
#
#	コンパイラは MinGw gccを使用します。
#	試してはいませんがcygwinでも大丈夫なはずです。
#	(cygwinでは、コンパイルオプション -mno-cygwinを付けます)
#	DEBUGの時には、-gをつけ、-sを外します。
#################################################################
# REMOVE DEFAULT MAKE RULE
MAKEFLAGS = -r



#============================
# DOSかどうかチェック.
 ifdef ComSpec
MSDOS=1
 endif

 ifdef COMSPEC
MSDOS=1
 endif
#============================

 ifdef MSDOS
USBFLAGS = -D_MSDOS_
EXE_SUFFIX = .exe
ICON_O	= hidasp.o icon.o
WIN32LIB= -lkernel32 -luser32 -lgdi32 -lsetupapi 
 else
USBFLAGS = -D_LINUX_
EXE_SUFFIX =
ICON_O	= hidasp_unix.o info.o
WIN32LIB= -lusb
 endif



USBFLAGS += -DHIDMON=1


CC = gcc
RC = windres
CFLAGS	= $(USBFLAGS) -O2 -Wall
#CFLAGS	= $(USBFLAGS) -g -O2 -Wall

LIBS	= $(USBLIBS) -lm

TARG1 = picmon
TARG2 = picboot

#################################################################
OBJS1 = monit.o analog.o util.o gr.o portlist.o gpopcode.o gpdis.o infra.o $(ICON_O)
OBJS2 = picboot.o util.o $(ICON_O)

PROGRAM1 = $(TARG1)$(EXE_SUFFIX)
PROGRAM2 = $(TARG2)$(EXE_SUFFIX)

#################################################################
all: $(PROGRAM1) $(PROGRAM2)

.c.o:
	$(CC) $(CFLAGS) -c $<

icon.o:	icon.rc
	$(RC) -i icon.rc -o icon.o

#################################################################
# Debug時は -s を指定しない

$(PROGRAM1): $(OBJS1)
#	$(CC) -o $(PROGRAM1) $(OBJS1) $(WIN32LIB) -lsetupapi $(LIBS)
	$(CC) -s -o $(PROGRAM1) $(OBJS1) $(WIN32LIB) $(LIBS)

$(PROGRAM2): $(OBJS2)
#	$(CC) -o $(PROGRAM2) $(OBJS2) $(WIN32LIB) -lsetupapi $(LIBS)
	$(CC) -s -o $(PROGRAM2) $(OBJS2) $(WIN32LIB) $(LIBS)

#################################################################
test:
	$(PROGRAM2) ../src/bootloader.hex

#################################################################
test-reset:
	$(PROGRAM2) -r ../src/bootloader.hex

#################################################################
selfupdate:
	$(PROGRAM2) -B ../src/bootloader.hex


#################################################################
clean:
	rm -f *.o

cleanall:
	rm -f *.o $(PROGRAM1) $(PROGRAM2)

#install:
#	install -c $(PROGRAM1) ../bin
#
#################################################################
###
analog.o: gr.h analog.h
gpdis.o: gpdis.h gpopcode.h portlist.h
gpopcode.o: gpopcode.h
gr.o: gr.h
hidasp.o: usbhid.h hidasp.h avrspx.h ../firmware/hidcmd.h monit.h
infra.o: monit.h ../firmware/hidcmd.h hidasp.h util.h
monit.o: monit.h ../firmware/hidcmd.h hidasp.h util.h gr.h analog.h portlist.h
picboot.o: monit.h ../firmware/hidcmd.h hidasp.h util.h
portlist.o: monit.h ../firmware/hidcmd.h hidasp.h portlist.h util.h \
portlist_2313.h portlist_mega88.h portlist_pic18.h portlist_14k50.h
util.o: hidasp.h monit.h ../firmware/hidcmd.h portlist.h util.h
