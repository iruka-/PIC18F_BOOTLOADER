# for hidspx

CC = gcc
INSTALL_DIR=/usr/local/bin
HIDSPX=hidspx
LIBUSB=-lusb

#for MacOS X
#CFLAGS=-O2 -DUSER_BOOKMARKS -DHIDASP_ONLY -DAVRSPX=1 -DMACOS -DTIME_DISPLAY -I/opt/local/include -L/opt/local/lib

#for BSD
#CFLAGS=-O2 -DHIDASP_ONLY -DAVRSPX=1 -I/usr/local/include -L/usr/local/lib

#for Linux
CFLAGS=-O2 -DUSER_BOOKMARKS -DHIDASP_ONLY -DAVRSPX=1 -DLINUX -DTIME_DISPLAY
#CFLAGS=-g -DHIDASP_ONLY -DAVRSPX=1 -DLINUX

hidspx: avrspx.c hwctrl.c utils.c hidasp.c Makefile
	$(CC) $(CFLAGS) -o $(HIDSPX) avrspx.c hwctrl.c utils.c hidasp.c info.c $(LIBUSB) 

clean:
	-rm -fv $(HIDSPX) *.o *~

install:
	-rm -rf $(INSTALL_DIR)/hidspx
	chmod 644 *.c *.h *.ini *.txt
	install -m 4755 $(HIDSPX) $(INSTALL_DIR)
	install -m 644 hidspx.ini $(INSTALL_DIR)/$(HIDSPX).ini
	install -m 644 fuse.txt $(INSTALL_DIR)
	-rm -fv $(HIDSPX) *.o *~

