/*
 *	student.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

// #define COUNT_GRADE_STANDARD_SCORE

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "predefine.h"
#include "student.h"
#include "textio.h"
#include "global_var.h"
#include "misclib.h"
#include "sys_conf.h"
#include "dtstruct.h"
#include "postcheck.h"

#define NUMBER_OF_WISHES		MAX_PER_STUDENT_WISH
#define WISHES_FIELD_LENGTH		4

// #define INFOFILE_START_AGAINST	34	/* 是否違規 */

struct student_indexes_t {
	int	sid;
	int	idx;
};

typedef struct student_t *student_ptr_t;

static struct student_extend_info_t		*stdinfo  = NULL;
static struct student_extend_info_level2_t	*stdinfo2 = NULL;

static struct student_module_t		st_module;
static struct textfile_io_t		*tio = NULL;
static struct student_t			*stdbuf = NULL;
static int				stdnum = 0;
static struct student_indexes_t		*stdsortedindex = NULL;
#if ENABLE_CEEC_FORMAT == 1
#  if ENABLE_CEEC_MERGE_SN == 1
static struct student_indexes_t		*stdsnsortedindex = NULL;
#  else
static struct student_indexes_t		*stdidsortedindex[3] = {
						NULL, NULL, NULL
					};
#  endif
static int				course_order_patch[] = {
						1, 2, 3, 4, 5, 6, 7, 8, 9
					};
#endif
static student_ptr_t			*stdstack = NULL;
static int				stkcount = 0;
static int				stktop = 0;

////////////////////////////////////////////////////////////////////

#if ENABLE_INCREMENTAL_ALGORITHM == 1
static int			load_all_students	(const int dummywish);
#endif
static int			load_score		(const int additional);
static int			load_wishes		(const int additional,
							 const int addonly);
static int			load_new_wishes		(void);
static int			load_info		(const int additional);
#if ENABLE_CHECKLIST == 1
static int			load_checklist		(const int level);
static int			load_checklist_all	(const int level);
#endif
//static int			load_special		(void);
static int			prepare_sorted_stack	(const int sorted);
static void			free_stack		(void);
static struct student_t *	st_pop			(void);
static int			st_push			(struct student_t *st);
static void			print_all		(FILE *);
static int16_t			nextwish		(struct student_t *st);
static int16_t			nextwish_with_order	(struct student_t *st,
								int *order);
static void			reset_wish		(struct student_t *st);
static int			get_wish_index		(struct student_t *st);
static int			number_of_students	(void);
//static struct student_flag_t *	st_special	(struct student_t *st);

////////////////////////////////////////////////////////////////////

static int generate_sorted_student_index (void);
static int search_sid_index (const int32_t sid);
static struct student_t * find_by_id (const int32_t);
static int order_by_sid (const struct student_indexes_t *i,
				const struct student_indexes_t *j);
#if ENABLE_CEEC_FORMAT == 1
static int generate_sorted_student_sn_index (void);
static int search_stdsn_index (const int32_t sid);
#endif

#ifdef COUNT_GRADE_STANDARD_SCORE
static void calculate_standard_score (void);
#endif

#if ENABLE_DEPARTMENT_FIRST == 1
static void set_wishidx (struct student_t *st, const int nw, const int idx);
static void set_valid_wish_mask (struct student_t *st, const short *mask);
#endif

#if ENABLE_DENYLIST == 1
static int load_deny_list (void);
static int is_wish_deny (const int32_t sid, const int16_t did);
#endif

static void get_student_special_flag (const int idx, const char key);

#if ENABLE_DISPATCHER92 == 1
#include "code92/stud92.c"
#endif
////////////////////////////////////////////////////////////////////

#if ENABLE_PARALLEL_ALGORITHM == 1
static struct student_t * get_stdbuf (void) {
	return stdbuf;
}
#endif

struct student_module_t * initial_student_module (void) {
	tio = initial_textfile_io_module ();

	st_module.load_wishes		= load_wishes;
	st_module.load_new_wishes	= load_new_wishes;
#if ENABLE_INCREMENTAL_ALGORITHM == 1
	st_module.load_all_students	= load_all_students;
#endif
	st_module.load_score		= load_score;
#if ENABLE_CHECKLIST == 1
	st_module.load_checklist	= load_checklist;
	st_module.load_checklist_all	= load_checklist_all;
#endif
	st_module.load_info		= load_info;
	st_module.prepare_sorted_stack	= prepare_sorted_stack;
	st_module.free_stack		= free_stack;
	// st_module.load_special  	= load_special;
	st_module.pop			= st_pop;
	st_module.push			= st_push;
	st_module.nextwish		= nextwish;
	st_module.nextwish_with_order	= nextwish_with_order;
	st_module.reset_wish		= reset_wish;
	st_module.get_wish_index	= get_wish_index;
	st_module.number_of_students	= number_of_students;
	st_module.find			= find_by_id;
	st_module.print_all		= print_all;
	/*
	st_module.check_special		= st_special;
	*/
#if ENABLE_DEPARTMENT_FIRST == 1
	st_module.set_valid_wish_mask	= set_valid_wish_mask;
	st_module.set_wishidx		= set_wishidx;
#endif
#if ENABLE_PARALLEL_ALGORITHM == 1
	st_module.get_stdbuf		= get_stdbuf;
#endif
#if ENABLE_DENYLIST == 1
	st_module.load_deny		= load_deny_list;
	st_module.deny			= is_wish_deny;
#endif

	return &st_module;
}

/////////////////////////////////////////////////////////////////////

static void initialize_student_info (const int idx) {
	int	i;

#if ENABLE_CEEC_FORMAT == 1
#  if ENABLE_CEEC_MERGE_SN == 1
	stdbuf[idx].sn  = -1;	// 尚不知道學生的流水序號
				// 在 load student info 之後才填上正確值
#  else
	stdbuf[idx].usedid[0] = stdbuf[idx].usedid[1]
			      = stdbuf[idx].usedid[2] = -1;
#  endif
#endif

	for (i = 0; i < 5; i++) stdbuf[idx].gradscore[i] = -1;
	for (i = 0; i < 5; i++) stdbuf[idx].gradmiss[i]  = 0;
	for (i = 0; i < 9; i++) stdbuf[idx].testscore[i] = -1;
	for (i = 0; i < 9; i++) stdbuf[idx].testmiss[i]  = 0;

#if ENABLE_DEPARTMENT_FIRST == 1
	stdbuf[idx].wishidx = NULL;
	stdbuf[idx].order_on_wish = NULL;
#endif

	stdbuf[idx].number_of_wishes = 0;

	// printf ("%s\r\n", buffer);

	stdbuf[idx].wish_index   = 0;
	stdbuf[idx].sflag        = 0;

	stdbuf[idx].score_disallow = 0;

	stdbuf[idx].matched_wish = -1;
	stdbuf[idx].final_score  = -1;
	stdbuf[idx].info	 = NULL;
#if ENABLE_DISPATCHER92 == 1
	stdbuf[idx].skinfo	 = NULL;
#endif
#if ENABLE_DEPARTMENT_FIRST
	stdbuf[idx].atleast_wish = MAX_PER_STUDENT_WISH;
#endif
	stdbuf[idx].highsch      = 0;

	stdbuf[idx].org_wish = NULL;
}

