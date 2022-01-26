/*
 *	textio.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __TEXTFILE_IO_H_
#define __TEXTFILE_IO_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct textfile_io_t {
	int	(*open)(const char *filename);
	int	(*close)(void);
	int	(*read)(char **linebuffer);
};

struct textfile_io_t * initial_textfile_io_module (void);

#if defined(__cplusplus)
}
#endif

#endif
