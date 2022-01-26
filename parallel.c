/*
 *	parallel.c
 *
 *	Copyright (c) 2003, Jiann-Ching Liu
 *
 *	Algorithm: A Parallel Algorithm to Solve The Stable
 *                 Marriage Problem
 */

#include "predefine.h"

#if ENABLE_PARALLEL_ALGORITHM == 1

#include <stdio.h>

#include "global_var.h"
#include "misclib.h"
#include "student.h"
#include "department.h"
#include "dispunit.h"
#include "sys_conf.h"

#define MAX_SOLUTION_SET	30	/* log n */
#define EXTRA_STUDENT_FOR_EVEN	2

#define USE_PA_EXTERNAL_PRINT	0

struct solution_set_t {
	int	num;
	int	*stdlist;
};

static struct department_module_t	*dxp = NULL;

static struct student_t		*std = NULL;
static struct department_t	*dep = NULL;
static struct solution_set_t	*solset[MAX_SOLUTION_SET];
static int			*free_student = NULL;
static int			fstd_stk = 0;
static int			fstd_stksize = 0;
static int			extra_student_for_even = EXTRA_STUDENT_FOR_EVEN;
static short			solset_use[MAX_SOLUTION_SET];
static int			solset_idx = 0;
static int			stdnum = 0;
static int			depnum = 0;
static int			maxlevel = 0;

static int init_free_student_stack (const int num) {
	if ((free_student = misc->malloc (num * sizeof (int))) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM, "Out of memory!!\n");
		return 0;
	}

	fstd_stksize = num;
	fstd_stk = 0;

	return 1;
}

static void push_free_student (const int stdid) {
	if (fstd_stk < fstd_stksize) {
		// fprintf (stderr, "push(%d)\n", fstd_stk);
		free_student[fstd_stk++] = stdid;
	} else {
		misc->print (PRINT_LEVEL_SYSTEM, "Stack overflow!!\n");
	}
}

static int pop_free_student (void) {
	if (fstd_stk == 0) return -1;

	return free_student[--fstd_stk];
}

static int new_solution_set (void) {
	int			i;
	struct solution_set_t	*sol;

	for (i = 0; i < solset_idx; i++) {
		if (solset_use[i] == 0) {
			solset_use[i] = 1;
			return i;
		}
	}

	if (solset_idx < MAX_SOLUTION_SET) {
		if ((solset[solset_idx] = misc->malloc (depnum *
				sizeof (struct solution_set_t))) == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM, "Out of memory!!\n");
			return -1;
		}

		sol = solset[solset_idx];

		for (i = 0; i < depnum; i++) {
			if ((sol[i].stdlist = misc->malloc (
				(dep[i].real_num + extra_student_for_even) *
					sizeof (int))) == NULL) {
				misc->print (PRINT_LEVEL_SYSTEM,
						"Out of memory!!\n");
				return -1;
			}
			sol[i].num = 0;
		}

		// print_memory ();
		solset_use[solset_idx] = 1;
		return solset_idx++;
	} else {
		misc->print (PRINT_LEVEL_SYSTEM, "Too many Solution Set!!\n");
		return -1;
	}
}

static void free_solution_set (const int i) {
	solset_use[i] = 0;
}

static void clean_solution_set (const int idx) {
	struct solution_set_t	*sol = solset[idx];
	int			i;

	for (i = 0; i < depnum; i++) sol[i].num = 0;
}

static int insert_to_solution_set (struct solution_set_t *sol,
				const int sid, const int si, const int did) {
	int	i, j, k, so, jo, ji;
	int	lsid, xsid, swi, lo, xo;

	if ((so = std[sid].order_on_wish[si]) < 0) return 0;

	for (i = sol[did].num - 1; i >= 0; i--) {
		j  = sol[did].stdlist[i];
		ji = std[j].wish_index;
		jo = std[j].order_on_wish[ji];

		if (so >= jo) break;
	}

	++i;
	// if (i >= dep[did].real_num) return 0;
	if (i >= dep[did].real_num + extra_student_for_even) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Too many student have same score!!\n");
	}

	for (j = sol[did].num; j > i; j--) {
		sol[did].stdlist[j] = sol[did].stdlist[j - 1];
	}

	sol[did].stdlist[i] = sid;
	sol[did].num++;

	if (sol[did].num > dep[did].real_num) {
		lsid = sol[did].stdlist[dep[did].real_num - 1];
		swi  = std[lsid].wish_index; //  - 1;
		lo   = std[lsid].order_on_wish[swi];

		ji = dep[did].real_num;

		for (k = 1, j = dep[did].real_num; j < sol[did].num; j++) {
			xsid = sol[did].stdlist[j];
			swi  = std[xsid].wish_index;
			xo   = std[xsid].order_on_wish[swi];

			if (k) {

				if (dxp->compare_order (&dep[did].stdlist[lo],
						&dep[did].stdlist[xo]) == 0) {
					// fprintf (stderr, "Add one\n");
					ji = j + 1;
					continue;
				}

				k = 0;
			}

#if USE_PA_EXTERNAL_PRINT == 1
			dep[did].stdlist[xo].aflag |= SWF_INVALID;
#endif

			push_free_student (sol[did].stdlist[j]);
		}

		sol[did].num = ji;

#if USE_PA_EXTERNAL_PRINT == 1
		if (ji > 0) {
			xsid = sol[did].stdlist[ji - 1];
			swi  = std[xsid].wish_index;

			dep[i].llindex = std[xsid].order_on_wish[swi] + 1;
		}
#endif
	}

	return 1;
}