static void read_student_wish_file_record (const int index,
						const char *buffer) {
#if ENABLE_DISPATCHER92 == 1
	struct ncu_stdwishes92	*ptr92 = (void *) buffer;
	int32_t			sid;
#endif
	struct ncu_stdwishes	*ptr = (void *) buffer;
	int			i;
	int			idx = index;

	// bzero (stdbuf[idx], sizeof (struct student_t));

#if ENABLE_INCREMENTAL_ALGORITHM == 1
	if (index < 0) {
		if (use_disp92) {
			sid = misc->dec (ptr92->studsid, STUDENT_ID_LENGTH);

			if (sid == 0) {
				sid = misc->dec (ptr92->studcid,
							STUDENT_ID_LENGTH);
			}
		} else {
			sid = misc->dec (ptr->studid, STUDENT_ID_LENGTH);
		}

		if ((idx = search_stdsn_index (sid)) < 0) {
			fprintf (stderr, "Student %d not found !!\n", sid);
			return;
		}
	}

	if (index == -2) {
		fprintf (stderr, "[%d]\n", stdbuf[idx].sid);
		// 將原志願複製到 org_wish
		stdbuf[idx].org_wish = misc->malloc (NUMBER_OF_WISHES *
				sizeof (int16_t));

		for (i = 0; i < NUMBER_OF_WISHES; i++) {
			stdbuf[idx].org_wish[i] = stdbuf[idx].wish[i];
		}
	}
#endif

	if (index >= 0) {
#if ENABLE_DISPATCHER92 == 1
		stdbuf[idx].extsid = 0;

		if (use_disp92) {
			sid = misc->dec (ptr92->studsid, STUDENT_ID_LENGTH);


			if (sid == 0) {
				sid = misc->dec (
					ptr92->studcid, STUDENT_ID_LENGTH);
#if DEBUG_LEVEL > 3
				if (ptr92->studsid[0] != '1') {
					fprintf (stderr,
						"OOP! Student C_SID = %d\n",
						sid);
				}
#endif
			} else {
#if DEBUG_LEVEL > 3
				if (ptr92->studsid[0] != '2') {
					fprintf (stderr,
						"OOP! Student S_SID = %d\n",
						sid);
				}
#endif
				stdbuf[idx].extsid = misc->dec (
					ptr92->studcid, STUDENT_ID_LENGTH);
			}

#  if ENABLE_INCREMENTAL_ALGORITHM == 1
			if (index >= 0) stdbuf[idx].sid = sid;
#  else
			stdbuf[idx].sid = sid;
#  endif
		} else {
#  if ENABLE_INCREMENTAL_ALGORITHM == 1
			if (index >= 0) {
				stdbuf[idx].sid = misc->dec (
						ptr->studid, STUDENT_ID_LENGTH);
			}
#  else
			stdbuf[idx].sid = misc->dec (ptr->studid, STUDENT_ID_LENGTH);
#  endif
		}
#else
		stdbuf[idx].sid = misc->dec (ptr->studid, STUDENT_ID_LENGTH);
#endif

		initialize_student_info (idx);
	}

	// 不管怎麼樣, 這個號碼不能, 也不應該重覆.
	// 如果真的重覆, 或 missing student info or score
	// 也應該在大考中心就 take care ...

	// if (index >= 0) initialize_student_info (idx);

#if ENABLE_DEPARTMENT_FIRST == 1
	if (index >= 0) {
		if (selected_algorithm != 1) {
			stdbuf[idx].wishidx = misc->malloc (
				MAX_PER_STUDENT_WISH * sizeof (int16_t));
			stdbuf[idx].order_on_wish = misc->malloc (
				MAX_PER_STUDENT_WISH * sizeof (int16_t));
		}
	}
#endif

	// if (index >= 0) stdbuf[idx].org_wish = NULL;

	for (i = 0; i < NUMBER_OF_WISHES; i++) {
#if ENABLE_DISPATCHER92 == 1
		if (use_disp92) {
			stdbuf[idx].wish[i] = misc->oct (ptr92->wishes[i],
							WISHES_FIELD_LENGTH);
		} else {
			stdbuf[idx].wish[i] = misc->oct (ptr->wishes[i],
							WISHES_FIELD_LENGTH);
		}
#else
		stdbuf[idx].wish[i] = misc->oct (ptr->wishes[i],
							WISHES_FIELD_LENGTH);
#endif
		if (stdbuf[idx].wish[i] != 0) stdbuf[idx].number_of_wishes ++;
#if ENABLE_DEPARTMENT_FIRST == 1
		if (selected_algorithm != 1) {
			stdbuf[idx].wishidx[i]       = -1;
			stdbuf[idx].order_on_wish[i] = -1;
		}
#endif
	}

#if 0
#if ENABLE_CEEC_FORMAT == 1
#  if ENABLE_CEEC_MERGE_SN == 1
	stdbuf[idx].sn  = -1;	// 尚不知道學生的流水序號
				// 在 load student info 之後才填上正確值
#  else
	stdbuf[idx].usedid[0] = stdbuf[idx].usedid[1]
			      = stdbuf[idx].usedid[2] = -1;
#  endif
#endif

	for (i = 0; i < 5; i++) stdbuf[idx].gradscore[i] = -1;
	for (i = 0; i < 9; i++) stdbuf[idx].testscore[i] = -1;

#if ENABLE_DEPARTMENT_FIRST == 1
	stdbuf[idx].wishidx = NULL;
	stdbuf[idx].order_on_wish = NULL;

	if (selected_algorithm != 1) {
		stdbuf[idx].wishidx = misc->malloc (
			MAX_PER_STUDENT_WISH * sizeof (int16_t));
		stdbuf[idx].order_on_wish = misc->malloc (
			MAX_PER_STUDENT_WISH * sizeof (int16_t));
	}
#endif

	stdbuf[idx].number_of_wishes = 0;

	for (i = 0; i < NUMBER_OF_WISHES; i++) {
#if ENABLE_DISPATCHER92 == 1
		if (use_disp92) {
			stdbuf[idx].wish[i] = misc->oct (ptr92->wishes[i],
							WISHES_FIELD_LENGTH);
		} else {
			stdbuf[idx].wish[i] = misc->oct (ptr->wishes[i],
							WISHES_FIELD_LENGTH);
		}
#else
		stdbuf[idx].wish[i] = misc->oct (ptr->wishes[i],
							WISHES_FIELD_LENGTH);
#endif
		if (stdbuf[idx].wish[i] != 0) stdbuf[idx].number_of_wishes ++;
#if ENABLE_DEPARTMENT_FIRST == 1
		if (selected_algorithm != 1) {
			stdbuf[idx].wishidx[i]       = -1;
			stdbuf[idx].order_on_wish[i] = -1;
		}
#endif
	}

	// printf ("%s\r\n", buffer);

	stdbuf[idx].wish_index   = 0;
#if ENABLE_INCREMENTAL_ALGORITHM == 1
	if (index >= 0) stdbuf[idx].sflag = 0;
#else
	stdbuf[idx].sflag        = 0;
#endif
	stdbuf[idx].matched_wish = -1;
	stdbuf[idx].final_score  = -1;
	stdbuf[idx].info	 = NULL;
#if ENABLE_DISPATCHER92 == 1
	stdbuf[idx].skinfo	 = NULL;
#endif
#if ENABLE_DEPARTMENT_FIRST
	stdbuf[idx].atleast_wish = MAX_PER_STUDENT_WISH;
#endif
#endif
}

