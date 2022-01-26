/*
 *	misclib.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#ifndef __BORLANDC__
#include <sys/time.h>
#endif
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "predefine.h"
#include "misclib.h"

#define NUMBER_OF_TIMER	10

#ifndef USE_PROCESSOR_TIME
static struct timeval	t_begin[NUMBER_OF_TIMER];
#else
static clock_t		t_begin[NUMBER_OF_TIMER];
#endif
static int		timer_idx = 0;
static FILE		*outstream = NULL;
static int		printlevel = PRINT_LEVEL_ALL;
static unsigned long	total_bytes_use = 0L;


#if DEBUG_LEVEL > 0
void dprint (const char *fmt, ...) {
	va_list	ap;

	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);
}

void dprintw (const char *fmt, ...) {
	va_list	ap;
	char	buffer[200];

	va_start (ap, fmt);
	vfprintf (stderr, fmt, ap);
	va_end (ap);

	fgets (buffer, 199, stdin);
}
#endif


static int print (const int level, const char *fmt, ...) {
	int	len = 0;

	if ((outstream != NULL) && (level <= printlevel)) {
		va_list	ap;

		va_start (ap, fmt);
		len = vfprintf (outstream, fmt, ap);
		va_end (ap);
	}

	return len;
}

static int set_print_level (const int level) {
	int	lev;

	lev = printlevel;
	printlevel = level;
	return lev;
}

static void print_error (const int level) {
	if ((outstream != NULL) && (level <= printlevel)) {
		fprintf (outstream, "%s\n", strerror (errno));
	}
}

static void* misc_malloc (size_t number_of_bytes) {
	total_bytes_use += number_of_bytes;

	return malloc (number_of_bytes);
}

static void* misc_calloc (int number, size_t number_of_bytes) {
	total_bytes_use += (number * number_of_bytes);

	return calloc (number, number_of_bytes);
}

static unsigned long bytes_use (void) {
	return total_bytes_use;
}

#ifndef USE_PROCESSOR_TIME
static void tvsub (struct timeval *tdiff,
		const struct timeval *t1, const struct timeval *t0) {
	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;

	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}
#endif

static int timer_started (void) {
	if (timer_idx < NUMBER_OF_TIMER) {
#ifndef USE_PROCESSOR_TIME
		gettimeofday (&t_begin[timer_idx++], (struct timezone *) 0);
#else
		t_begin[timer_idx++] = clock ();
#endif

		return 1;
	}

	return 0;
}

static double timer_ended (void) {
	// elapsed time

	if (timer_idx > 0) {
#ifndef USE_PROCESSOR_TIME
		struct timeval	td;
		struct timeval	t_end;
		gettimeofday (&t_end, (struct timezone *) 0);
		tvsub (&td, &t_end, &t_begin[--timer_idx]);

	        return td.tv_sec + (td.tv_usec / 1000000.);
#else
		clock_t		t_end;

		t_end = clock ();

		return (double) (t_end - t_begin[--timer_idx]) /
			CLOCKS_PER_SEC;
#endif
	}

	return 0.0;
}

static int decstring_to_integer (const char *str, const int len) {
	int	i, retval;

	for (i = retval = 0; i < len; i++) {
		if ((str[i] >= '0') && (str[i] <= '9')) {
			retval *= 10;
			retval += ((int) (str[i] - '0'));
		}
	}

	return retval;
}

static int octstring_to_integer (const char *str, const int len) {
	int	i, retval;

	for (i = retval = 0; i < len; i++) {
		if ((str[i] >= '0') && (str[i] <= '7')) {
			retval *= 8;
			retval += ((int) (str[i] - '0'));
		}
	}

	return retval;
}

static int score_conv (const char *str) {
	int	i, retval = 0;

	for (i = 0; i < 5; i++) {
		if (str[i] == '*') {		// 缺考
			return 0;
		} else if (str[i] == '-') {	// 不用考
			return -1;
		} else {
			if ((str[i] >= '0') && (str[i] <= '9')) {
				retval *= 10;
				retval += (int)(str[i] - '0');
			}
		}
	}
	/*
	int	i, j, have_dot, retval;

	for (i = j = have_dot = retval = 0; i < 5; i++) {
		if (str[i] == '*') {		// 缺考
			return 0;
		} else if (str[i] == '-') {	// 不用考
			return -1;
		} else if (str[i] == '.') {
			have_dot++;
		} else {
			if ((str[i] >= '0') && (str[i] <= '9')) {
				retval *= 10;
				retval += (int)(str[i] - '0');
				if (have_dot) j++;
			}
		}
	}

	for (i = 2; i > j; i--) retval *= 10;
	*/

	return retval;
}

static int32_t toInt (double val) {
	int32_t		retval;
	int		x;

	retval = (int32_t) val;

	x = ((long) (val * 10)) % 10;
	if (x >= 5) retval++;
	// return (int32_t) rint (val);

	return retval;
}

#define NUMBER_OF_COLOR_BUFFER	8

static int	use_ansi = 0;

