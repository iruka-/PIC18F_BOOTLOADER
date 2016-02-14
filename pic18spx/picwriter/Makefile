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
WIN32LIB= -lkernel32 -luser32 -lgdi32 -lwinmm -lsetupapi 
 else
USBFLAGS = -D_LINUX_
EXE_SUFFIX =
ICON_O	= hidasp_unix.o info.o
WIN32LIB= -lusb
 endif



USBFLAGS += -DHIDMON=1


CC = gcc
RC = windres
CFLAGS	= $(USBFLAGS) -O2 -Os -Wall
#CFLAGS	= $(USBFLAGS) -g -O2 -Wall

LIBS	= $(USBLIBS) -lm

TARG = picwrite
OBJS = util.o portlist.o picpgm.o picboot.o picdevice.o $(ICON_O)
#icon.o

PROGRAM = $(TARG)$(EXE_SUFFIX)

all: $(PROGRAM)

.c.o:
	$(CC) $(CFLAGS) -c $<

icon.o:	icon.rc
	$(RC) -i icon.rc -o icon.o

$(PROGRAM): $(OBJS)
#	$(CC) -o $(PROGRAM) $(OBJS) $(WIN32LIB) -lsetupapi $(LIBS)
	$(CC) -s -o $(PROGRAM) $(OBJS) $(WIN32LIB) $(LIBS)

strip: $(PROGRAM)
	strip $(PROGRAM)

clean:
	rm -f *.o $(PROGRAM)

test:
	picwrite -r

install:
	install -c $(PROGRAM) ../bin
#