#if ENABLE_INCREMENTAL_ALGORITHM == 1
static int load_all_students (const int dummywish) {
	char			*filename;
	char			*buffer;
	int			minlen = 1024;
	int			len, idx, cnt;
	int32_t			sid;
	struct ncu_stdinfo	*ncu;
	struct ceec_stdinfo	*ceec;
	struct ceec_stdinfo92	*ceec92;
	int			sn = 0;

	if ((filename = sysconfig->getstr ("student-info-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
		    "%% system variable (student-info-file) not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "Load all students from ");

	if (use_ncu_format) {
		minlen = sizeof (struct ncu_stdinfo); 
		misc->print (PRINT_LEVEL_SYSTEM, "NCU");
	} else if (use_disp92) {
		minlen = sizeof (struct ceec_stdinfo);
		misc->print (PRINT_LEVEL_SYSTEM, "CEEC92");
	} else {
		minlen = sizeof (struct ceec_stdinfo92);
		misc->print (PRINT_LEVEL_SYSTEM, "CEEC");
	}

	misc->print (PRINT_LEVEL_SYSTEM, " format [ %s ] ... ", filename);

	if (tio->open (filename)) {
		cnt = 0;
		while ((len = tio->read (&buffer)) >= 0) {
			if (len >= minlen) cnt++;
		}

		tio->close ();
		misc->print (PRINT_LEVEL_SYSTEM, "%d read.\r\n", cnt);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_INFO,
			"Allocate %lu memory buffers ... ",
			cnt * sizeof (struct student_t));

	if ((stdbuf = misc->calloc (cnt, sizeof (struct student_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	} else {
		stdnum = cnt;
		misc->print (PRINT_LEVEL_INFO, "ok\r\n");
	}

	misc->print (PRINT_LEVEL_INFO,
				"Load student info into memory  ... ");

	if (tio->open (filename)) {
		idx = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < minlen) continue;

			if (use_ncu_format) {
				ncu = (void *) buffer;
				sid = misc->dec (ncu->studid,
							STUDENT_ID_LENGTH);
			} else if (use_disp92) {
				ceec92 = (void *) buffer;
				sid = misc->dec (ceec92->sid_exam,
							STUDENT_ID_LENGTH);

				if (sid <= 0) {
					sid = misc->dec (
							ceec92->sid_basic,
							STUDENT_ID_LENGTH);
				}
				sn = misc->dec (ceec92->serial,
						sizeof ceec92->serial);
			} else {
				ceec = (void *) buffer;
				sid = misc->dec (ceec->sid_exam,
							STUDENT_ID_LENGTH);

				if (sid <= 0) {
					sid = misc->dec (
							ceec->sid_basic,
							STUDENT_ID_LENGTH);
				}
				sn = misc->dec (ceec->serial,
						sizeof ceec->serial);
			}

			if (sid <= 0) {
				/*
				fprintf (stderr,
					"OOPS! current index = %d (sn=%06d)\n",
						idx,
						sn);
				*/
				continue;
			}

			stdbuf[idx].sid      = sid;

			initialize_student_info (idx);
#if 0
			stdbuf[idx].sflag	  = 0;
			stdbuf[idx].wishidx       = NULL;
                	stdbuf[idx].order_on_wish = NULL;
			stdbuf[idx].info	  = NULL;
			stdbuf[idx].extsid	  = 0;
#endif
			idx++;
		}

		tio->close ();

		if (idx != stdnum) {
			stdnum = idx;
			misc->print (PRINT_LEVEL_SYSTEM, "[sad but true]");
		}

		misc->print (PRINT_LEVEL_SYSTEM, "%d, [ pass two ]\r\n",
					idx, cnt);

	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->timer_started ();

	misc->print (PRINT_LEVEL_SYSTEM,
			"Generating sorted index for %d students ... ",
			stdnum);

	if (generate_sorted_student_index ()) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"ok.  (%.4f seconds)\r\n",
					misc->timer_ended());
	} else {
		misc->print (PRINT_LEVEL_SYSTEM, "failed\r\n");
		return 0;
	}

	if (dummywish) {
		int	i;

		for (idx = 0; idx < stdnum; idx++) {
			for (i = 0; i < NUMBER_OF_WISHES; i++) {
				stdbuf[idx].wish[i] = i + 1;
			}
		}
	}

	return 1;
}

static int load_wishes_for_all_students (const char *filename) {
	int	len, minlen;
	char	*buffer;

	if (use_disp92) {
		minlen = sizeof (struct ncu_stdwishes92);
	} else {
		minlen = sizeof (struct ncu_stdwishes);
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load all student wishes file [ %s ] ... ", filename);

	if (tio->open (filename)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < minlen) continue;
			if (buffer[0] == ';') continue;

			read_student_wish_file_record (-1, buffer);
		}

		tio->close ();
		misc->print (PRINT_LEVEL_SYSTEM,
				"ok   [ pass two ]\r\n");
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	return 1;
}
#endif


static int real_load_wishes (const int is_first, const char *filename,
				const int additional, const int addonly) {
	// char		*filename;
	int		len, shortline = 0;
	int		idx = 0;
	unsigned long	count = 0L;
	char		*buffer;
	int		total_wishes = 0;
	int		wishes_cnt = 0;
	int		minlen;
#if DEBUG_LEVEL > 0
	char		*patchfile = NULL;
	unsigned long	pcount = 0L;
#endif

#if 0
	if ((filename = sysconfig->getstr ("student-wishes-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable (student-wishes-file) "
				"not define!!\r\n");
		return 0;
	}
#endif

#if ENABLE_INCREMENTAL_ALGORITHM == 1
	if (is_first) {
		if (stdbuf != NULL) {
			return load_wishes_for_all_students (filename);
		}
	}
#endif

	minlen = sizeof (struct ncu_stdwishes);

#if ENABLE_DISPATCHER92 == 1
	if (use_disp92) minlen = sizeof (struct ncu_stdwishes92);
#endif

	if (! addonly) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Load student wishes file [ %s ] ... ",
				filename);

		if (tio->open (filename)) {
			// 計算出學生筆數, Allocate Space for students
			count = shortline = 0;

			while ((len = tio->read (&buffer)) >= 0) {
				if (len >= minlen) {
					if (buffer[0] != ';') {
						count++;
						if (! is_first) {
						 read_student_wish_file_record (
								-2, buffer);
						}
					}
				} else {
					shortline++;
				}
			}

			/*
			misc->print (PRINT_LEVEL_SYSTEM,
					"%lu (pass one) [ %d discard ]\r\n",
					count, shortline);
				*/
			misc->print (PRINT_LEVEL_INFO,
					"%lu   [ pass one ]\r\n", count);

			tio->close ();
		} else {
			misc->perror (PRINT_LEVEL_SYSTEM);

			return 0;
		}
	}

	if (! is_first) return 1;

#if DEBUG_LEVEL > 0
	if ((addonly || additional) && ((patchfile = sysconfig->getstr (
				"student-wishes-additional")) != NULL)) {

		misc->print (PRINT_LEVEL_INFO,
			"Load additional student wishes file [ %s ] ... ",
			patchfile);
		pcount = 0;

		if (tio->open (patchfile)) {
			while ((len = tio->read (&buffer)) >= 0) {
				if (len >= minlen) {
					if (buffer[0] != ';') pcount++;
				}
			}

			tio->close ();
			misc->print (PRINT_LEVEL_INFO,
					"%lu record(s)\r\n", pcount);

			count += pcount;
		} else {
			misc->perror (PRINT_LEVEL_INFO);
		}
	}
#endif

	// allocate student buffer
	
	if (is_first) {
		misc->print (PRINT_LEVEL_INFO,
				"Allocate %lu memory buffers ... ",
				count * sizeof (struct student_t));

		if ((stdbuf = misc->calloc (count,
					sizeof (struct student_t))) == NULL) {
			misc->perror (PRINT_LEVEL_SYSTEM);
			return 0;
		} else {
			stdnum = count;
			misc->print (PRINT_LEVEL_INFO, "ok\r\n");
		}
	}


	if (! addonly) {
		misc->print (PRINT_LEVEL_INFO,
				"Load student wishes into memory  ... ");

		if (tio->open (filename)) {
			for (idx = 0; (len = tio->read (&buffer)) >= 0; ){
				if (len < minlen) continue;
				if (buffer[0] == ';') continue;

				read_student_wish_file_record (idx, buffer);

				total_wishes += stdbuf[idx].number_of_wishes;
				wishes_cnt++;
				++idx;
			}

			tio->close ();
			misc->print (PRINT_LEVEL_SYSTEM,
					"ok   [ pass two ]\r\n");

			/*
			printf ("Total: Sum=%d, Cnt=%d, Avg=%7.5f\n",
				total_wishes, wishes_cnt,
				((float) total_wishes / (float) wishes_cnt));
			*/
		} else {
			misc->perror (PRINT_LEVEL_SYSTEM);
			return 0;
		}
	}

#if DEBUG_LEVEL > 0
	if (pcount > 0L) {
		misc->print (PRINT_LEVEL_INFO,
			"Load additional student wishes into memory  ... ");

		if (tio->open (patchfile)) {
			for (; (len = tio->read (&buffer)) >= 0; ){
				if (len < minlen)
					continue;
				if (buffer[0] == ';') continue;

				read_student_wish_file_record (idx, buffer);
				++idx;
			}

			tio->close ();
			misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");
		} else {
			misc->perror (PRINT_LEVEL_SYSTEM);
		}
	}
#endif

	// 改在 load wish 之後就直接去 gen index

	misc->timer_started ();

	misc->print (PRINT_LEVEL_SYSTEM,
			"Generating sorted index for students ... ");

	if (generate_sorted_student_index ()) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"ok.  (elapsed time: %.4f seconds)\r\n",
					misc->timer_ended());
	} else {
		misc->print (PRINT_LEVEL_SYSTEM, "failed\r\n");
		return 0;
	}

	return 1;
}

static int load_new_wishes (void) {
	char	*filename;
	// int	i;

	if (stdbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM, "Load wish first\n");
		return 0;
	}

	if ((filename = sysconfig->getstr (
					"student-new-wishes-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable (student-new-wishes-file) "
				"not define!!\r\n");
		return 0;
	}

	return real_load_wishes (0, filename, 0, 0);
}

static int load_wishes (const int additional, const int addonly) {
	char	*filename;

	if ((filename = sysconfig->getstr ("student-wishes-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable (student-wishes-file) "
				"not define!!\r\n");
		return 0;
	}

	return real_load_wishes (1, filename, additional, addonly);
}
static int read_student_score_file_record (const char *buffer) {
		// 當成績檔的次序與系組(志願)限制檔的格式不同時, 要做
		// 些 patch
// #define NEED_ORDER_PATCH
// #define SKILL_NEED_ORDER_PATCH
	struct ncu_score	*ptr = (struct ncu_score *) buffer;

#ifdef NEED_ORDER_PATCH
	const int	po[] = { 0, 1, 3, 2, 7, 6, 8, 4, 5 };
#endif
#ifdef SKILL_NEED_ORDER_PATCH
	const int	skillpatch[] = { 0, 1, 2, 7, 3, 4, 5, 6, 8 };
		/// int	skillpatch[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
#endif
	int		j, k, idx, sid;
	int		sum, gsc;

	sid = misc->dec (ptr->studid, STUDENT_ID_LENGTH);
	// i++;

	if ((idx = search_sid_index (sid)) != -1) {
		// nof++;
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_SCORE;

#if DEBUG_LEVEL > 9
		dprint ("5 score [");
#endif
		// 5 科級分
		for (k = sum = 0; k < 5; k++) {
			if (ptr->grade[k][0] == '*') {
				gsc = 0;
				stdbuf[idx].gradmiss[k] = 1;
			} else if (ptr->grade[k][0] == '-') {
				sum = -1;
				break;
			} else {
				gsc = misc->dec (ptr->grade[k], 2);
			}
			j += 2;

			if ((gsc >= 0) && (gsc <= 15)) {
				stdbuf[idx].gradscore[k] = gsc;
				sum += gsc;
			} else {
				misc->print (PRINT_LEVEL_SYSTEM,
					"\r\n\r\nIllegal grade score: %d "
					"for %d%c\r\n\r\n",
					gsc, stdbuf[idx].sid, 7);
				sum = -1;
				break;
			}
#if DEBUG_LEVEL > 9
			dprint (" %d", gsc);
#endif
			gsc = 0;
		}

		if (sum == -1) {
			// 只要有一科被註記成─不用考
			// 所有科目當成不用考 (應該是種錯誤)
			for (k = 0; k < 5; k++) {
				stdbuf[idx].gradscore[k] = -1;
			}
		}

		stdbuf[idx].gradsum = sum;
#if DEBUG_LEVEL > 9
		dprint ("] sum = %d\r\n", sum);
#endif

#if DEBUG_LEVEL > 1
		if (trace_sid == stdbuf[idx].sid) {
			misc->print (PRINT_LEVEL_DEBUG,
				"Grade Test SUM for %d = %d\n",
				trace_sid, sum);
		}
#endif

		// 若補考通過, 則將學科能力測驗視為通過
		if (ptr->pass == '1') {
			stdbuf[idx].sflag |= STUDENT_FLAG_PASS1;
		}

#if DEBUG_LEVEL > 9
		dprint ("9 score [");
#endif
		// 九科
		for (k = 0; k < 9; k++) {
#ifdef NEED_ORDER_PATCH
			if (ptr->score[po[k]][0] == '*') {
				stdbuf[idx].testmiss[k] = 1;
			}

			stdbuf[idx].testscore[po[k]] =
					misc->score_conv (ptr->score[k]);
#else
			if (ptr->score[k][0] == '*') {
				stdbuf[idx].testmiss[k] = 1;
			}

			stdbuf[idx].testscore[k] =
					misc->score_conv (ptr->score[k]);
#endif

		}
#if DEBUG_LEVEL > 9
		dprintw ("]\r\n");
#endif

		// 術科組別
		//
		j = misc->dec (ptr->skillgroup, 1);
#ifdef SKILL_NEED_ORDER_PATCH
		j = skillpatch[j];
#endif

		stdbuf[idx].skill_group = j;
#if SKILL_CHECK_BY_CALCULATE == 0
		stdbuf[idx].skillcheck = 0;
#endif
		stdbuf[idx].skillmajor_score = 0;
		stdbuf[idx].skillmajor = 0;
		stdbuf[idx].skillminor = 0;

#if ENABLE_ONSAME_FAILECNT == 1
		stdbuf[idx].onsame_failcnt = 0;
		stdbuf[idx].grp_onsame_failcnt[0]
			= stdbuf[idx].grp_onsame_failcnt[1]
			= stdbuf[idx].grp_onsame_failcnt[2]
			= stdbuf[idx].grp_onsame_failcnt[3] = 0;
#endif
#if ENABLE_CHALLENGE_CNT == 1
		stdbuf[idx].challenge_cnt = 0;
#endif

		if (j != 0) {
			// 術科成績

			if (ptr->skillscore[0] == '*') {
				stdbuf[idx].sflag |= STUDENT_FLAG_SKILL_MISS;
			}

			stdbuf[idx].skillscore =
					misc->score_conv (ptr->skillscore);

#if SKILL_CHECK_BY_CALCULATE == 0
			stdbuf[idx].skillcheck = misc->score_conv (
					ptr->reserved);
#endif

			stdbuf[idx].skillmajor_score =
				misc->score_conv (ptr->skillmajor_score);

			stdbuf[idx].skillmajor = misc->dec (
					&ptr->skillmajor[1], 4);

			stdbuf[idx].skillminor = misc->dec (
					&ptr->skillminor[1], 4);
		}
	}

	return idx;
}

static int load_NCU_format_score (const int additional) {
	char	*filename;
	char	*buffer;
	int	i, len, nof;


	if ((filename = sysconfig->getstr ("student-score-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
		    "%% system variable (student-score-file) not define!!\r\n");
		return 0;
	}


	misc->print (PRINT_LEVEL_SYSTEM,
			"Load NCU format score file [ %s ] ... ",
			filename);

	if (tio->open (filename)) {
		i = nof = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct ncu_score)) continue;

			if (read_student_score_file_record (buffer) != -1)
				nof++;

			i++;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_SYSTEM, "%d read, %d use\r\n", i, nof);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);

		return 0;
	}

