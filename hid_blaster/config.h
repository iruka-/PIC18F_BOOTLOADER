/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* 0 if you don't want the Amontec JTAG-Accelerator driver. */
#define BUILD_AMTJTAGACCEL 0

/* 0 if you don't want the ARM-JTAG-EW JTAG driver. */
#define BUILD_ARMJTAGEW 0

/* 0 if you don't want at91rm9200. */
#define BUILD_AT91RM9200 0

/* 0 if you don't want a bitbang interface. */
#define BUILD_BITBANG 1

/* 0 if you don't want a bitq interface. */
#define BUILD_BITQ 0

/* 0 if you don't want the Buspirate JTAG driver. */
#define BUILD_BUSPIRATE 0

/* 0 if you don't want dummy driver. */
#define BUILD_DUMMY 1

/* 0 if you don't want eCosBoard. */
#define BUILD_ECOSBOARD 0

/* 0 if you don't want ep93xx. */
#define BUILD_EP93XX 0

/* 0 if you don't want ftd2xx ft2232. */
#define BUILD_FT2232_FTD2XX 0

/* Support FT2232H/FT4232HS with FTD2XX or libftdi. */
/* #undef BUILD_FT2232_HIGHSPEED */

/* 0 if you don't want libftdi ft2232. */
#define BUILD_FT2232_LIBFTDI 0

/* 0 if you don't want the Gateworks GW16012 driver. */
#define BUILD_GW16012 0

/* 0 if you don't want the J-Link JTAG driver. */
#define BUILD_JLINK 0

/* Use the dummy minidriver. */
/* #undef BUILD_MINIDRIVER_DUMMY */

/* 0 if you don't want the OpenOCD+trace ETM capture driver. */
#define BUILD_OOCD_TRACE 0

/* 0 if you don't want parport. */
#define BUILD_PARPORT 0

/* 0 if you don't want the ASIX PRESTO driver using FTD2XX. */
#define BUILD_PRESTO_FTD2XX 0

/* 0 if you don't want the ASIX PRESTO driver using libftdi. */
#define BUILD_PRESTO_LIBFTDI 0

/* 0 if you don't want the RLink JTAG driver. */
#define BUILD_RLINK 0

/* 0 if you don't want the usbprog JTAG driver. */
#define BUILD_USBPROG 0

/* 0 if you don't want ftd2xx usb_blaster. */
#define BUILD_USB_BLASTER_FTD2XX 0

/* 0 if you don't want libftdi usb_blaster. */
#define BUILD_USB_BLASTER_LIBFTDI 0

/* 0 if you don't want the Versaloon-Link JTAG driver. */
#define BUILD_VSLLINK 0

/* 0 if you don't want ZY1000. */
#define BUILD_ZY1000 0

/* 0 if you don't want ZY1000 JTAG master registers. */
#define BUILD_ZY1000_MASTER 0

/* Define to 1 if you have the <arpa/inet.h> header file. */
/* #undef HAVE_ARPA_INET_H */

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <elf.h> header file. */
/* #undef HAVE_ELF_H */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <ifaddrs.h> header file. */
/* #undef HAVE_IFADDRS_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <jtag_minidriver.h> header file. */
/* #undef HAVE_JTAG_MINIDRIVER_H */

/* Define to 1 if the system has the type `long long int'. */
#define HAVE_LONG_LONG_INT 1

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <netdb.h> header file. */
/* #undef HAVE_NETDB_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
/* #undef HAVE_NETINET_IN_H */

/* Define to 1 if you have the <netinet/tcp.h> header file. */
/* #undef HAVE_NETINET_TCP_H */

/* Define to 1 if you have the <net/if.h> header file. */
/* #undef HAVE_NET_IF_H */

/* Define to 1 if you have the <pthread.h> header file. */
/* #undef HAVE_PTHREAD_H */

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strndup' function. */
/* #undef HAVE_STRNDUP */

/* Define to 1 if you have the `strnlen' function. */
/* #undef HAVE_STRNLEN */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
/* #undef HAVE_SYS_IOCTL_H */

/* Define to 1 if you have the <sys/io.h> header file. */
/* #undef HAVE_SYS_IO_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/poll.h> header file. */
/* #undef HAVE_SYS_POLL_H */

/* Define to 1 if you have the <sys/select.h> header file. */
/* #undef HAVE_SYS_SELECT_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
/* #undef HAVE_SYS_SOCKET_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <usb.h> header file. */
/* #undef HAVE_USB_H */

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to 1 if you have the `vasprintf' function. */
/* #undef HAVE_VASPRINTF */

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* 0 if not building for Cygwin. */
/* #undef IS_CYGWIN */

/* 0 if not building for Darwin. */
#define IS_DARWIN 0

/* 1 if building for MinGW. */
#define IS_MINGW 1

/* 0 if not building for Win32. */
#define IS_WIN32 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if assertions should be disabled. */
/* #undef NDEBUG */

/* Must declare 'environ' to use it. */
/* #undef NEED_ENVIRON_EXTERN */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "openocd"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "OpenOCD Mailing List <openocd-development@lists.berlios.de>"

/* Define to the full name of this package. */
#define PACKAGE_NAME "openocd"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "openocd 0.5.0-dev"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "openocd"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.5.0-dev"

/* 0 if you don't want parport to use giveio. */
#define PARPORT_USE_GIVEIO 1

/* 0 if you don't want parport to use ppdev. */
#define PARPORT_USE_PPDEV 0

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Version number of package */
#define VERSION "0.5.0-dev"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Include malloc free space in logging */
/* #undef _DEBUG_FREE_SPACE_ */

/* Print verbose JTAG I/O messages */
/* #undef _DEBUG_JTAG_IO_ */

/* Print verbose USB communication messages */
/* #undef _DEBUG_USB_COMMS_ */

/* Print verbose USB I/O messages */
/* #undef _DEBUG_USB_IO_ */

/* Use GNU C library extensions (e.g. stdndup). */
#define _GNU_SOURCE 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */


#include <helper/system.h>
//#include <helper/replacements.h>

