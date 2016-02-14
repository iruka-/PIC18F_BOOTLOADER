#
#	Windows用です.
#
#	コンパイラは minGw gccを使用します。
#	試してはいませんがcygwinでも大丈夫なはずです。
#
EXE_SUFFIX = .exe

O = .o
CC		= gcc
CFLAGS	= $(USBFLAGS) -O2 -Wall

TARG = dlltest
OBJS = dlltest$(O)

PROGRAM = $(TARG)$(EXE_SUFFIX)

all: $(PROGRAM)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(PROGRAM): $(OBJS)
	$(CC) -s -o $(PROGRAM) $(OBJS) 

#hidmon.lib

strip: $(PROGRAM)
	strip $(PROGRAM)

clean:
#	rm -f *.o $(PROGRAM)
	rm -f *$(O)

install:
	install -c $(PROGRAM) ../bin
#
###
dlltest$(O): hidmon.h
