/*
 *	textio.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#ifndef __BORLANDC__
#include <unistd.h>
#else
#include <io.h>
#endif
#include "predefine.h"
#include "textio.h"

#define TIO_BUFFER_SIZE	4096

static char	buffer[2 * TIO_BUFFER_SIZE];
static int	bp  = 0;
static int	lp  = 0;
static int	fd  = -1;

static struct textfile_io_t	tio;

static int tio_open  (const char *file) {
	if (fd >= 0) close (fd);
	bp = lp = 0;
	fd = open (file, O_RDONLY);

	return fd >= 0 ? 1 : 0;
}

static int tio_close (void) {
	int	retval;

	retval = close (fd);
	fd = -1;

	return retval;
}
static int tio_read  (char **linebuffer) {
	int	i, j, dp, found, retval = 0;
	int	len, try = 0;

	if (fd < 0) return -1;

	do {
		for (dp = lp, found = retval = 0; ! found && (dp < bp); dp++) {
			switch (buffer[dp]) {
			case '\0':
				// fprintf (stderr, "Oops!!\n");
				for (i = dp; i < bp - 1; i++) {
					buffer[i] = buffer[i+1];
				}
				bp--;
				dp--;
				// retval++;
				break;
			case '\r':
				buffer[dp] = '\0';
				break;
			case '\n':
				found = 1;
				buffer[dp] = '\0';
				break;
			case '\x1a':
				found = 1;
				buffer[dp] = '\0';
				return -1;
			default:
				retval++;
				break;
			}
		}

		if (! found) {
			if (bp > TIO_BUFFER_SIZE) {
				for (i = 0, j = lp; j < bp; j++, i++)
					buffer[i] = buffer[j];
				bp -= lp;
				lp = 0;
			}

			if (bp > TIO_BUFFER_SIZE) {
				fprintf (stderr, "line too long\n");
				return -1;
			}

			len = read (fd, &buffer[bp], TIO_BUFFER_SIZE);

			if (len > 0) {
				bp += len;
				try = 0;
			} else { // if (len <= 0) 
				if (retval == 0) return -1;

				switch (++try) {
				case 1: // give a chance
					break;
				case 2: // flush
					buffer[lp + retval] = '\0';
					*linebuffer = &buffer[lp];
					return retval;
				case 3: // quit
				default:
					return -1;
				}
			}
		}
	} while (! found);

	*linebuffer = &buffer[lp];
	lp = dp;

	return retval;
}

struct textfile_io_t * initial_textfile_io_module (void) {
	tio.open  = tio_open;
	tio.read  = tio_read;
	tio.close = tio_close;

	return &tio;
}