static char *colorstr (const int fgcolor, const int bgcolor) {
	static char	buffer[NUMBER_OF_COLOR_BUFFER][11];
	static int	idx = 0;
	int		fg, bg;

	idx = (idx + 1) % NUMBER_OF_COLOR_BUFFER;

	if (! use_ansi) {
		buffer[idx][0] = '\0';
		return buffer[idx];
	}

	fg = fgcolor;
	bg = bgcolor;

	if ((fg < 0) || (fg > 7)) fg = -1;
	if ((bg < 0) || (bg > 7)) bg = -1;

	if ((fg == -1) && (bg == -1)) {	// back to normal
		sprintf (buffer[idx], "%c[m", 27);
	} else if (bg == -1) {		// forground only
		sprintf (buffer[idx], "%c[3%dm", 27, fg);
	} else if (bg == -1) {		// background only
		sprintf (buffer[idx], "%c[4%dm", 27, bg);
	} else {
		sprintf (buffer[idx], "%c[3%dm%c[4%dm", 27, fg, 27, bg);
	}

	return buffer[idx];
}

static char *reset_color (void) { return colorstr (-1, -1); }

#if defined(__BORLANDC__)
extern int print_on_memo (const int level, const char *fmt, ...);
extern void perror_on_memo (const int level);
extern int set_print_level_on_memo (const int level);
#endif

static int compare_list (const int32_t *list1,
				const int32_t *list2, const int num) {
	int	i;

	for (i = 0; i < num; i++) {
		if (list1[i] > list2[i]) {
			return i + 1;
		} else if (list1[i] < list2[i]) {
			return -(i + 1);
		}
	}

	return 0;
}

static void copy_list (int32_t *list1, const int32_t *list2, const int num) {
	int	i;

	for (i = 0; i < num; i++) list1[i] = list2[i];
}

static int misc_chomp (char *buffer) {
	int	i, len;

	len = strlen (buffer);

	for (i = len - 1; i >= 0; i--) {
		if (buffer[i] == '\r' || buffer[i] == '\n') {
			buffer[len = i] = '\0';
		} else {
			break;
		}
	}
	return len;
}

static int misc_rtrim (char *buffer) {
	int	i, len;

	len = strlen (buffer);

	for (i = len - 1; i >= 0; i--) {
		if (buffer[i] == ' ' || buffer[i] == '\t') {
			buffer[len = i] = '\0';
		} else {
			break;
		}
	}

	return len;
}

static int misc_ltrim (char *buffer) {
	int	i, j, len;

	len = strlen (buffer);

	for (i = 0; i < len; i++) {
		if ((buffer[i] != ' ') && (buffer[i] != '\t')) {
			break;
		}
	}

	if (i > 0) {
		for (j = 0, len -= i; j <= len; i++, j++) {
			buffer[j] = buffer[i];
		}
	}

	return len;
}

static const char* number_order_postfix (const int n) {
	static const char	*postfix[] = { "st", "nd", "rd", "th" };
	int			i;

	if (n < 0) {
		i = 3;
	} else if ((n == 11) || (n == 12)) {
		i = 3;
	} else if ((i = (n - 1) % 10) > 3) {
		i = 3;
	}

	return postfix[i];
}

static int str_to_birthday (const char *ptr) {
	int	rc;

	rc = decstring_to_integer (ptr, 2) * 10000 +
		decstring_to_integer (&ptr[2], 2) * 100 +
		decstring_to_integer (&ptr[4], 2);

	return rc;
}

struct misc_libraries_t * initial_misc_libraries_modules (const int ansicolor) {
	static struct misc_libraries_t misclib;

	use_ansi = ansicolor;

	misclib.oct             = octstring_to_integer;
	misclib.dec             = decstring_to_integer;
	misclib.timer_started   = timer_started;
	misclib.timer_ended     = timer_ended;
	misclib.birthday	= str_to_birthday;

	misclib.malloc		= misc_malloc;
	misclib.calloc		= misc_calloc;
	misclib.bytes_use	= bytes_use;
#if ! defined(__BORLANDC__)
	misclib.print           = print;
        misclib.perror          = print_error;
	misclib.set_print_level = set_print_level;
#else
        misclib.print           = print_on_memo;
        misclib.perror          = perror_on_memo;
	misclib.set_print_level = set_print_level_on_memo;
#endif

	misclib.compare_list	= compare_list;
	misclib.copy_list	= copy_list;
	misclib.score_conv      = score_conv;
	misclib.toInt           = toInt;
	misclib.color           = colorstr;
	misclib.reset_color     = reset_color;

	misclib.ltrim		= misc_ltrim;
	misclib.rtrim		= misc_rtrim;
	misclib.chomp		= misc_chomp;

	misclib.number_order_postfix	= number_order_postfix;

	outstream = stderr;

	misclib.print (PRINT_LEVEL_SYSTEM,
			"Miscellaneous libraries initialized, timer based on: "
#ifdef USE_PROCESSOR_TIME
			"processor"
#else
			"timeofday"
#endif
			" clock\r\n"
	);

	return &misclib;
}

// ---------------------------------------------------------

int fdprintf (const int fd, const char *fmt, ...) {
	va_list	ap;
	char	buffer[8192];
	int	rc;

	va_start (ap, fmt);
	rc = vsprintf (buffer, fmt, ap);
	va_end (ap);

	write (fd, buffer, rc);

	return rc;
}
