/*
 *	usot/usot_dept.c
 *
 *	Copyright (c) 2003, Jainn-Ching Liu
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "predefine.h"
#include "global_var.h"
#include "sys_conf.h"
#include "misclib.h"
#include "textio.h"
#include "studlist.h"

#include "usot_dtst.h"
#include "usot_dept.h"
#include "usot_stud.h"

#define ENABLE_RESERVED_STUDENT		1

struct department_indexes_t {
	int32_t	depid;
	int	idx;
};

struct course_indexes_t {
	int16_t	crsid;
	int	idx;
};

static struct usot_department_t		*depbuf = NULL;
static int				depnum = 0;
static struct textfile_io_t		*tio = NULL;
static struct department_indexes_t	*depsortedindex = NULL;
////////////////////////////////////////////////////////////////
static struct usot_course_t		*crsbuf = NULL;
static int				crsnum = 0;
static struct course_indexes_t		*crssortedindex = NULL;
static int				use_onsame = 0;

static struct lost_student_list_t	*lslptr = NULL;


static int sort_students (struct usot_department_t *dp);


static int allocate_space (void) {
	int		i;
	int		freshman = 0;
	int		allocate = 0;
	unsigned long	total = 0L;

	misc->print (PRINT_LEVEL_SYSTEM,
			"Allocate space for each departments ... ");

	for (i = 0; i < depnum; i++) {
		// 請注意, 要加 2 是和程式的 sorting 演算法有關 ....
		allocate = (depbuf[i].num + 2) *
				sizeof (struct usot_student_on_wish_t);

		freshman += depbuf[i].num;
		depbuf[i].stdlist = misc->malloc (allocate);

		total += allocate;

		// ----------------------------------------------

#if ENABLE_RESERVED_STUDENT == 1
		allocate = sizeof (struct usot_department_t);

		depbuf[i].rdep = misc->malloc (allocate);

		total += allocate;

		if (depbuf[i].rdep == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM, "failed");
			return 0;
		}

		memcpy (depbuf[i].rdep, &depbuf[i], allocate);

		depbuf[i].rdep->num = depbuf[i].res;

		// --------

		allocate = (depbuf[i].rdep->num + 2) *
				sizeof (struct usot_student_on_wish_t);

		depbuf[i].rdep->stdlist = misc->malloc (allocate);

		depbuf[i].rdep->rdep = NULL;

		total += allocate;

		if (depbuf[i].rdep->stdlist == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM, "failed");
			return 0;
		}
#endif
		if (depbuf[i].stdlist == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM, "failed");
			return 0;
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"%lu bytes allocated (%d)\r\n", total, freshman); 

	return 1;
}

static int order_by_crsid (const struct course_indexes_t *i,
				const struct course_indexes_t *j) {
	if (i->crsid < j->crsid) {
		return -1;
	} else if (i->crsid > j->crsid) {
		return 1;
	} else {
		return 0;
	}
}

static int generate_sorted_course_index (void) {
	int		i;

	if ((crssortedindex != NULL) || (crsbuf == NULL)) return 0;

	if ((crssortedindex = misc->malloc (crsnum *
		sizeof (struct course_indexes_t))) == NULL) return 0;

	for (i = 0; i < crsnum; i++) {
		crssortedindex[i].idx   = i;
		crssortedindex[i].crsid = crsbuf[i].crsid;
	}

	qsort (crssortedindex, crsnum,
			sizeof (struct course_indexes_t),
			(int (*)(const void *, const void *)) order_by_crsid);

	return 1;
}

static int search_crsid_index (const int16_t crsid) {
	struct course_indexes_t	*ptr;
	struct course_indexes_t	key;

	key.crsid = crsid;

	ptr = bsearch (&key, crssortedindex, crsnum,
			sizeof (struct course_indexes_t),
			(int (*)(const void *, const void *)) order_by_crsid);

	if (ptr == NULL) return -1;

	return ptr->idx;
}

static int find_course_group (const int16_t crsid) {
	int	i;

	if ((i = search_crsid_index (crsid)) < 0) return -1;

	return crsbuf[i].crsgrp;
}

///////////////////////////////////////////////////////////////////////

static int order_by_depid (const struct department_indexes_t *i,
				const struct department_indexes_t *j) {
	if (i->depid < j->depid) {
		return -1;
	} else if (i->depid > j->depid) {
		return 1;
	} else {
		return 0;
	}
}

static int generate_sorted_department_index (void) {
	int		i;

	if ((depsortedindex != NULL) || (depbuf == NULL)) return 0;

	if ((depsortedindex = misc->malloc (depnum *
		sizeof (struct department_indexes_t))) == NULL) return 0;

	for (i = 0; i < depnum; i++) {
		depsortedindex[i].idx   = i;
		depsortedindex[i].depid = depbuf[i].depid;
	}

	qsort (depsortedindex, depnum,
			sizeof (struct department_indexes_t),
			(int (*)(const void *, const void *)) order_by_depid);

	return 1;
}

static int search_depid_index (const int32_t depid) {
	struct department_indexes_t	*ptr;
	struct department_indexes_t	key;

	key.depid = depid;

	ptr = bsearch (&key, depsortedindex, depnum,
			sizeof (struct department_indexes_t),
			(int (*)(const void *, const void *)) order_by_depid);

	if (ptr == NULL) return -1;

	return ptr->idx;
}

//////////////////////////////////////////////////////////////////////////

static void read_usot_course (const int idx, void *buffer) {
	struct usot_course_map_t	*ptr = buffer;
	struct usot_course_t		*crs = &crsbuf[idx];

	crs->crsid  = misc->dec (ptr->crsid, sizeof ptr->crsid);
	crs->crsgrp = misc->dec (ptr->crsgrp, sizeof ptr->crsgrp);
	memcpy (crs->crsname, ptr->crsname, sizeof ptr->crsname);
	crs->crsname[sizeof ptr->crsname] = '\0';
	misc->ltrim (crs->crsname);
}

static void read_usot_deplim (const int idx, void *buffer) {
	struct usot_deplim_t		*ptr = buffer;
	struct usot_department_t	*dep = &depbuf[idx];
	int				i;

	dep->depid  = misc->dec (ptr->depid, sizeof ptr->depid);
	dep->dflag  = 0;
	dep->crset  = NULL;
	dep->limsum = 0;
	dep->limcnt = 0;

	dep->minreq	= 0;
	dep->inlistcnt	= 0;
	dep->lastidx	= 0;
	dep->llindex	= 0;

#if ENABLE_RESERVED_STUDENT == 1
	dep->rdep = NULL;
#endif

	dep->max_onsame_use = 0;
	for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) dep->min_onsame[i] = 0;

	dep->num = misc->dec (ptr->num, sizeof ptr->num);
	dep->res = misc->dec (ptr->res, sizeof ptr->res);
	dep->gender = misc->dec (ptr->gender, sizeof ptr->gender);

	dep->lim = misc->dec (ptr->lim, sizeof ptr->lim);

	for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) {
		dep->exam[i]   = misc->dec (ptr->exam[i], 
						sizeof ptr->exam[i]);
		dep->lowsco[i] = misc->dec (ptr->lowsco[i],
						sizeof ptr->lowsco[i]);
		dep->onsame[i] = ptr->onsame[i];
	}

	dep->lowsum = misc->dec (ptr->lowws, sizeof ptr->lowws);
}

static int read_usot_depna (void *buffer) {
	struct usot_depna_t		*ptr = buffer;
	struct usot_department_t	*dep;
	int				i, depid;

	depid = misc->dec (ptr->depid, sizeof ptr->depid);

	if ((i = search_depid_index (depid)) == -1) return 0;

	dep = &depbuf[i];
	dep->dflag |= USOT_DEPT_FLAG_HAVE_DEPNAME;

	memcpy (dep->school, ptr->school, sizeof ptr->school);
	dep->school[sizeof ptr->school] = '\0';
	misc->ltrim (dep->school);

	memcpy (dep->depname, ptr->depname, sizeof ptr->depname);
	dep->depname[sizeof ptr->depname] = '\0';
	misc->ltrim (dep->depname);

	dep->age = misc->dec (ptr->age, sizeof ptr->age);
	dep->uclass = misc->dec (ptr->uclass, sizeof ptr->uclass);

	return 1;
}

static int read_usot_course_set (void *buffer) {
	struct usot_depcrs_t		*ptr = buffer;
	struct usot_department_t	*dep;
	int				i, j, depid;
	char				*bptr = ptr->exid;
	struct usot_courseset_t		*xset = NULL;

	depid = misc->dec (ptr->depid, sizeof ptr->depid);

	if ((i = search_depid_index (depid)) == -1) return 0;

	dep = &depbuf[i];
	dep->dflag |= USOT_DEPT_FLAG_HAVE_CRSSET;

	i = misc->dec (ptr->limcnt, sizeof ptr->limcnt);

	if (dep->limsum == 0) {
		dep->limsum = i;
		dep->limcnt = 0;
	} else if (dep->limsum != i) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"%d crsgrp not match (%d v.s %d)\n",
				depid, dep->limsum, i);
		return 0;
	}
	dep->limcnt++;

	if ((xset = misc->malloc (sizeof (struct usot_courseset_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	xset->next = dep->crset;
	dep->crset = xset;

	for (i = j = 0; i < MAX_USOT_EXAM_COURSE; i++) {
		xset->excrs[i] = misc->dec (ptr->excrs[i],
						sizeof ptr->excrs[i]);
		j += xset->excrs[i];
	}

	xset->num = j;

	if ((xset->crslist = misc->malloc (j * sizeof (int16_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	for (i = 0; i < j; i++) {
		xset->crslist[i] = misc->dec (bptr, sizeof ptr->exid);
		bptr += sizeof ptr->exid;
	}

	return 1;
}

static int load_limits (void) {
	char			*filename;
	char			*buffer;
	int			len, count, idx;

	if ((filename = sysconfig->getstr ("department-limit-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (department-limit-file)"
			" not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
		"Load USOT deplim file [ %s ] ... ", filename);

	count = 0;

	if (tio->open (filename)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct usot_deplim_t)) continue;
			count++;
		}
		tio->close ();
		misc->print (PRINT_LEVEL_INFO, "%lu [ pass one ]\r\n", count);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_INFO, "Allocate %lu memory buffers ... ",
			count * sizeof (struct usot_department_t));

	if ((depbuf = misc->calloc (count,
				sizeof (struct usot_department_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	} else {
		depnum = count;
		misc->print (PRINT_LEVEL_INFO, "ok\r\n");
	}

	misc->print (PRINT_LEVEL_INFO, "Load USOT deplim into memory  ... ");

	if (tio->open (filename)) {
		idx = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct usot_deplim_t)) continue;
			read_usot_deplim (idx, buffer);
			idx++;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_SYSTEM, "ok   [ pass two ]\r\n");
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Generating sorted index for departments ... ");

	misc->timer_started ();

	if (generate_sorted_department_index ()) {
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

static int load_course_set (void) {
	char		*filename;
	char		*buffer;
	int		i, nof, len, errcnt;


	if (depbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Please load deplim first\r\n");
                return 0;
	}

	if ((filename = sysconfig->getstr ("dept-course-set-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (dept-course-set-file) "
			"not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load USOT course-set file [ %s ] ... ", filename);

	nof = 0;

	if (tio->open (filename)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct usot_depcrs_t)) continue;
			if (read_usot_course_set (buffer)) nof++;
		}
		tio->close ();
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	for (i = errcnt = 0; i < depnum; i++) {
		if ((depbuf[i].dflag & USOT_DEPT_FLAG_HAVE_CRSSET) == 0) {
			errcnt++;
		}
	}

	if (errcnt > 0) {
		misc->print (PRINT_LEVEL_SYSTEM, "%d no map\r\n", errcnt);
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%d record(s)\r\n", nof);

	return 1;
}

static int load_depna (void) {
	char		*filename;
	char		*buffer;
	int		i, nof, len, errcnt;


	if (depbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Please load deplim first\r\n");
                return 0;
	}

	if ((filename = sysconfig->getstr ("dept-name-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (dept-name-file) "
			"not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load USOT depna file [ %s ] ... ", filename);

	nof = 0;

	if (tio->open (filename)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct usot_depna_t)) continue;
			if (read_usot_depna (buffer)) nof++;
		}
		tio->close ();
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	for (i = errcnt = 0; i < depnum; i++) {
		if ((depbuf[i].dflag & USOT_DEPT_FLAG_HAVE_DEPNAME) == 0) {
			errcnt++;
		}
	}

	if (errcnt > 0) {
		misc->print (PRINT_LEVEL_SYSTEM, "%d no map\r\n", errcnt);
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%d record(s)\r\n", nof);

	return 1;
}

static int load_courses (void) {
	char			*filename;
	char			*buffer;
	int			len, count, idx;

	if ((filename = sysconfig->getstr ("course-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (course-file)"
			" not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
		"Load USOT course file [ %s ] ... ", filename);

	count = 0;

	if (tio->open (filename)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct usot_course_map_t)) continue;
			count++;
		}
		tio->close ();
		misc->print (PRINT_LEVEL_INFO, "%lu [ pass one ]\r\n", count);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_INFO, "Allocate %lu memory buffers ... ",
			count * sizeof (struct usot_course_t));

	if ((crsbuf = misc->calloc (count,
				sizeof (struct usot_course_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	} else {
		crsnum = count;
		misc->print (PRINT_LEVEL_INFO, "ok\r\n");
	}

	misc->print (PRINT_LEVEL_INFO, "Load USOT course into memory  ... ");

	if (tio->open (filename)) {
		idx = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct usot_course_map_t)) continue;
			read_usot_course (idx, buffer);
			idx++;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_SYSTEM, "ok   [ pass two ]\r\n");
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Generating sorted index for courses ... ");

	misc->timer_started ();

	if (generate_sorted_course_index ()) {
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

static struct usot_department_t * find_by_id (const int16_t idx) {
	int     i;

	if ((i = search_depid_index (idx)) == -1) return NULL;

	return &depbuf[i];
}

static void write_wishfinal (const int fd) {
	int				i, j, k;
	struct usot_department_t	*ptr;
	struct student_list_t		*sl;
	struct usot_student_on_wish_t	*sw;

	for (k = 0; k < depnum; k++) {
		for (j = 0, ptr = &depbuf[k]; ptr != NULL;
						j++, ptr = ptr->rdep) {
			for (i = 0; i < ptr->lastidx; i++) {
				fdprintf (fd, "%06d  %04d%6d%2d%d\r\n",
					ptr->stdlist[i].st->sid,
					ptr->depid,
					ptr->stdlist[i].score,
					ptr->stdlist[i].wish_index,
					j + 1);
			}

			for (sl = ptr->lastlist; sl != NULL; sl = sl->next) {
				sw = sl->ptr;

				fdprintf (fd, "%06d  %04d%6d%2d%d\r\n",
					sw->st->sid,
					ptr->depid,
					sw->score,
					sw->wish_index,
					j + 1);
			}
		}
	}
}

static void count_min_onsame (void) {
	struct usot_department_t	*ptr;
	int				i, j, k;

	for (k = 0; k < depnum; k++) {
		ptr = &depbuf[k];

		if ((i = ptr->lastidx -1) >= 0) {
			for (j = 0; j < MAX_USOT_EXAM_COURSE; j++) {
				ptr->min_onsame[j] =
						ptr->stdlist[i].onsame[j];
			}
		}
	}
}

static void write_wishminal (const int fd) {
	struct usot_department_t	*ptr;
	int				i, k;


	count_min_onsame ();

	for (k = 0; k < depnum; k++) {
		ptr = &depbuf[k];

		fdprintf (fd, "%04d%6d", ptr->depid, ptr->minreq);

		for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) {
			fdprintf (fd, "%5d", ptr->min_onsame[i]);
		}

		fdprintf (fd, "%d\r\n", ptr->max_onsame_use);
	}
}

static void intf_write_wishfinal (void) {
	char	*filename;
	int	fd;

	if ((filename = sysconfig->getstr ("wishfinal-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (wishfinal-file)"
			" not define!!\r\n");
		return;
	}

	if ((fd = open (filename, O_CREAT|O_WRONLY|O_TRUNC, 0644)) < 0) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Write wishfinal [ %s ] ... ", filename);

	write_wishfinal (fd);

	close (fd);

	misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");
}

static void intf_write_wishminimal (void) {
	char	*filename;
	int	fd;

	if ((filename = sysconfig->getstr ("wishminimal-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (wishminimal-file)"
			" not define!!\r\n");
		return;
	}

	if ((fd = open (filename, O_CREAT|O_WRONLY|O_TRUNC, 0644)) < 0) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Write wishminimal [ %s ] ... ", filename);

	write_wishminal (fd);

	close (fd);

	misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");
}


struct usot_department_module_t  *initial_usot_department_module (void) {
	struct usot_department_module_t	*self;

	lslptr = initial_lostlist_module ();
	tio = initial_textfile_io_module ();

	if ((self = malloc (sizeof (
				struct usot_department_module_t))) != NULL) {
		self->load_limits	= load_limits;
		self->load_course_set	= load_course_set;
		self->load_depna	= load_depna;
		/////////////////////////////////////////////////
		self->load_courses	= load_courses;
		/////////////////////////////////////////////////
		self->allocate_space	= allocate_space;

		self->find		= find_by_id;

		self->sort_students	= sort_students;

		self->write_wishfinal	= intf_write_wishfinal;
		self->write_wishminimal	= intf_write_wishminimal;

		self->find_course_group	= find_course_group;
	}

	return self;
}

static int compare_order (struct usot_student_on_wish_t *st1,
					struct usot_student_on_wish_t *st2) {
	int		i;
	int		debug_flag = 0;

	use_onsame = 0;

	if (st1->score < st2->score) {
		return 1;
	} else if (st1->score > st2->score) {
		return -1;
	}

	if (debug_flag) {
		fprintf (stderr, "%06d v.s. %06d (onsame):",
					st1->st->sid, st2->st->sid);
	}

	for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) {
		use_onsame++;


		if (st1->onsame[i] < st2->onsame[i]) {
			if (debug_flag) {
				fprintf (stderr, "[%d < %d]\n",
					st1->onsame[i],
					st2->onsame[i]);
			}
			return 1;
		} else if (st1->onsame[i] > st2->onsame[i]) {
			if (debug_flag) {
				fprintf (stderr, "[%d > %d]\n",
					st1->onsame[i],
					st2->onsame[i]);
			}
			return -1;
		}
		if (debug_flag) {
			fprintf (stderr, "[%d = %d]",
					st1->onsame[i],
					st2->onsame[i]);
		}
	}

	if (debug_flag) {
		fprintf (stderr, "[+] Warning: Same score ...\n");
	}

	return 0;
}

static void stow_dup (struct student_list_t *sl,
					struct usot_student_on_wish_t *sw) {
	if (sl->ptr == NULL) {
		sl->ptr = malloc (sizeof (struct usot_student_on_wish_t));
	}

	if (sl->ptr != NULL) {
		memcpy (sl->ptr, sw, sizeof (struct usot_student_on_wish_t));
	}
}

static int sort_students (struct usot_department_t *dp) {
	int			i, j, k;
	int			order;
	int			addorder  = 1;	// 先假定需要增加名次
	int			retval    = 0;
	int			lastorder;	// 最後一名的名次
	int			myorder = 0;
	struct student_list_t	*sl;

	// 只使用在學生榜上, 其實是使用 Insertion sort ....
	//
	// 這個演算法有點小慢 .....

	// 注意原則 ... 只要能塞得下, 就不折
	// 一但折了之後, 要出去就一起出去, 沒有折了之後再塞的事 ...

	k = dp->lastidx;

	for (i = 0; i < k - 1; i++) {
		myorder = dp->stdlist[i].order;

		order = compare_order (&dp->stdlist[i], &dp->stdlist[k]);

		// use_onsame 記錄同分比較次數

		// onsame_cnt

		if (order > 0) {
			retval = 1;   // 錄取了 ...
			break;
		} else if (order == 0) {
			addorder = 0; // 不增加名次
			retval = 1;   // 一定錄取, 至少增額錄取
			break;
		}

		myorder++;
	}

	dp->stdlist[k].order = myorder;


	for (j = k - 1; j > i; j--) {
		// 往後推
		memcpy (&dp->stdlist[j], &dp->stdlist[j-1],
				sizeof (struct usot_student_on_wish_t));
		// 看看要不要增加名次
		if (addorder) dp->stdlist[j].order++;
	}

	// 真正塞入位置
	memcpy (&dp->stdlist[i], &dp->stdlist[k],
				sizeof (struct usot_student_on_wish_t));

	// 看看名額限制 ...
	
	if (dp->inlistcnt  < dp->num) return 1;	// 名額未滿
	if (dp->inlistcnt == dp->num) {		// 名額剛好 ... 寫最低錄取分
		dp->minreq      = dp->stdlist[k-1].score;
		return 1;
	}

	// 她沒有比別人高分, 又沒有和別人同分, 名額又滿了
	if (! retval) {
		dp->lastidx--;
		dp->inlistcnt--;

		if (dp->max_onsame_use < use_onsame)
					dp->max_onsame_use= use_onsame;
		return 0;
	}

	////////////////////////////////////////////////////
	// 準備增額錄取或擠出一些學生


	// 取得最後一名的名次
	lastorder = dp->stdlist[k-1].order;


	if (dp->llindex > 0) {	// 已經折了
		if (myorder == lastorder) {	// 她剛好是最後一名
						// 只好跟大家一起擠
			sl = allocate_student_list (1);
			sl->next = (void *) dp->lastlist;
			sl->st   = (void *) dp->stdlist[--dp->lastidx].st;

			stow_dup (sl, &dp->stdlist[dp->lastidx]);

			dp->lastlist = (void *) sl;
			dp->llindex++;	// 在 lastlist 的人數
					// array 使用的指標
					// 降回
			return 1;	// 暫時入選
		} else if (k - 1 >= dp->num) {

			// 踢掉一堆同分最後一名的人
			// -- 同分最後一名的代表 user
			sl = allocate_student_list (1);
			sl->next = (void *) dp->lastlist;
			sl->st   = (void *) dp->stdlist[--dp->lastidx].st;

			stow_dup (sl, &dp->stdlist[dp->lastidx]);

			// [?] 重算一下同分參酌到第幾序
			order = compare_order (
					&dp->stdlist[dp->lastidx - 1],
					&dp->stdlist[dp->lastidx]);

			dp->max_onsame_use = use_onsame;

			dp->inlistcnt = dp->inlistcnt - dp->llindex - 1;
			// 將 lastlist 丟回 queue
			// lslptr->put_list (dp->lastlist);
			lslptr->put_list (sl);
			dp->lastlist = NULL;
			dp->llindex = 0;
			// ??
			dp->minreq = dp->stdlist[dp->lastidx-1].score;
			
			return 2;
		}

		// 眾多的同分最後一名
		return 1;
	} else {
		// 算一算最後一名的人數有幾人

		for (i = k - 1, j = 0; i >= 0; i--) {
			if (dp->stdlist[i].order == lastorder) {
				j++;
			} else {
				break;
			}
		}

		if (j == 1) {

			// 踢掉最後這個人
			dp->inlistcnt--;
			lslptr->put ((void *) dp->stdlist[--dp->lastidx].st);
			// 把這個人丟回 queue

			// [?] 重算一下同分參酌到第幾序
			order = compare_order (
					&dp->stdlist[dp->lastidx - 1],
					&dp->stdlist[dp->lastidx]);

			dp->max_onsame_use = use_onsame;

			// ??
			dp->minreq = dp->stdlist[dp->lastidx-1].score;

			return 2;
		} else {
			// 把同名次的這堆人折起來

			sl = allocate_student_list (j - 1);
			dp->lastlist = (void *) sl;

			while (sl != NULL) {
				sl->st = (void *) dp->stdlist[--dp->lastidx].st;
				stow_dup (sl, &dp->stdlist[dp->lastidx]);
				sl = sl->next;
				dp->llindex++;
			}

			// ??
			dp->minreq = dp->stdlist[dp->lastidx-1].score;
			return 1;
		}
	}
	// return retval;
}