#if DEBUG_LEVEL > 0
	if (additional && ((filename = sysconfig->getstr (
				"student-score-additional")) != NULL)) {
		misc->print (PRINT_LEVEL_INFO,
			"Load additional student score file [ %s ] ... ",
			filename);

		if (tio->open (filename)) {
			i = nof = 0;

			while ((len = tio->read (&buffer)) >= 0) {
				if (len < sizeof (struct ncu_score)) continue;

				if (read_student_score_file_record (
							buffer) != -1) nof++;

				i++;
			}
			tio->close ();

			misc->print (PRINT_LEVEL_SYSTEM,
					"%d read, %d use\r\n", i, nof);
		} else {
			misc->perror (PRINT_LEVEL_SYSTEM);
		}
	}
#endif


#ifdef COUNT_GRADE_STANDARD_SCORE
	calculate_standard_score ();
#endif

	return 1;
}

#if ENABLE_CEEC_FORMAT == 1
static int read_ceec_basic_score_file_record (char *buffer) {
	struct ceec_grade_score		*ptr = (void *) buffer;
#if ENABLE_DISPATCHER92 == 1
	struct ceec_grade_score92	*ptr92 = (void *) buffer;
#endif
	int			k, idx, sid;
	int			xsum, sum, gsc;

	// sid = misc->dec (&buffer[SCOREFILE_START_SID], STUDENT_ID_LENGTH);

#if ENABLE_CEEC_MERGE_SN == 1
	sid = misc->dec (ptr->serial, STUDENT_CEEC_SN_LENGTH);
#else
	sid = misc->dec (ptr->sid_basic, STUDENT_ID_LENGTH);
#endif

	if ((idx = search_stdsn_index (sid)) != -1) {
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_SCORE;
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_CEEC_BASIC;

#if ENABLE_DISPATCHER92 == 1
		if ((use_disp92) && (ptr92->disallow[0] == 'Y')) {
			stdbuf[idx].score_disallow |= STSCD_DISALLOW_BASIC;
		}
#endif

		for (k = sum = 0; k < 5; k++) {
			if (ptr->grade[k][0] == '*') {
				gsc = 0;
				stdbuf[idx].gradmiss[k] = 1;
			} else if (ptr->grade[k][0] == '-') {
				sum = -1;
				break;
			} else {
				gsc = misc->dec (ptr->grade[k], 2);
			}

			if ((gsc >= 0) && (gsc <= 15)) {
				stdbuf[idx].gradscore[k] = gsc;
				sum += gsc;
			} else {
				misc->print (PRINT_LEVEL_SYSTEM,
					"\r\n\r\nIllegal grade score: %d "
					"for %d%c\r\n\r\n",
					gsc, stdbuf[idx].sid, 7);
				sum = -1;
				break;
			}
#if DEBUG_LEVEL > 9
			dprint (" %d", gsc);
#endif
			gsc = 0;
		}

		if (ptr->sum[0] == '-') {
			xsum = -1;
		} else if (ptr->sum[0] == '*') {
			xsum = 0;
		} else {
			xsum = misc->dec (ptr->sum, 2);
		}

		if (sum != xsum) {
			fprintf (stderr, "Oops! %d != %d for %d\n",
					sum, xsum, stdbuf[idx].sid);
		}

		stdbuf[idx].gradsum = sum;

		if (ptr->pass == '1') {
			stdbuf[idx].sflag |= STUDENT_FLAG_PASS1;
#if DEBUG_LEVEL > 1
			if (stdbuf[idx].sid == trace_sid) {
				fprintf (stderr, "pass\n");
			}
		} else if (stdbuf[idx].sid == trace_sid) {
			fprintf (stderr, "not pass\n");
#endif
		}
	}

	return idx;
}
#endif

#if ENABLE_CEEC_FORMAT == 1
static int read_ceec_exam_score_file_record (char *buffer) {
	struct ceec_exam_score	*ptr = (struct ceec_exam_score *) buffer;
	int			i, idx, sid;

#if ENABLE_CEEC_MERGE_SN == 1
	sid = misc->dec (ptr->serial, STUDENT_CEEC_SN_LENGTH);
#else
	sid = misc->dec (ptr->sid_exam, STUDENT_ID_LENGTH);
#endif

	if ((idx = search_stdsn_index (sid)) != -1) {
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_SCORE;
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_CEEC_EXAM;

		for (i = 0; i < 9; i++) {
			if (ptr->score[i][0] == '*') {
				stdbuf[idx].testmiss[course_order_patch[i] - 1]
							= 1;
			}

			stdbuf[idx].testscore[course_order_patch[i] - 1] =
					misc->score_conv (ptr->score[i]);
		}

		if ((i = stdbuf[idx].skill_group) != 0) {

			if (ptr->skill[i - 1][0] == '*') {
				stdbuf[idx].sflag |= STUDENT_FLAG_SKILL_MISS;
			} else if ((stdbuf[idx].sflag &
					STUDENT_FLAG_SKILL_MISS) != 0){
				stdbuf[idx].sflag &= ~STUDENT_FLAG_SKILL_MISS;
			}

			stdbuf[idx].skillscore = misc->score_conv (
							ptr->skill[i - 1]);

			stdbuf[idx].skillmajor_score =
						misc->score_conv (ptr->major);
		} else {
			stdbuf[idx].skillscore       = 0;
			stdbuf[idx].skillmajor_score = 0;
		}
	}

	return idx;
}
#endif

#if ENABLE_CEEC_FORMAT == 1
static int load_CEEC_basic_score (const char *file) {
	int	i, len, nof;
	char	*buffer;

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load CEEC basic score file [ %s ] ... ", file);

	// -------------------------------------------------------------
	
	if (tio->open (file)) {
		i = nof = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct ceec_grade_score)) continue;

			if (read_ceec_basic_score_file_record (buffer) != -1)
				nof++;

			i++;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_INFO, "%d read, %d use\r\n", i, nof);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);

		return 0;
	}

	// -------------------------------------------------------------

	// misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");
	return 1;
}
#endif

#if ENABLE_CEEC_FORMAT == 1
static int load_CEEC_exam_score (const char *file) {
	int	i, len, nof;
	char	*buffer;
	int	(*readfunc)(char *) = read_ceec_exam_score_file_record;
	size_t	minlen = sizeof (struct ceec_exam_score);

#if ENABLE_DISPATCHER92 == 1
	if (use_disp92) {
		readfunc = read_ceec_exam_score_file_record92;
		minlen = sizeof (struct ceec_exam_score92);
	}
#endif

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load CEEC%s exam score file [ %s ] ... ",
			use_disp92 ? "92" : "", file);

	// -------------------------------------------------------------
	
	if (tio->open (file)) {
		i = nof = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < minlen) continue;

			// fprintf (stderr, "%d\n", i);

			if (readfunc (buffer) != -1) nof++;

			i++;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_INFO, "%d read, %d use\r\n", i, nof);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);

		return 0;
	}

	// -------------------------------------------------------------
	
	// misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");
	return 1;
}
#endif

