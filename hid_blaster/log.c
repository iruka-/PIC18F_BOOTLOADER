#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define	HAVE_STDINT_H

#include "helper/command.h"
#include "helper/log.h"

static	int count=0;
static	int server_use_pipes=0;
//static  
int debug_level=9;
static	int	jtag_reset_config=0;
static char *log_strings[5] =
{
	"User : ",
	"Error: ",
	"Warn : ",  /* want a space after each colon, all same width, colons aligned */
	"Info : ",
	"Debug: "
};

#define log_output	stderr

void jtag_sleep(int us)					{}

//enum reset_types 
int	jtag_get_reset_config(void)
{
	return jtag_reset_config;
}

void* buf_set_buf(const void *_src, unsigned src_start,
		void *_dst, unsigned dst_start, unsigned len)
{
	const uint8_t *src = _src;
	uint8_t *dst = _dst;

	unsigned src_idx = src_start, dst_idx = dst_start;
	for (unsigned i = 0; i < len; i++)
	{
		if (((src[src_idx / 8] >> (src_idx % 8)) & 1) == 1)
			dst[dst_idx / 8] |= 1 << (dst_idx % 8);
		else
			dst[dst_idx / 8] &= ~(1 << (dst_idx % 8));
		dst_idx++;
		src_idx++;
	}

	return dst;
}

void* buf_cpy(const void *from, void *_to, unsigned size)
{
	if (NULL == from || NULL == _to)
		return NULL;

	// copy entire buffer
	memcpy(_to, from, DIV_ROUND_UP(size, 8));

	/* mask out bits that don't belong to the buffer */
	unsigned trailing_bits = size % 8;
	if (trailing_bits)
	{
		uint8_t *to = _to;
		to[size / 8] &= (1 << trailing_bits) - 1;
	}
	return _to;
}


/* return allocated string w/printf() result */
char *alloc_vprintf(const char *fmt, va_list ap)
{
	va_list ap_copy;
	int len;
	char *string;

	/* determine the length of the buffer needed */
	va_copy(ap_copy, ap);
	len = vsnprintf(NULL, 0, fmt, ap_copy);
	va_end(ap_copy);

	/* allocate and make room for terminating zero. */
	/* FIXME: The old version always allocated at least one byte extra and
	 * other code depend on that. They should be probably be fixed, but for
	 * now reserve the extra byte. */
	string = malloc(len + 2);
	if (string == NULL)
		return NULL;

	/* do the real work */
	vsnprintf(string, len + 1, fmt, ap);

	return string;
}

/* The log_puts() serves to somewhat different goals:
 *
 * - logging
 * - feeding low-level info to the user in GDB or Telnet
 *
 * The latter dictates that strings without newline are not logged, lest there
 * will be *MANY log lines when sending one char at the time(e.g.
 * target_request.c).
 *
 */
static void log_puts(enum log_levels level, const char *file, int line, const char *function, const char *string)
{
#if	1
	char *f;
	if (level == LOG_LVL_OUTPUT)
	{
		/* do not prepend any headers, just print out what we were given and return */
		fputs(string, log_output);
		fflush(log_output);
		return;
	}

	f = strrchr(file, '/');
	if (f != NULL)
		file = f + 1;

	if (strlen(string) > 0)
	{
		if (debug_level >= LOG_LVL_DEBUG)
		{
			/* print with count and time information */
			int t = 0;	//(int)(timeval_ms()-start);
#ifdef _DEBUG_FREE_SPACE_
			struct mallinfo info;
			info = mallinfo();
#endif
			fprintf(log_output, "%s%d %d %s:%d %s()"
#ifdef _DEBUG_FREE_SPACE_
					" %d"
#endif
					": %s", log_strings[level + 1], count, t, file, line, function,
#ifdef _DEBUG_FREE_SPACE_
					info.fordblks,
#endif
					string);
		}
		else if (server_use_pipes == 0)
		{
			/* if we are using gdb through pipes then we do not want any output
			 * to the pipe otherwise we get repeated strings */
			fprintf(log_output, "%s%s",
					(level > LOG_LVL_USER)?log_strings[level + 1]:"", string);
		}
	} else
	{
		/* Empty strings are sent to log callbacks to keep e.g. gdbserver alive, here we do nothing. */
	}

	fflush(log_output);

	/* Never forward LOG_LVL_DEBUG, too verbose and they can be found in the log if need be */
	if (level <= LOG_LVL_INFO)
	{
//		log_forward(file, line, function, string);
	}
#endif
}


void log_printf(enum log_levels level, const char *file, unsigned line, const char *function, const char *format, ...)
{
	char *string;
	va_list ap;

	count++;
	if (level > debug_level)
		return;

	va_start(ap, format);

	string = alloc_vprintf(format, ap);
	if (string != NULL)
	{
		log_puts(level, file, line, function, string);
		free(string);
	}

	va_end(ap);
}

void log_printf_lf(enum log_levels level, const char *file, unsigned line, const char *function, const char *format, ...)
{
	char *string;
	va_list ap;

	count++;
	if (level > debug_level)
		return;

	va_start(ap, format);

	string = alloc_vprintf(format, ap);
	if (string != NULL)
	{
		strcat(string, "\n"); /* alloc_vprintf guaranteed the buffer to be at least one char longer */
		log_puts(level, file, line, function, string);
		free(string);
	}

	va_end(ap);
}

