#
#	Windows用です.
#
#	コンパイラは MinGw gccを使用します。
#	試してはいませんがcygwinでも大丈夫なはずです。
#
EXE_SUFFIX = .dll

O = .o
CC		= gcc
CFLAGS	= $(USBFLAGS) -DHAVE_CONFIG_H -std=gnu99 -I. -I./jtag -I./helper -O2 -Wall -mno-cygwin
LIBS	= $(USBLIBS)

TARGET = hidblast
OBJS  =  main.o hidblast.o bitbang.o commands.o interface.o log.o hidasp.o util.o picpgm.o
PROGRAM = $(TARGET).dll $(TARGET).lib

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(OBJS)
	dllwrap -mwindows -mno-cygwin --enable-stdcall-fixup --add-stdcall-alias -o hidblast.dll $(OBJS) -lsetupapi -Wl,--out-implib,hidblast.lib 

#	cp hidblast.dll ../


inst:
	cp hidblast.dll ../

clean:
	rm -f *.o $(PROGRAM)

#