#if ENABLE_CEEC_FORMAT == 1
static int load_CEEC_format_score (const int additional) {
	int	i, *list, len;
	char	*bfile = NULL;
	char	*efile = NULL;
#if ENABLE_DISPATCHER92 == 1
	char	*sfile = NULL;
#endif

	if ((bfile = sysconfig->getstr ("student-basic-score-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable "
				"(student-basic-score-file) not define!!\r\n");
		return 0;
	}

	if ((efile = sysconfig->getstr ("student-exam-score-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable "
				"(student-exam-score-file) not define!!\r\n");
		return 0;
	}

#if ENABLE_DISPATCHER92 == 1
	if (use_disp92) {
		if ((sfile = sysconfig->getstr (
					"student-skill-score-file")) == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable "
				"(student-skill-score-file) not define!!\r\n");
			return 0;
		}
	}
#endif

	if ((list = sysconfig->intlist ("course-order-patch", &len)) != NULL) {
		if (len == 9) {
			for (i = 0; i < 9; i++) {
				course_order_patch[i] = list[i];
			}
		}
	}

	// ------------------------------------------------

	if (load_CEEC_basic_score (bfile) == 0) return 0;
	if (load_CEEC_exam_score  (efile) == 0) return 0;
#if ENABLE_DISPATCHER92 == 1
	if (use_disp92) {
		if (load_CEEC_skill_score  (sfile) == 0) return 0;
	}
#endif

	// ------------------------------------------------

	if (additional) {
		if ((bfile = sysconfig->getstr
				("student-basic-score-additional")) == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable "
				"(student-basic-score-additional)"
				" not define!!\r\n");
			return 0;
		}

		if ((efile = sysconfig->getstr
				("student-exam-score-additional")) == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable "
				"(student-exam-score-file)"
				" not define!!\r\n");
			return 0;
		}

		if (load_CEEC_basic_score (bfile) == 0) return 0;
		if (load_CEEC_exam_score  (efile) == 0) return 0;
	}

	return 1;
}
#endif

#if ENABLE_CHECKLIST == 1

static int init_checklist_report (const int level) {
	char	*filename;

	if (include_student_extend_info == 0) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Student extend info not activated!\r\n");
		return 0;
	}


	if ((filename = sysconfig->getstr ("checklist-report")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
		    "%% system variable (checklist-report) not define!!\r\n");
		return 0;
	} else {
		if (level > 1) {
			return pstck->open (filename);
		} else if ((cklfp = fopen (filename, "w")) == NULL) {
			misc->perror (PRINT_LEVEL_SYSTEM);
			return 0;
		}
	}
	return 1;
}

static void write_to_cklist (const int idx, const char *buffer) {
	int	j, k;

	if (cklfp == NULL) return;

	fprintf (cklfp, "%08d: SN=%5s,SEX=%d,PTYPE=%c,"
			"NAME=%s,DESC=%s\n"
			"%08d: C_SCORE=",
			stdbuf[idx].sid,
			stdbuf[idx].ck->check_sn,
			(stdbuf[idx].sflag & STUDENT_FLAG_GIRL) == 0 ? 1 : 2,
			stdbuf[idx].ptype,
			stdbuf[idx].info->name,
			buffer,
			stdbuf[idx].sid);

	for (j = 0; j < 5; j++) {
		fprintf (cklfp, "%2d,", stdbuf[idx].gradscore[j]);
	}

	fprintf (cklfp, "SUM=%d\n"
			"%08d: S_SCORE=",
			stdbuf[idx].gradsum, stdbuf[idx].sid);

	for (j = 0; j < 9; j++) {
		fprintf (cklfp, "%5d", stdbuf[idx].testscore[j]);
		if (j < 8) fprintf (cklfp, ",");
	}

	fprintf (cklfp, "\n%08d: "
			"SKILL=%d,%d,%04d,%04d,%d\n"
			"%08d: VOLUN=",
			stdbuf[idx].sid, stdbuf[idx].skill_group,
			stdbuf[idx].skillscore,
			stdbuf[idx].skillmajor, stdbuf[idx].skillminor,
			stdbuf[idx].skillmajor_score,
			stdbuf[idx].sid);

	for (k = MAX_PER_STUDENT_WISH-1; k >= 0; k--) {
		if (stdbuf[idx].wish[k] != 0) break;
	}

	fprintf (cklfp, "%2d", k+1);
	for (j = 0; j <= k; j++) {
		fprintf (cklfp, "," WISHID_FMT, stdbuf[idx].wish[j]);
	}
	fprintf (cklfp, "\n");
}

static int load_checklist_all (const int level) {
	int	i;

	if (! init_checklist_report (level)) return 0;

	for (i = 0; i < stdnum; i++) {
		stdbuf[i].ck = misc->malloc (sizeof (
					struct student_check_info_t));

		stdbuf[i].ck->matched_idx = -1;
		stdbuf[i].ck->new_matched_idx = -1;
		// stdbuf[idx].ck->check_sn = -1;
		// stdbuf[idx].ck->check_description = NULL;
		sprintf (stdbuf[i].ck->check_sn, "%06d", i + 1);
		write_to_cklist (i, stdbuf[i].ck->check_sn);
	}

	return 1;
}

static int load_checklist (const int level) {
	char			*filename;
	struct checklist_t	*ptr;
	char			buffer[256];
	int			sid, idx, len;

	if (! init_checklist_report (level)) return 0;

	if ((filename = sysconfig->getstr ("checklist-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
		    "%% system variable (checklist-file) not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load check-list file [ %s ] ... ",
			filename);

	if (tio->open (filename)) {
		int	i = 0;
		int	j; // k;
		int	err = 0;

		while ((len = tio->read ((char **) &ptr)) >= 0) {
			if (len < 13) continue;

			sid = misc->dec (ptr->studid, STUDENT_ID_LENGTH);

			if ((idx = search_sid_index (sid)) == -1) {
				int	i;

				for (i = 0; i < stdnum; i++) {
					if ((stdbuf[i].usedid[0] == sid) ||
				       		(stdbuf[i].usedid[1] == sid)) {
						idx = i;
						break;
					}
				}
			}

			if (idx != -1) {
				stdbuf[idx].ck = misc->malloc (
					sizeof (struct student_check_info_t));

				stdbuf[idx].ck->matched_idx = -1;
				stdbuf[idx].ck->new_matched_idx = -1;

				// stdbuf[idx].ck->check_sn = -1;
				// stdbuf[idx].ck->check_description = NULL;
				strncpy (stdbuf[idx].ck->check_sn,
						ptr->sn, 5);
				stdbuf[idx].ck->check_sn[5] = '\0';
//					stdbuf[idx].ck->check_sn =
//							misc->dec (ptr->sn, 6);

				strncpy (buffer, ptr->description,
						sizeof ptr->description);
				for (j = sizeof ptr->description - 1;
						j >= 0; j--) {
					if (buffer[j] != ' ') {
						break;
					}
				}

				buffer[j] = '\0';

				stdbuf[idx].ck->check_description =
					strdup (buffer);

				write_to_cklist (idx, buffer);
				/*
				fprintf (cklfp,
					"%08d: SN=%5s,SEX=%d,PTYPE=%c,"
					"NAME=%s,DESC=%s\n"
					"%08d: C_SCORE=",
					stdbuf[idx].sid,
					stdbuf[idx].ck->check_sn,
					(stdbuf[idx].sflag & STUDENT_FLAG_GIRL)
					== 0 ? 1 : 2,
					stdbuf[idx].ptype,
					stdbuf[idx].info->name,
					buffer,
					stdbuf[idx].sid);

				for (j = 0; j < 5; j++) {
					fprintf (cklfp, "%2d,",
						stdbuf[idx].gradscore[j]);
				}

				fprintf (cklfp, "SUM=%d\n"
						"%08d: S_SCORE=",
						stdbuf[idx].gradsum,
						stdbuf[idx].sid);

				for (j = 0; j < 9; j++) {
					fprintf (cklfp, "%5d",
						stdbuf[idx].testscore[j]);
					if (j < 8) fprintf (cklfp, ",");
				}

				fprintf (cklfp, "\n%08d: "
						"SKILL=%d,%d,%04d,%04d,%d\n"
						"%08d: VOLUN=",
						stdbuf[idx].sid,
						stdbuf[idx].skillgroup,
						stdbuf[idx].skillscore,
						stdbuf[idx].skillmajor,
						stdbuf[idx].skillminor,
						stdbuf[idx].skillmajor_score,
						stdbuf[idx].sid);

				for (k = MAX_PER_STUDENT_WISH-1; k >= 0; k--) {
					if (stdbuf[idx].wish[k] != 0) break;
				}

				fprintf (cklfp, "%2d", k+1);
				for (j = 0; j <= k; j++) {
					fprintf (cklfp, "," WISHID_FMT,
							stdbuf[idx].wish[j]);
				}
				fprintf (cklfp, "\n");
				*/
			} else {
				//misc->print (PRINT_LEVEL_SYSTEM,
				//		"Student [%d] not found\n",
				//		sid);
				strncpy (buffer, ptr->sn, 5); buffer[5] = '\0';

				if (cklfp != NULL) {
					fprintf (cklfp,
						"%08d: SN=%5s,NOT FOUND,DESC=",
						sid,
						buffer);
				}

				strncpy (buffer, ptr->description,
						sizeof ptr->description);
				buffer[sizeof ptr->description - 1] = '\0';
				if (cklfp != NULL) {
					fprintf (cklfp, "%s\n", buffer);
				}
				err++;
			}

			i++;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_SYSTEM, "%d read\r\n", i);

		//if (err) return 0;
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);

		return 0;
	}

	return 1;
}
#endif

static int load_score (const int additional) {
	int	i, j, retval;

	if (stdbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Please load wishes first\r\n");
		return 0;
	}

#if 0
	misc->timer_started ();

	misc->print (PRINT_LEVEL_SYSTEM,
			"Generating sorted index for students ... ");

	if (generate_sorted_student_index ()) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"ok.  (elapsed time: %.4f seconds)\r\n",
					misc->timer_ended());
	} else {
		misc->print (PRINT_LEVEL_SYSTEM, "failed\r\n");
		return 0;
	}
