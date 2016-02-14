#################################################################
#	Windows用です.
#	コンパイラは MinGw gccを使用します。
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



#============================
 ifdef MSDOS
OSFLAGS = -D_MSDOS_
EXE_SUFFIX = .exe
DRIVER	= driver.o
WIN32LIB= -lkernel32 -luser32 -lgdi32 -lsetupapi 
 else
#============================
OSFLAGS = -D_LINUX_
EXE_SUFFIX =
DRIVER	= driver.o driver_unix.o 
WIN32LIB= 
 endif
#============================



USBFLAGS += -DHIDMON=1


CC = gcc

LIBUSBDIR = ../libusb

 ifdef MSDOS
LIBUSBINC = -I$(LIBUSBDIR)/include/
LIBUSBLD  = -L$(LIBUSBDIR)/lib/gcc/
 endif

CFLAGS	= $(OSFLAGS) -O2 -Wall $(LIBUSBINC)

LIBS	= $(LIBUSBLD) -lusb -lm


#################################################################
# picmonit.exe
#
TARG1 = picmonit
PROGRAM1 = $(TARG1)$(EXE_SUFFIX)
OBJS1 = monit.o analog.o util.o gr.o portlist.o gpopcode.o gpdis.o infra.o $(DRIVER)

 ifdef MSDOS
#################################################################
# pic18ctl.dll
#
TARG2 = pic18ctl
PROGRAM2 = $(TARG2).dll
OBJS2 = pic18ctl.o util.o portlist.o driver.o
 endif

#################################################################
# default target
#
all: $(PROGRAM1) $(PROGRAM2)
#all: $(PROGRAM1) 
#all: $(PROGRAM2) 


#################################################################
# build rule
#
.c.o:
	$(CC) $(CFLAGS) -c $<

icon.o:	icon.rc
	$(RC) -i icon.rc -o icon.o

#################################################################
# Debug時は -s を指定しない
#
$(PROGRAM1): $(OBJS1)
	$(CC) -s -o $(PROGRAM1) $(OBJS1) $(WIN32LIB) $(LIBS)

$(PROGRAM2): $(OBJS2)
	dllwrap -mwindows -mno-cygwin --enable-stdcall-fixup --add-stdcall-alias -o $(PROGRAM2) $(OBJS2) $(WIN32LIB) $(LIBS) -Wl,--out-implib,$(TARG2).lib

#-lsetupapi 

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
