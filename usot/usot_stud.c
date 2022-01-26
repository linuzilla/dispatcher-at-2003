/*
 *	usot/usot_stud.c
 *
 *	Copyright (c) 2003, Jainn-Ching Liu
 */

#include <stdlib.h>

#include "predefine.h"
#include "global_var.h"
#include "sys_conf.h"
#include "misclib.h"
#include "textio.h"

#include "usot_dtst.h"
#include "usot_stud.h"

struct student_indexes_t {
	int32_t	sid;
	int	idx;
};

typedef struct usot_student_t *usot_student_ptr_t;

static struct usot_student_t		*stdbuf = NULL;
static int				stdnum = 0;
static struct textfile_io_t		*tio = NULL;
static struct student_indexes_t		*stdsortedindex = NULL;
static usot_student_ptr_t		*stdstack = NULL;
static int				stkcount = 0;
static int				stktop = 0;

static void reset_wish (struct usot_student_t *st) {
	st->wish_index = 0;
}

static int16_t nextwish (struct usot_student_t *st) {
	if (st == NULL) return -1;

	if (st->wish_index >= MAX_USOT_WISHES) {
		// make wish_index larger than MAX_USOT_WISHES 
		// so that it can be checked for student dosen't accept
		// by any school
		st->wish_index = MAX_USOT_WISHES + 1;
		return -1;
	}

	return st->wish[(int) st->wish_index++];
}

static void free_stack (void) {
	if (stdstack != NULL) {
		// 清掉舊的, 讓 stack 有機會重新 construct
		stkcount = stktop = 0;
		free (stdstack);
		stdstack = NULL;
	}
}

static int st_push (struct usot_student_t *st) {
	if (stktop > 0) {
		stdstack[--stktop] = st;
		return 1;
	}
	return 0;
}

static struct usot_student_t * st_pop (void) {
	if (stktop < stkcount) return stdstack[stktop++];
	return NULL;
}

static int prepare_sorted_stack (void) {
	int	i;

	if (stdstack == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Allocate space for student stack ... ");

		if ((stdstack = misc->malloc (stdnum *
					sizeof (usot_student_ptr_t))) == NULL) {
			misc->perror (PRINT_LEVEL_SYSTEM);
			return 0;
		}
	} else {
		misc->print (PRINT_LEVEL_SYSTEM,
			"Re-allocate space for student stack ... ");
	}

	stkcount = stktop = 0;
	misc->timer_started ();

	stkcount = stdnum;
	stktop = 0;

	for (i = 0; i < stdnum; i++) stdstack[i] = &stdbuf[i];

	misc->print (PRINT_LEVEL_SYSTEM, "ok. [ %.4f second(s) ]\r\n",
			misc->timer_ended ());

	return 1;
}

/////////////////////////////////////////////////////////////////////////

static int order_by_sid (const struct student_indexes_t *i,
				const struct student_indexes_t *j) {
	if (i->sid < j->sid) {
		return -1;
	} else if (i->sid > j->sid) {
		return 1;
	} else {
		return 0;
	}
}

static int generate_sorted_student_index (void) {
	int		i;

	if ((stdsortedindex != NULL) || (stdbuf == NULL)) return 0;

	if ((stdsortedindex = misc->malloc (stdnum *
			sizeof (struct student_indexes_t))) == NULL) return 0;

	for (i = 0; i < stdnum; i++) {
		stdsortedindex[i].idx = i;
		stdsortedindex[i].sid = stdbuf[i].sid;
	}

	qsort (stdsortedindex, stdnum,
			sizeof (struct student_indexes_t),
			(int (*)(const void *, const void *)) order_by_sid);

	return 1;
}

static int search_sid_index (const int32_t sid) {
	struct student_indexes_t	*ptr;
	struct student_indexes_t	key;

	key.sid = sid;

	ptr = bsearch (&key, stdsortedindex, stdnum,
			sizeof (struct student_indexes_t),
			(int (*)(const void *, const void *)) order_by_sid);

	if (ptr == NULL) return -1;

	return ptr->idx;
}

static void read_usot_student_wish (const int idx, void *buffer) {
	struct usot_wish_t	*ptr = buffer;
	struct usot_student_t	*std = &stdbuf[idx];
	int			i;

	std->sid = misc->dec (ptr->sid, sizeof (ptr->sid));
	// std->number_of_wishes = 0;
	
	for (i = 0; i < MAX_USOT_WISHES; i++) {
		std->have_wish[i] = (ptr->wishes[i][0] == '-') ? 0 : 1;
		std->wish[i] = misc->dec (ptr->wishes[i], 4);
		// if (std->wish[i] != 0) std->number_of_wishes ++;
	}

	std->sflag = 0;
	std->wish_index = 0;
}

static int read_usot_student_info (void *buffer) {
	struct usot_basic_t	*ptr = buffer;
	int32_t			sid;
	int			idx;
	struct usot_student_t	*std;

	sid = misc->dec (ptr->sid, sizeof ptr->sid);

	if ((idx = search_sid_index (sid)) == -1) return -1;

	std = &stdbuf[idx];

	std->sflag |= USOT_STUDENT_FLAG_HAVE_INFO;
	std->gender = (ptr->gender[0] == 'F') ? 2 : 1;	// 非女即男
	std->ptype  = ptr->ptype[0];
	std->check  = misc->dec (ptr->result, sizeof ptr->result);

	if (std->check == 2) std->check = 3;

	std->uclass = misc->dec (ptr->uclass, sizeof ptr->uclass);

	return 1;
}