#endif

#if ENABLE_CEEC_FORMAT == 1
	if (use_ncu_format) {
		retval = load_NCU_format_score (additional);
	} else {
		retval = load_CEEC_format_score (additional);
	}
#else
	retval = load_NCU_format_score (additional);
#endif

	if (retval) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Check all student\'s score ... ");

		for (i = j = 0; i < stdnum; i++) {
			if ((stdbuf[i].sflag & STUDENT_FLAG_HAVE_SCORE) == 0) {
				j++;
#define PRINT_OUT_CLASS_SCORE	0
#if PRINT_OUT_CLASS_SCORE
			} else {
				int	k, m, n, sc;
				static const int course[4][7] = {
					{ 1, 2, 4, 5, 6, 0, 0 },
					{ 1, 2, 3, 7, 8, 0, 0 },
					{ 1, 2, 3, 7, 8, 9, 0 },
					{ 1, 2, 3, 8, 9, 0, 0 }
				};

				printf ("%d", stdbuf[i].sid);

				for (k = 0; k < 4; k++) {
					sc = 0;
					for (m = 0; (n = course[k][m]) != 0;
								m++) {
						if (stdbuf[i].testscore[n-1]
								< 0){
							sc = -1;
							break;
						}
						sc += stdbuf[i].testscore[n-1];
					}
					printf (",%d", sc);
				}
				printf ("\n");
#endif
			}
		}

		if (j == 0) {
			misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");
		} else {
			misc->print (PRINT_LEVEL_SYSTEM,
					"%d student(s) missing\r\n", j);
		}
	}

	return retval;
}

static void get_student_special_flag (const int idx, const char key) {
	int	tp, percent;

	if ((tp = sysconfig->get_special (key, &percent)) != 0) {
		if (tp == 1) {
			stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
		} else if (tp == 2) {
			stdbuf[idx].sflag |= STUDENT_FLAG_LOW;
		}

		stdbuf[idx].ratio = percent;
	} else {
		switch (key) {
			case '2':	// 降低錄取標準 - 退伍軍人 10%
				stdbuf[idx].sflag |= STUDENT_FLAG_LOW;
				stdbuf[idx].ratio = 10;
				break;
			case '3':	// 降低錄取標準 - 退伍軍人 25%
				stdbuf[idx].sflag |= STUDENT_FLAG_LOW;
				stdbuf[idx].ratio = 25;
				break;
			case '4':	// 降低錄取標準 - 山胞族籍 25%
				stdbuf[idx].sflag |= STUDENT_FLAG_LOW;
				stdbuf[idx].ratio = 25;
				break;
			case '5':	// 降低錄取標準 - 港澳生 10%
				stdbuf[idx].sflag |= STUDENT_FLAG_LOW;
				stdbuf[idx].ratio = 10;
				break;
			case 'A':	// 增加總分 - 退伍軍人 25%
				stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				stdbuf[idx].ratio = 25;
				break;
			case 'B':	// 增加總分 - 退伍軍人 8%
				stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				stdbuf[idx].ratio = 8;
				break;
			case 'C':	// 增加總分 - 蒙藏生 25%
				// 降低錄取標準 25%
				stdbuf[idx].sflag |= STUDENT_FLAG_LOW;
				stdbuf[idx].ratio = 25;
				//// 增加總分 25%
				// stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				// stdbuf[idx].ratio = 25;
				// break;
				break;
			case 'D':	// 增加總分 - 港澳生 10%
				stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				stdbuf[idx].ratio = 10;
				break;
			case 'E':	// 增加總分 - 僑生 20%
				stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				stdbuf[idx].ratio = 20;
				break;
			case 'W':	// 增加總分 - 派外子女 25%
				stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				stdbuf[idx].ratio = 25;
				break;
			case 'X':	// 增加總分 - 派外子女 20%
				stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				stdbuf[idx].ratio = 20;
				break;
			case 'Y':	// 增加總分 - 派外子女 15%
				stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				stdbuf[idx].ratio = 15;
				break;
			case 'Z':	// 增加總分 - 派外子女 10%
				stdbuf[idx].sflag |= STUDENT_FLAG_ADD;
				stdbuf[idx].ratio = 10;
				break;
			case '1':	// 普通生
			default:
				stdbuf[idx].ratio = 0;
				break;
		}
	}
}

static int read_student_info_file_record (char *buffer) {
	struct ncu_stdinfo	*ptr = (struct ncu_stdinfo *) buffer;
	int			sid, idx;

	sid = misc->dec (ptr->studid, STUDENT_ID_LENGTH);
	// i++;

	if ((idx = search_sid_index (sid)) != -1) {
		if ((stdbuf[idx].sflag & STUDENT_FLAG_HAVE_INFO) != 0) {
			fprintf (stderr, "Oops!! duplicat student wish\n");
		}

		// nof++;
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_INFO;

		if (ptr->sex != '1') {
			// 不是男生的都算是女生
			stdbuf[idx].sflag |= STUDENT_FLAG_GIRL;
		}

		get_student_special_flag (idx, ptr->ptype);

		/*
		//      是否違規 
		if (buffer[INFOFILE_START_AGAINST] == 'Y') {
			stdbuf[idx].sflag |= STUDENT_FLAG_AGAINST;
		}
		*/

		/* 是否符合登記 */
		if (ptr->disallow == 'Y') {
			stdbuf[idx].sflag |= STUDENT_FLAG_DISALLOW;
		}

#if ENABLE_CHECKLIST == 1
		stdbuf[idx].ck = NULL;
#endif
		stdbuf[idx].ptype = ptr->ptype;

		if (include_student_extend_info) {
			struct student_extend_info_t		*info1;
			struct student_extend_info_level2_t	*info2;

			if (have_level_2_info) {
				info2 = &stdinfo2[idx];
				info1 = (void *) info2;
			} else {
				info1 = &stdinfo[idx];
				info2 = (void *) info1;
			}
			stdbuf[idx].info = info1;

			strncpy (info1->name, ptr->name, MAX_STUDENT_LEN * 2);
			info1->birthday = misc->birthday (ptr->birth);

			if (have_level_2_info) {
				/*
				memcpy (info2->regist_sn,
						ptr->regist_sn,
						sizeof info2->regist_sn);
				memcpy (info2->address,
						ptr->address,
						sizeof info2->address);
				memcpy (info2->cmaddr,
						ptr->cmaddr,
						sizeof info2->cmaddr);
				memcpy (info2->edu_level,
						ptr->edu_level,
						sizeof info2->edu_level);
				memcpy (info2->tel_no,
						ptr->tel_no,
						sizeof info2->tel_no);
				memcpy (info2->zip_code,
						ptr->zip_code,
						sizeof info2->zip_code);
				memcpy (info2->edu_year,
						ptr->edu_year,
						sizeof info2->edu_year);
				memcpy (info2->guard_man,
						ptr->guard_man,
						sizeof info2->guard_man);
				*/
			}

			// stdbuf[idx].info->check_sn = -1;
			// stdbuf[idx].info->check_description = NULL;

#if DEBUG_LEVEL > 0
			if (stdbuf[idx].sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
					"Student: %d, Name = %s, Gender=%s\r\n",
					stdbuf[idx].sid,
					stdbuf[idx].info->name,
					(stdbuf[idx].sflag & STUDENT_FLAG_GIRL)
					!= 0 ? "女" : "男");
			}
#endif
		}
	}

	return idx;
}

static int load_NCU_format_student_info (const int additional) {
	char	*filename;
	char	*buffer;
	int	i, nof, len;

	if (stdbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Please load wishes first\r\n");
		return 0;
	}


	if ((filename = sysconfig->getstr ("student-info-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
		    "%% system variable (student-info-file) not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load NCU format info file [ %s ] ... ", filename);

	if (include_student_extend_info) {
		// int	infosize = sizeof (struct student_t);
		int	infosize = sizeof (struct student_extend_info_t);

		if (have_level_2_info) {
			infosize = sizeof (struct student_extend_info_level2_t);
		}

		// allocat space for student extend info
		if ((stdinfo = misc->calloc (stdnum, infosize)) == NULL) {
			misc->perror (PRINT_LEVEL_SYSTEM);
			return 0;
		}

		stdinfo2 = (void *) stdinfo;
	}

	if (tio->open (filename)) {
		i = nof = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct ncu_stdinfo)) continue;

			if (read_student_info_file_record (buffer) != -1)
				nof++;
			i++;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_INFO, "%d read, %d use\r\n", i, nof);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);

		return 0;
	}


#if DEBUG_LEVEL > 0
	if (additional && ((filename = sysconfig->getstr (
				"student-info-additional")) != NULL)) {
		misc->print (PRINT_LEVEL_INFO,
			"Load additional student info file [ %s ] ... ",
			filename);

		if (tio->open (filename)) {
			i = nof = 0;

			while ((len = tio->read (&buffer)) >= 0) {
				if (len < sizeof (struct ncu_stdinfo)) continue;

				if (read_student_info_file_record (
							buffer) != -1) nof++;
				i++;
			}
			tio->close ();

			misc->print (PRINT_LEVEL_INFO,
					"%d read, %d use\r\n", i, nof);
		} else {
			misc->perror (PRINT_LEVEL_INFO);
		}
	}
#endif


	return 1;
}

#if ENABLE_CEEC_FORMAT == 1

