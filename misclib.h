/*
 *	misclib.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef _MISC_LIB_H_
#define _MISC_LIB_H_

#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define PRINT_LEVEL_SYSTEM	1
#define PRINT_LEVEL_INFO	4
#define PRINT_LEVEL_DEBUG	8
#define PRINT_LEVEL_ALL		9

#ifndef COLOR_BLACK
#define COLOR_BLACK     0
#define COLOR_RED       1
#define COLOR_GREEN     2
#define COLOR_YELLOW    3
#define COLOR_BLUE      4
#define COLOR_MAGENTA   5
#define COLOR_CYAN      6
#define COLOR_WHITE     7
#endif

struct misc_libraries_t {
	int	(*oct)(const char *, const int);
	int	(*dec)(const char *, const int);
	int	(*score_conv)(const char *);
	int	(*birthday)(const char *);
	int	(*timer_started)(void);
	double	(*timer_ended)(void);
	int	(*print)(const int, const char *, ...);
	int	(*set_print_level)(const int);
	void	(*perror)(const int);
	void*	(*malloc)(size_t);
	void*	(*calloc)(int, size_t);
	unsigned long	(*bytes_use)(void);
	int32_t	(*toInt)(double);
	char*	(*color)(const int fg, const int bg);
	char*	(*reset_color)(void);
	int 	(*compare_list)(const int32_t *, const int32_t *, const int);
	void	(*copy_list)(int32_t *, const int32_t *, const int);
	int	(*ltrim)(char *);
	int	(*rtrim)(char *);
	int	(*chomp)(char *);
	const char*	(*number_order_postfix)(const int);
};

struct misc_libraries_t * initial_misc_libraries_modules (const int use_ansi);

#if DEBUG_LEVEL > 0
void dprint  (const char *fmt, ...);
void dprintw (const char *fmt, ...);
#endif

int fdprintf (const int fd, const char *fmt, ...);

#if defined(__cplusplus)
}
#endif

#endif