static int read_usot_student_score (void *buffer, const int len) {
	struct usot_score_t	*ptr = buffer;
	int32_t			sid;
	struct usot_student_t	*std;
	int			i, noc;

	sid = misc->dec (ptr->sid, sizeof ptr->sid);

	if ((i = search_sid_index (sid)) == -1) return -1;

	std = &stdbuf[i];

	std->sflag |= USOT_STUDENT_FLAG_HAVE_SCORE;

	if (len >= sizeof (struct usot_score_t)) {
		noc = MAX_USOT_EXAM_COURSE;
	} else {
		noc = MAX_USOT_EXAM_COURSE -
			(sizeof (struct usot_score_t) - len) / 
			(sizeof (struct usot_score_sub_t));
	}

	for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) {
		if (i >= noc) {
			std->crs[i]  = 0;
			std->sco[i]  = -1;
			std->miss[i] = 1;
		} else {
			std->crs[i]  = misc->dec (ptr->sc[i].crs,
							sizeof ptr->sc[i].crs);
			std->sco[i]  = misc->dec (ptr->sc[i].src,
							sizeof ptr->sc[i].src);

			if (ptr->sc[i].src[0] == '*') {
				std->miss[i] = 1;
			} else {
				std->miss[i] = 0;
			}
		}
	}

	return 1;
}

static int load_wishes (void) {
	char		*filename;
	int		count, shortline, len, idx;
	char		*buffer;

	if ((filename = sysconfig->getstr ("student-wishes-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable (student-wishes-file) "
				"not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
		"Load student wishes file [ %s ] ... ",
		filename);

	if (tio->open (filename)) {
		count = shortline = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len >= sizeof (struct usot_wish_t)) {
				count++;
			} else {
				shortline++;
			}
		}

		misc->print (PRINT_LEVEL_INFO,
				"%lu   [ pass one ]\r\n", count);
		tio->close ();
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_INFO, "Allocate %lu memory buffers ... ",
				count * sizeof (struct usot_student_t));

	if ((stdbuf = misc->calloc (count,
				sizeof (struct usot_student_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	} else {
		stdnum = count;
		misc->print (PRINT_LEVEL_INFO, "ok\r\n");
	}

	misc->print (PRINT_LEVEL_INFO,
			"Load student wishes into memory  ... ");

	if (tio->open (filename)) {
		for (idx = 0; (len = tio->read (&buffer)) >= 0; ){
			if (len < sizeof (struct usot_wish_t)) continue;
			read_usot_student_wish (idx, buffer);
			++idx;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_SYSTEM, "ok   [ pass two ]\r\n");
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->timer_started ();

	misc->print (PRINT_LEVEL_SYSTEM,
			"Generating sorted index for students ... ");

	if (generate_sorted_student_index ()) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"ok.  (elapsed time: %.4f seconds)\r\n",
			misc->timer_ended());
	} else {
		misc->timer_ended ();
		misc->print (PRINT_LEVEL_SYSTEM, "failed\r\n");
		return 0;
	}

	return 1;
}

static int load_info (void) {
	char		*filename;
	char		*buffer;
	int		i, len, nof;

	if (stdbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Please load wishes first\r\n");
		return 0;
	}

	if ((filename = sysconfig->getstr ("student-info-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (student-info-file) "
			"not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load USOT format info file [ %s ] ... ", filename); 

	i = nof = 0;

	if (tio->open (filename)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct usot_basic_t)) continue;

			if (read_usot_student_info (buffer) != -1) nof++;
			i++;
		}

		misc->print (PRINT_LEVEL_INFO, "%d read, %d use\r\n", i, nof);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	for (i = nof = 0; i < stdnum; i++) {
		if ((stdbuf[i].sflag & USOT_STUDENT_FLAG_HAVE_INFO) == 0) {
			misc->print (PRINT_LEVEL_SYSTEM,
					"[%d]", stdbuf[i].sid);
			nof++;
		}

		if (nof > 0) {
			misc->print (PRINT_LEVEL_SYSTEM,
					"[Oops!] %d student missing info\n",
					nof);
			return 1;
		}
	}

	return 1;
}

static int load_score (void) {
	char	*filename;
	char	*buffer;
	int	i, len, nof;

	if (stdbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Please load wishes first\r\n");
		return 0;
	}

	if ((filename = sysconfig->getstr ("student-score-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (student-score-file) "
			"not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load USOT format score file [ %s ] ... ", filename);

	i = nof = 0;

	if (tio->open (filename)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < USOT_SID_LENGTH) continue;

			if (read_usot_student_score (buffer, len) != -1) nof++;
			i++;
		}

		misc->print (PRINT_LEVEL_SYSTEM,
				"%d read, %d use\r\n", i, nof);
		tio->close ();
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	for (i = nof = 0; i < stdnum; i++) {
		if ((stdbuf[i].sflag & USOT_STUDENT_FLAG_HAVE_SCORE) == 0) {
			misc->print (PRINT_LEVEL_SYSTEM,
					"[%d]", stdbuf[i].sid);
			nof++;
		}

		if (nof > 0) {
			misc->print (PRINT_LEVEL_SYSTEM,
					"[Oops!] %d student missing score\n",
					nof);
			return 1;
		}
	}

	return 1;
}

struct usot_student_module_t  *initial_usot_student_module (void) {
	struct usot_student_module_t	*self;

	tio = initial_textfile_io_module ();

	if ((self = malloc (sizeof (struct usot_student_module_t))) != NULL) {
		self->load_wishes	= load_wishes;
		self->load_info		= load_info;
		self->load_score	= load_score;
		self->push		= st_push;
		self->pop		= st_pop;
		self->free_stack	= free_stack;
		self->nextwish		= nextwish;
		self->reset_wish	= reset_wish;
		self->prepare_sorted_stack = prepare_sorted_stack;
	}

	return self;
}