static int read_ceec_student_info_file_record (char *buffer) {
	struct ceec_stdinfo	*ptr = (struct ceec_stdinfo *) buffer;
	int			sid, idx;
	char			*ceec_idpos[4];
	int			i;
	int			skstf = 0;

	if (sizeof ptr->skillmajor == 4) {
		skstf = 0;
	} else if (sizeof ptr->skillmajor == 5) {
		skstf = 1;
	}

	ceec_idpos[0] = ptr->sid_exam;
	ceec_idpos[1] = ptr->sid_basic;
	ceec_idpos[2] = ptr->sid_mend;
	ceec_idpos[3] = NULL;

	for (i = 0, idx = -1; ceec_idpos[i] != NULL; i++) {
		sid = misc->dec (ceec_idpos[i], STUDENT_ID_LENGTH);
		if ((idx = search_sid_index (sid)) != -1) {
			break;
		}
	}

	if (idx >= 0) {
		if ((stdbuf[idx].sflag & STUDENT_FLAG_HAVE_INFO) != 0) {
			fprintf (stderr, "Oops!! duplicat student wish\n");
		}

		// stdbuf[idx].sflag = 0;
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_INFO;

#if ENABLE_CEEC_MERGE_SN == 1
		stdbuf[idx].sn     = misc->dec (
					ptr->serial, sizeof ptr->serial);
#else
#  if TRY_CEEC_MERGE_SN == 1
		stdbuf[idx].serial = misc->dec (
					ptr->serial, sizeof ptr->serial);
#  endif
		stdbuf[idx].usedid[0] =
				misc->dec (ptr->sid_exam,  STUDENT_ID_LENGTH);
		stdbuf[idx].usedid[1] =
				misc->dec (ptr->sid_basic, STUDENT_ID_LENGTH);
		stdbuf[idx].usedid[2] =
				misc->dec (ptr->sid_mend,  STUDENT_ID_LENGTH);
#endif

		if (ptr->sex != '1') {
			// 不是男生的都算是女生
			stdbuf[idx].sflag |= STUDENT_FLAG_GIRL;
		}

		// 注意!! 這個欄位其實不是大考原應有的 ...
		get_student_special_flag (idx, ptr->ptype);

		stdbuf[idx].ptype = ptr->ptype;

		stdbuf[idx].highsch = misc->dec (
				ptr->edu_level, sizeof ptr->edu_level);

		if ((i = misc->dec (ptr->skillgroup, 1)) != 0) {
			stdbuf[idx].skill_group = i;
			stdbuf[idx].skillmajor = misc->dec (
						&ptr->skillmajor[skstf], 4);
			stdbuf[idx].skillminor = misc->dec (
						&ptr->skillminor[skstf], 4);
		} else {
			stdbuf[idx].skill_group = 0;
			stdbuf[idx].skillmajor = 0;
			stdbuf[idx].skillminor = 0;
		}

		if (ptr->disallow == 'N') {
			stdbuf[idx].sflag |= STUDENT_FLAG_DISALLOW;
		}

		switch (misc->dec (&ptr->special, 1)) {
		case 2: // 國防
			// dprint ("Student: %d DF\n", stdbuf[idx].sid);
			stdbuf[idx].sflag |= STUDENT_FLAG_DF_ALLOW;
			break;
		case 3: // 師範
			// dprint ("Student: %d nou only\n", stdbuf[idx].sid);
			stdbuf[idx].sflag |= STUDENT_FLAG_NU_ONLY;
			break;
		case 1:
		default:
			break;
		}

		if (include_student_extend_info) {
			strncpy (stdinfo[idx].name,
				ptr->name, MAX_STUDENT_LEN * 2);
			stdbuf[idx].info = &stdinfo[idx];

			if (have_level_2_info) {
			}
#if DEBUG_LEVEL > 0
			if (stdbuf[idx].sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
					"Student: %d, Name = %s, Gender=%s\r\n",
					stdbuf[idx].sid,
					stdbuf[idx].info->name,
					(stdbuf[idx].sflag & STUDENT_FLAG_GIRL)
					!= 0 ? "女" : "男");
			}
#endif
		}
	}

	return idx;
}
#endif

#if ENABLE_CEEC_FORMAT == 1
static int load_CEEC_format_student_info (void) {
	char	*filename;
	char	*buffer;
	int	i, nof, len;
	int	(*readfunc)(char *) = read_ceec_student_info_file_record;
	int	minlen = sizeof (struct ceec_stdinfo);

#if ENABLE_DISPATCHER92 == 1
	if (use_disp92) {
		readfunc = read_ceec_student_info_file_record92;
		minlen   = sizeof (struct ceec_stdinfo92);
	}
#endif

	if (stdbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Please load wishes first\r\n");
		return 0;
	}


	if ((filename = sysconfig->getstr ("student-info-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
		    "%% system variable (student-info-file) not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load CEEC%s info file [ %s ] ... ",
			use_disp92 ? "92" : "", filename);

	if (include_student_extend_info) {
		int	infosize = sizeof (struct student_extend_info_t);

		if (have_level_2_info) {
			infosize = sizeof (struct student_extend_info_level2_t);
		}
		// allocat space for student extend info
		if ((stdinfo = misc->calloc (stdnum, infosize)) == NULL) {
			misc->perror (PRINT_LEVEL_SYSTEM);
			return 0;
		} else {
			misc->print (PRINT_LEVEL_SYSTEM, "[%d]",
					infosize);
		}

		stdinfo2 = (void *) stdinfo;
	}

	if (tio->open (filename)) {
		i = nof = 0;

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < minlen) continue;

			if (readfunc (buffer) != -1) nof++;
			i++;
		}
		tio->close ();

		misc->print (PRINT_LEVEL_INFO, "%d read, %d use\r\n", i, nof);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);

		return 0;
	}

	generate_sorted_student_sn_index ();

	return 1;
}
#endif

static int load_info (const int additional) {
	int	retval = 0;
	int	i, nof;

#if ENABLE_CEEC_FORMAT == 1
	if (use_ncu_format) {
		retval = load_NCU_format_student_info (additional);
	} else {
		retval = load_CEEC_format_student_info ();
	}
#else
	retval = load_NCU_format_student_info (additional);
#endif

	if (retval) {
		misc->print (PRINT_LEVEL_INFO,
				"Check all student\'s info ... ");

		for (i = nof = 0; i < stdnum; i++) {
			if ((stdbuf[i].sflag & STUDENT_FLAG_HAVE_INFO) == 0) {
				nof++;
				misc->print (PRINT_LEVEL_DEBUG,
						"student: %d\n",
						stdbuf[i].sid);
			}
		}

		if (nof == 0) {
			misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");
		} else {
			misc->print (PRINT_LEVEL_SYSTEM,
				"%d student(s) missing or duplicate\r\n", nof);
			return 0;
		}
	}

	return retval;
}

#if ENABLE_CEEC_FORMAT == 1
static int generate_sorted_student_sn_index (void) {
#  if ENABLE_CEEC_MERGE_SN == 1
	int	i;

	if ((stdsnsortedindex != NULL) || (stdbuf == NULL)) return 0;
	if ((stdsnsortedindex = misc->malloc (stdnum *
			sizeof (struct student_indexes_t))) == NULL) return 0;

	for (i = 0; i < stdnum; i++) {
		stdsnsortedindex[i].idx = i;
		stdsnsortedindex[i].sid = stdbuf[i].sn;
	}

	qsort (stdsnsortedindex, stdnum,
			sizeof (struct student_indexes_t),
			(int (*)(const void *, const void *)) order_by_sid);

	return 1;
#  else
	int	i, j;

	for (i = 0; i < 3; i++) {
		if ((stdidsortedindex[i] = misc->malloc (stdnum *
				sizeof (struct student_indexes_t))) == NULL) {
			return 0;
		}

		for (j = 0; j < stdnum; j++) {
			stdidsortedindex[i][j].idx = j;
			stdidsortedindex[i][j].sid = stdbuf[j].usedid[i];
		}

		qsort (stdidsortedindex[i], stdnum,
			sizeof (struct student_indexes_t),
			(int (*)(const void *, const void *)) order_by_sid);
	}

	return 1;
#  endif
}
#endif

static int generate_sorted_student_index (void) {
	int	i;

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

	/*
	for (i = 0; i < stdnum; i++) {
		misc->print (PRINT_LEVEL_SYSTEM, "[ %5d, %10d ]\r\n",
				stdsortedindex[i].idx,
				stdsortedindex[i].sid);
	}
	*/


	return 1;
}

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

static struct student_t * find_by_id (const int32_t sid) {
	int	idx;

	if ((idx = search_sid_index (sid)) != -1) {
		return &stdbuf[idx];
	} else {
		return NULL;
	}
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


#if ENABLE_CEEC_FORMAT == 1
static int search_stdsn_index (const int32_t sid) {
	struct student_indexes_t	*ptr = NULL;
	struct student_indexes_t	key;
#  if ENABLE_CEEC_MERGE_SN != 1
	int				i;
#  endif

#  if ENABLE_CEEC_MERGE_SN == 1
	if (stdsnsortedindex == NULL) return -1;
#  endif


	key.sid = sid;

#  if ENABLE_CEEC_MERGE_SN == 1
	ptr = bsearch (&key, stdsnsortedindex, stdnum,
			sizeof (struct student_indexes_t),
			(int (*)(const void *, const void *)) order_by_sid);
#  else
	for (i = 0; i < 3; i++) {
		if (stdidsortedindex[i] == NULL) continue;

		ptr = bsearch (&key, stdidsortedindex[i], stdnum,
			sizeof (struct student_indexes_t),
			(int (*)(const void *, const void *)) order_by_sid);

		if (ptr != NULL) break;
	}
#  endif

	if (ptr == NULL) return -1;

	return ptr->idx;
}
#endif

#if ENABLE_DEPARTMENT_FIRST == 1

static void set_wishidx (struct student_t *st, const int nw, const int idx) {
	st->wishidx[nw] = idx;
}

static void set_valid_wish_mask (struct student_t *st, const short *mask) {
	int	i;

	for (i = 0; i < MAX_PER_STUDENT_WISH; i++) {
		if (mask[i] == 0) {
			st->wish[i] = - 2 - st->wish[i];
		}
	}
}

#endif

static int st_push (struct student_t *st) {
	if (stktop > 0) {
		stdstack[--stktop] = st;
		return 1;
	}

	return 0;
}

static struct student_t * st_pop (void) {
	if (stktop < stkcount) return stdstack[stktop++];

