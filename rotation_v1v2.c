/*
 *	rotation_v1.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#include "predefine.h"
#include "department.h"
#include "student.h"
#include "rotation.h"
#include "misclib.h"
#include "global_var.h"

#if ENABLE_DEPARTMENT_FIRST == 1

//	rotation version 1
//
//	這個演算法是不撕殺, 而且以沒有增額錄取或不足額錄取
//	的學系, 凡是碰到增額錄取或不足額錄取的就當成 sink
//	還有那些 Best admirer 的是個落榜生時, 一樣當成 sink
//	當成 sink 的就沒有必要一直重覆參與 rotation 檢查,
//	That's all ...


struct department_flag {
	int8_t			dflag;
};

static struct department_flag			*depflag;
static int					rotation_version = 0;

#define DFLAG_IN_SINK		 1
#define DFLAG_OVER		 2
#define DFLAG_UNDER		 4
#define	DFLAG_HAVE_DONE		 8
#define DFLAG_NO_BEST_ADMIRER	16
#define DFLAG_NOT_UNIQ_ADMIRER	32

// --------------------------------------------------------------------

static struct department_t			*depbuf;
static int					depnum;
static struct department_internal_data_t	*didt = NULL;


static int init (struct department_internal_data_t *ptr) {
	if (didt == NULL) {
		didt = ptr;

		depbuf = didt->depbuf;
		depnum = didt->depnum;

		// misc->print (PRINT_LEVEL_INFO,
		//		"Regist rotation algorithm version 1 ... ");

		depflag = misc->malloc (
				depnum * sizeof (struct department_flag));

		if (depflag == NULL) {
			misc->perror (PRINT_LEVEL_INFO);
			return 0;
		}

		// misc->print (PRINT_LEVEL_INFO, "ok\r\n");

	}
	return 1;
}

static int prepare (void) {
	int	i, j, n, k;


	for (i = 0; i < depnum; i++) {
		depflag[i].dflag = 0;

		if (depbuf[i].num < depbuf[i].real_num) {
			// 增額錄取
			depflag[i].dflag |= DFLAG_IN_SINK;
			depflag[i].dflag |= DFLAG_OVER;
		} else if (depbuf[i].num < depbuf[i].real_num) {
			// 不足額錄取
			depflag[i].dflag |= DFLAG_IN_SINK;
			depflag[i].dflag |= DFLAG_UNDER;
		} else {
			n = 0;

			if ((j = didt->first_admirer (i, -1, NULL)) >= 0) {
				for (n = 1; (k = didt->next_admirer (
						i, j, NULL)) != -1; j = k) {
					n++;
				}
			}

			// Best Admirer 並不唯一
			if (n != 1) {
				depflag[i].dflag |= DFLAG_NOT_UNIQ_ADMIRER;
			}
		}
	}
	return 1;
}


static int number_of_shift;

static int find_a_loop (const int i, const int level, int *rescue) {
	static int	maxlevel;
	static int	inloop;
	int		j, k,  x, y;
	int		retval;
	int		rejno;

	maxlevel = level;
	inloop = rejno = 0;

	if ((depflag[i].dflag & DFLAG_IN_SINK) != 0) return -1;

	if (rotation_version == 1) {
		// 第一版的不處理多個 Best Admirer
		if ((depflag[i].dflag & DFLAG_NOT_UNIQ_ADMIRER) != 0) return -1;
	}

	if ((depflag[i].dflag & DFLAG_HAVE_DONE) != 0) {
		// 射回去了
		inloop = 1;
		return i;
	}


	if ((j = didt->first_admirer (i, -1, &rejno)) < 0) {
		// 沒有 Best Admirer
		depflag[i].dflag |= DFLAG_IN_SINK;
		depflag[i].dflag |= DFLAG_NO_BEST_ADMIRER;
		return -1;
	}

	x = depbuf[i].stdlist[j].st->matched_wish - 1;
	y = depbuf[i].stdlist[j].st->wishidx[x];

	depflag[i].dflag |= DFLAG_HAVE_DONE;

	retval = find_a_loop (y, level + 1, rescue);

	depflag[i].dflag &= (~DFLAG_HAVE_DONE);

	if (retval == -1) {
		// 看要不要再試 next admirer
		
		while ((j = didt->next_admirer (i, j, &rejno)) != -1) {
#if DEBUG_LEVEL > 1
			if (trace_wishid == depbuf[i].id) {
				misc->print (PRINT_LEVEL_DEBUG,
					"Try next admirer %d on "
						WISHID_FMT "\r\n",
						depbuf[i].stdlist[j].st->sid,
						depbuf[i].id);
			}
#endif
			x = depbuf[i].stdlist[j].st->matched_wish - 1;
			y = depbuf[i].stdlist[j].st->wishidx[x];

			depflag[i].dflag |= DFLAG_HAVE_DONE;

			retval = find_a_loop (y, level + 1, rescue);

			depflag[i].dflag &= (~DFLAG_HAVE_DONE);

			if (retval != -1) {
#if DEBUG_LEVEL > 1
				if (debug_flag) {
					misc->print (PRINT_LEVEL_DEBUG,
						"Found loop from next "
						"best admirer on " WISHID_FMT
						" (student=%d)\r\n",
						depbuf[i].id,
						depbuf[i].stdlist[j].st->sid);
				}
#endif
				break;
			}
		}

		if (retval == -1) return -1;
	}


	if (inloop) {
		for (k = j; (k = didt->next_admirer (i, k, &rejno)) != -1;) {
			// 目前這段程式碼的用處是計算 rescue 數
			//
			// 但實際上, 這裡隱含了另外 loop 的串列
		}

		(*rescue) += rejno;
#if DEBUG_LEVEL > 0
		if (verbose > 1) {
			misc->print (PRINT_LEVEL_DEBUG,
				"(" WISHID_FMT ",%d)",
				depbuf[i].id, 
				depbuf[i].stdlist[j].st->sid);
		}
#endif
		if (retval == i) { // start from me
#if DEBUG_LEVEL > 0
			if (verbose > 1) {
#  if DEBUG_LEVEL > 3
				misc->print (PRINT_LEVEL_DEBUG,
						"[%d,%d]\r\n", i, j);
				misc->print (PRINT_LEVEL_DEBUG,
					"On " WISHID_FMT
					", Start=%d, matched=%d, on "
					WISHID_FMT "\r\n",
					depbuf[i].id,
					depbuf[i].stdlist[j].st->sid,
					depbuf[i].stdlist[j].st->matched_wish,
					depbuf[y].id);
#   else
				misc->print (PRINT_LEVEL_DEBUG, "\r\n");
#   endif
			}
#endif
			didt->invalid_rest_wishes (&depbuf[i].stdlist[j]);
			number_of_shift = maxlevel - level;
			inloop = 0;
		}
	}

	return retval;
}

static int try_rotation (int *rotation, int *shift, int *rescue) {
	int	i, j;
	int	have_best_admirer;

	*rotation = 0;
	*shift    = 0;
	*rescue   = 0;

#if 0
	for (i = 0; i < depnum; i++) {
		if ((depflag[i].dflag & DFLAG_IN_SINK) == 0) {
			depflag[i].dflag = 0;
		}
	}
#endif

	do {
		for (i = j = have_best_admirer = 0; i < depnum; i++) {
			if ((depflag[i].dflag & DFLAG_IN_SINK) != 0) continue;
			if ((depflag[i].dflag & DFLAG_HAVE_DONE) != 0) continue;

			if ((j = find_a_loop (i, 0, rescue)) > 0) {
				(*rotation)++;
				(*shift) += number_of_shift;
			}

			while (didt->do_require_fix ());
		}
	} while (have_best_admirer);

#if DEBUG_LEVEL > 1
	if (*rotation > 0) {
		misc->print (PRINT_LEVEL_DEBUG,
		    "Rotation version %d: %d rotate, %d shift, %d rescue\r\n",
		    rotation_version, *rotation, *shift, *rescue);
	}
#endif

	return *rotation;
}

#if ENABLE_DR_LEE_SPECIAL == 1
int finding_reject_possible (void) {
	int	i, j;
	int	n;
	int	rescue;

	prepare ();

	for (i = j = n = 0; i < depnum; i++) {
		if ((depflag[i].dflag & DFLAG_IN_SINK) != 0) continue;
		if ((depflag[i].dflag & DFLAG_HAVE_DONE) != 0) continue;

		dr_lee_rjstcnt = 0;

		if ((j = find_a_loop (i, 0, &rescue)) > 0) {
			n++;
			// fprintf (stderr, "Loop found on %d\n", i);
			fprintf (stderr, "Dr'Lee Student [%d on " WISHID_FMT
					"]\n", dr_lee_studid, dr_lee_wishid);
		} else {
			// fprintf (stderr, "Loop not found on %d\n", i);
		}
	}

	return n;
}
#endif

static int try_rotation_v1 (int *rotation, int *shift, int *rescue) {
	rotation_version = 1;

	return try_rotation (rotation, shift, rescue);
}

static int try_rotation_v2 (int *rotation, int *shift, int *rescue) {
	rotation_version = 2;

	return try_rotation (rotation, shift, rescue);
}


void regist_rotation_algorithm_version_1 (void) {
	static struct rotation_algorithm_t	rtam;

	rtam.init         = init;
	rtam.try_rotation = try_rotation_v1;
	rtam.prepare	  = prepare;

	regist_rotation_algorithm (&rtam);
}

void regist_rotation_algorithm_version_2 (void) {
	static struct rotation_algorithm_t	rtam;

	rtam.init         = init;
	rtam.try_rotation = try_rotation_v2;
	rtam.prepare	  = prepare;

	regist_rotation_algorithm (&rtam);
}

#endif