static int merge_solution_set (const int si, const int sj) {
	int			sid, swi, i, j, ii, jj, kk, oi, oj, sk, num;
	struct solution_set_t	*ssi, *ssj, *ssk;

	if ((si >= 0) && (sj < 0)) {
		return si;
	} else if ((si < 0) && (sj >= 0)) {
		return sj;
	} else if ((si < 0) && (sj < 0)) {
		return -1;
	}

	sk = new_solution_set ();
	clean_solution_set (sk);

	ssi = solset[si];
	ssj = solset[sj];
	ssk = solset[sk];

	// fprintf (stderr, "Merge loop\n");

	for (i = 0; i < depnum; i++) {
		num = dep[i].real_num;

		for (ii = jj = kk = 0; kk < num; kk++) {
			if (ii >= ssi[i].num) {
				if (jj >=  ssj[i].num) break;
	
				ssk[i].stdlist[kk] = ssj[i].stdlist[jj++];
				continue;
			} else if (jj >= ssj[i].num) {
				ssk[i].stdlist[kk] = ssi[i].stdlist[ii++];
				continue;
			}

			sid = ssi[i].stdlist[ii];
			swi = std[sid].wish_index; //  - 1;
			oi  = std[sid].order_on_wish[swi];

			sid = ssj[i].stdlist[jj];
			swi = std[sid].wish_index; //  - 1;
			oj  = std[sid].order_on_wish[swi];

			// if (ssi[i].stdlist[ii] <= ssj[i].stdlist[jj]) {
			if (oi <= oj) {
				ssk[i].stdlist[kk] = ssi[i].stdlist[ii++];
			} else if (oi > oj) {
				ssk[i].stdlist[kk] = ssj[i].stdlist[jj++];
			}
		}

		if (kk == num) {
			// 考慮增額錄取的可能性

			sid = ssk[i].stdlist[kk - 1];
			swi = std[sid].wish_index;
			oi  = std[sid].order_on_wish[swi];

			for (j = ii; j < ssi[i].num; j++) {
				sid = ssi[i].stdlist[j];
				swi = std[sid].wish_index;
				oj  = std[sid].order_on_wish[swi];

				if (dxp->compare_order (&dep[i].stdlist[oi],
						&dep[i].stdlist[oj]) == 0) {
					ssk[i].stdlist[kk++] =
							ssi[i].stdlist[j];
					ii = j + 1;
					// fprintf (stderr, "Add one\n");
				} else {
					break;
				}
			}

			for (j = jj; j < ssj[i].num; j++) {
				sid = ssj[i].stdlist[j];
				swi = std[sid].wish_index;
				oj  = std[sid].order_on_wish[swi];

				if (dxp->compare_order (&dep[i].stdlist[oi],
						&dep[i].stdlist[oj]) == 0) {
					ssk[i].stdlist[kk++] =
							ssj[i].stdlist[j];
					jj = j + 1;
					// fprintf (stderr, "Add one\n");
				} else {
					break;
				}
			}

			if (kk >= num + extra_student_for_even) {
				misc->print (PRINT_LEVEL_SYSTEM,
					"Too many student have same score!!\n");
			}
		}

#if USE_PA_EXTERNAL_PRINT == 1
		if (kk > 0) {
			sid = ssk[i].stdlist[kk - 1];
			swi = std[sid].wish_index;
			oi  = std[sid].order_on_wish[swi];

			dep[i].llindex = oi + 1;
		}
#endif
		ssk[i].num = kk;

		/*
		if (kk != 0) {
			fprintf (stderr, "%d,%d,%d\n", i, kk, num);
		}
		*/

		for (; ii < ssi[i].num; ii++) {
#if USE_PA_EXTERNAL_PRINT == 1
			sid = ssi[i].stdlist[ii];
			swi = std[sid].wish_index;
			oi  = std[sid].order_on_wish[swi];
			dep[i].stdlist[oi].aflag |= SWF_INVALID;
#endif

			push_free_student (ssi[i].stdlist[ii]);
		}

		for (; jj < ssj[i].num; jj++) {
#if USE_PA_EXTERNAL_PRINT == 1
			sid = ssj[i].stdlist[jj];
			swi = std[sid].wish_index;
			oi  = std[sid].order_on_wish[swi];
			dep[i].stdlist[oi].aflag |= SWF_INVALID;
#endif

			push_free_student (ssj[i].stdlist[jj]);
		}
	}

	while ((ii = pop_free_student ()) >= 0) {
		for (i = std[ii].wish_index + 1;
					i < MAX_PER_STUDENT_WISH; i++) {
			std[ii].wish_index = i;

			if (std[ii].wish[i] <= 0) continue;

			jj = std[ii].wishidx[i];

			if (insert_to_solution_set (ssk, ii, i, jj)) break;
		}
	}

	free_solution_set (si);
	free_solution_set (sj);

	return sk;
}