	return NULL;
}

static int16_t nextwish	(struct student_t *st) {
	if (st == NULL) return -1;
	if (st->wish_index >= NUMBER_OF_WISHES) {
		// make wish_index larger than NUMBER_OF_WISHES
		// so that it can be checked for student dosen't accept
		// by any school
		st->wish_index = NUMBER_OF_WISHES + 1;
		return -1;
	}

	return st->wish[(int) st->wish_index++];
}

static int16_t nextwish_with_order (struct student_t *st, int *order) {
	*order = (int) st->wish_index;

	return nextwish (st);
}

static int number_of_students (void) {
	return stdnum;
}

static int get_wish_index (struct student_t *st) {
	return st->wish_index;
}

static void reset_wish (struct student_t *st) {
	if (st == NULL) return;

	st->wish_index = 0;
}

static void free_stack (void) {
	if (stdstack != NULL) {
		// 清掉舊的, 讓 stack 有機會重新 construct
		stkcount = stktop = 0;
		free (stdstack);
		stdstack = NULL;
	}
}

static int order_by_score (const student_ptr_t *i, const student_ptr_t *j) {
	if ((*i)->testscore[1] < (*j)->testscore[1]) {
		return 1;
	} else if ((*i)->testscore[1] > (*j)->testscore[1]) {
		return -1;
	} else {
		return 0;
	}
}

static int order_by_checksn (const student_ptr_t *i, const student_ptr_t *j) {
	if ((*i)->ck != NULL) {
		if ((*j)->ck != NULL) {
			return memcmp ((*i)->ck->check_sn,
					(*j)->ck->check_sn,
					sizeof ((*i)->ck->check_sn) - 1);
		} else {
			return -1;
		}
	} else {
		if ((*j)->ck != NULL) {
			return 1;
		} else {
			return order_by_score (i, j);
		}
	}
}

static int prepare_sorted_stack	(const int sorted) {
	int	i;

	// free_stack ();

	if (stdstack == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"Allocate space for student stack ... ");

		if ((stdstack = misc->malloc (stdnum *
					sizeof (student_ptr_t))) == NULL) {
			misc->perror (PRINT_LEVEL_SYSTEM);
			return 0;
		}
	} else {
		misc->print (PRINT_LEVEL_SYSTEM,
			"Re-allocate space for student stack ... ");
	}
	stkcount = stktop = 0;


	// -----------------------------------

	misc->timer_started ();

	stkcount = stdnum;
	stktop = 0;

	for (i = 0; i < stdnum; i++) stdstack[i] = &stdbuf[i];

	if (sorted == 1) {
		// 其實是希望對成績先 sort 過, 讓這種近似 bubble sort
		// 的演算法得到最好的效能
		qsort (stdstack, stkcount,
			sizeof (student_ptr_t),
			(int (*)(const void *, const void *)) order_by_score);
		misc->print (PRINT_LEVEL_SYSTEM, "(sort) ok");
	} else if (sorted == 2) {
		qsort (stdstack, stkcount,
			sizeof (student_ptr_t),
			(int (*)(const void *, const void *)) order_by_checksn);
		misc->print (PRINT_LEVEL_SYSTEM, "[sort] ok");
	} else {
		misc->print (PRINT_LEVEL_SYSTEM, "ok");
	}

	misc->print (PRINT_LEVEL_SYSTEM, " [ %.4f second(s) ]\r\n",
			misc->timer_ended ());


	return 1;
}

static void print_all (FILE *fp) {
	int	i, j;
	int	wishid, score;

	for (i = 0; i < stdnum; i++) {
		if ((j = stdbuf[i].wish_index) <= NUMBER_OF_WISHES) {
			wishid = stdbuf[i].wish[j-1];
			score  = stdbuf[i].final_score;
		} else {
			wishid = 0;
			score  = 0;
			j      = 0;
		}

		fprintf (fp, WISHID_FMT ", %d, %3d, %6d\n",
				wishid, stdbuf[i].sid,
				j, score);
	}
}

#ifdef COUNT_GRADE_STANDARD_SCORE

static int standard_compare (const int *x, const int *y) {
	if (*x > *y) {
		return -1;
	} else if (*x < *y) {
		return 1;
	} else {
		return 0;
	}
}

static void calculate_standard_score (void) {
	int			i, j, k, m, n;
	int			*score;
	unsigned long long	total, t2;
	double			stand[5];

	if ((score = malloc (sizeof (int) * stdnum)) != NULL) {
		for (i = 0; i < 5; i++) {
			total = 0L;
			for (j = k = 0; j < stdnum; j++) {
				score[j] = stdbuf[j].gradscore[i];

				if (score[j] >= 0) {
					total += score[j];
					k++;
				}
			}

			stand[2] = (double) total / (double) k;

			qsort (score, stdnum, sizeof (int),
					(int (*)(const void*, const void*))
					standard_compare);

			total = 0L;
			for (j = m = 0; j < k / 4; j++, m++) {
				total += score[j];
			}

			stand[0] = (double) total / (double) m;

			for (; j < k / 2; j++, m++) {
				total += score[j];
			}

			stand[1] = (double) total / (double) m;

			total = t2 = 0;
			for (m = 0; j < k * 3 / 4; j++, m++) {
				total += score[j];
			}

			for (n = 0; j < k; j++, m++, j++, n++) {
				total += score[j];
				t2    += score[j];
			}

			stand[3] = (double) total / (double) m;
			stand[4] = (double) t2 / (double) n;


			fprintf (stderr, "Score[%d]:", i + 1);

			for (k = 0; k < 5; k++) {
				fprintf (stderr, " %5.2f", stand[k]);
			}

			fprintf (stderr, "\n");
		}

		free (score);
	}
}
#endif

#if ENABLE_DENYLIST == 1

static struct deny_list_t {
	int32_t		sid;
	int16_t		did;
} *denylist = NULL;

static int denylist_len = 0;

static int cmp_denylist (struct deny_list_t *dlst1, struct deny_list_t *dlst2) {
	if (dlst1->sid < dlst2->sid) {
		return 1;
	} else if (dlst1->sid > dlst2->sid) {
		return -1;
	} else {
		if (dlst1->did < dlst2->did) {
			return 1;
		} else if (dlst1->did > dlst2->did) {
			return -1;
		}
	}

	return 0;
}

static int load_deny_list (void) {
	char		*filename;
	int		i, len;
	int		sid, did;
	unsigned long	count = 0L;
	char		*buffer;

	if ((filename = sysconfig->getstr ("deny-list-file")) == NULL) {
		misc->print (PRINT_LEVEL_INFO,
		    "%% system variable (deny-list-file) not define!!\r\n");
		return 0;
	} else if (filename[0] == '\0') {
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "Load deny list file [ %s ] ... ",
			filename);

	if (tio->open (filename)) {
		// 計算出學生筆數, Allocate Space for students
		count = 0;
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < STUDENT_ID_LENGTH) continue;
			if ((buffer[0] >= '0') || (buffer[0] <= '9')) count++;
		}
		tio->close ();

		if (count == 0) {
			misc->print (PRINT_LEVEL_SYSTEM,
					"empty list (good)\r\n");
			return 0;
		}
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	// allocate memory
	if ((denylist = misc->calloc (count,
					sizeof (struct deny_list_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	} else {
		denylist_len = count;
	}

	if (tio->open (filename)) {
		i = 0;
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < STUDENT_ID_LENGTH) continue;
			if ((buffer[0] < '0') || (buffer[0] > '9')) continue;

			sid = misc->dec (&buffer[0], STUDENT_ID_LENGTH);
			if (len >= STUDENT_ID_LENGTH+DEPARTMENT_ID_LENGTH+1) {
				did = misc->oct (
						&buffer[STUDENT_ID_LENGTH+1],
						DEPARTMENT_ID_LENGTH);
			} else {
				did = 0;
			}
			denylist[i].sid = sid;
			denylist[i].did = did;

			// fprintf (stderr, "[%d][%04o]", sid, did);
			i++;
		}
		tio->close ();
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	if (denylist_len > 0) {
		qsort (denylist, denylist_len,
			sizeof (struct deny_list_t),
			(int (*)(const void *, const void *)) cmp_denylist);
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%d read\r\n", denylist_len);
	return 1;
}

static int is_wish_deny (const int32_t sid, const int16_t did) {
	struct deny_list_t	dl;
	struct eeny_list_t	*ptr;

	if (denylist == NULL) return 0;

	dl.sid = sid;
	dl.did = did;

	ptr = bsearch (&dl, denylist, denylist_len,
			sizeof (struct deny_list_t),
			(int (*)(const void *, const void *)) cmp_denylist);

	if (ptr == NULL) return 0;

	return 1;
}
#endif