static int divide_and_conquer (const int ii, const int jj, const int level) {
	int			i, j, m;
	int			si, sj;
	struct solution_set_t	*sol;

	if (ii == jj) {
		maxlevel = level > maxlevel ? level : maxlevel;

		si = new_solution_set ();
		clean_solution_set (si);

		sol = solset[si];

		std[ii].wish_index = -1;

		for (i = 0; i < MAX_PER_STUDENT_WISH; i++) {
			if (std[ii].wish[i] > 0) {
				std[ii].wish_index = i;
				j = std[ii].wishidx[i];
				sol[j].num = 1;
				sol[j].stdlist[0] = ii;
				//	std[ii].order_on_wish[i];
				break;
			}
		}

		return si;
	}

	m = (ii + jj) / 2;
	si = sj = -1;

	if (ii <= m) si = divide_and_conquer (ii, m, level + 1);
	if (jj >  m) sj = divide_and_conquer (m + 1, jj, level + 1);
	// fprintf (stderr, "Merge (%d,%d,%d)\n", ii, m, jj);

	return merge_solution_set (si, sj);
}

int pa_main (struct student_module_t *st,
				struct department_module_t *dp,
				struct dispatcher_unit_t *du) {
	FILE			*fp;
	char			*output_file;
	int			i, j, k, m, p;
	struct solution_set_t	*sol;

	dxp    = dp;

	std    = st->get_stdbuf ();
	stdnum = st->number_of_students ();
	dep    = dp->get_depbuf ();
	depnum = dp->number_of_departments ();

	if ((i = sysconfig->getint ("department-increase-space")) >
						EXTRA_STUDENT_FOR_EVEN) {
		extra_student_for_even = i;
	}

	if (! init_free_student_stack (stdnum)) return 0;

	misc->print (PRINT_LEVEL_SYSTEM,
		"\nDivide-and-Conquer Parallel Algorithm started\n");
	misc->timer_started ();

	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

	maxlevel = 0;
	solset_idx = 0;

	sol = solset[divide_and_conquer (0, stdnum - 1, 0)];
	// divide_and_conquer (0, 4096, 0);

	print_memory ();

	// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

	misc->print (PRINT_LEVEL_SYSTEM,
			"Elapsed time parallel algorithm (%d level, "
			"%d solution set)"
			":%s %.2f %ssecond(s)\r\n",
			maxlevel,
			solset_idx,
			misc->color (COLOR_BLUE, COLOR_YELLOW),
			misc->timer_ended (),
			misc->reset_color ());

	while ((output_file = sysconfig->getstr ("output-file")) != NULL) {
		if ((fp = fopen (output_file, "w")) == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM,
				"%s: ", output_file);
				misc->perror (PRINT_LEVEL_SYSTEM);
			break;
		}

#if USE_PA_EXTERNAL_PRINT == 0
		for (i = 0; i < depnum; i++) {
			for (j = 0; j < sol[i].num; j++) {
				k = sol[i].stdlist[j];
				m = std[k].wish_index;
				p = std[k].order_on_wish[m];

				fprintf (fp, WISHID_FMT ", %8d, %3d, %6d\n",
					dep[i].id,
					std[k].sid,
					m + 1,
					dep[i].stdlist[p].score
					);
			}
		}
#endif
#if 0
		for (i = 0; i < depnum; i++) {
			fprintf (fp, WISHID_FMT ", %3d, %3d",
				dep[i].id,
				dep[i].real_num,
				sol[i].num
				);

			if (dep[i].real_num > sol[i].num) {
				fprintf (fp, " -%d\n",
					dep[i].real_num - sol[i].num);
			} else if (dep[i].real_num < sol[i].num) {
				fprintf (fp, " +\n");
			} else {
				fprintf (fp, "\n");
			}
		}
#endif

		fclose (fp);
		break;
	}

#if USE_PA_EXTERNAL_PRINT == 1
	return 1;
#else
	return 0;
#endif
}

#endif
