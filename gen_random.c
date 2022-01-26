/*
 *	main.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__BORLANDC__)
#else
#include <unistd.h>
#endif

#include "predefine.h"
#include "global_var.h"
#include "sys_conf.h"
#include "misclib.h"

////////////////////////////////////////////////////////////////

#if WITH_RANDOM_DATA_GENERATING == 1

struct student_score_t {
	int	sid;            // student's identifier
	int	gradscore[5];
	int	testscore[9];
};

static struct student_score_t	*st;
static int			*stindex = NULL;
static int			number_of_students = 0;
static unsigned long long	sum;
/*
static int			stdgrade[5][5];
static int			stdtest[9][3];
*/

static int	cmpgort = 0;
static int	cmpitem = 0;

static int compare (int	*x, int *y) {
	if (cmpgort == 0) {
		if (st[*x].gradscore[cmpitem] > st[*y].gradscore[cmpitem]) {
			return -1;
		} else if (st[*x].gradscore[cmpitem] <
					st[*y].gradscore[cmpitem]) {
			return 1;
		} else {
			return 0;
		}
	} else {
		if (st[*x].testscore[cmpitem] > st[*y].testscore[cmpitem]) {
			return -1;
		} else if (st[*x].testscore[cmpitem] <
					st[*y].testscore[cmpitem]) {
			return 1;
		} else {
			return 0;
		}
	}
}

static int generate_index (const int gort, const int item) {
	int	i;

	cmpgort = gort;
	cmpitem = item;

	for (i = 0; i < number_of_students; i++) { stindex[i] = i; }

	qsort (stindex, number_of_students, sizeof (int),
			(int (*)(const void *, const void *)) compare);

	return 1;
}

int  generate_random_data (void) {
	int			i, j, k;
	int			gtr, etr;
	int			studlevel, variation;
	short			have_grade_test;
	short			have_exam_test;
	short			have_skill_test;
	short			exam_sort, skill_sort;
	int			vgrade[5];
	int			vtest[9];

	//	瓣璣计ヒ计菌瞶瞶て厩ネ
	//	 0   1    2     3     4     5    6      7     8
	short			score_take[5][9] = {
				//	  0  1  2  3  4  5  6  7  8
					{ 1, 1, 1, 1, 1, 1, 1, 1, 1 },
					{ 1, 1, 0, 1, 1, 1, 0, 0, 0 },
					{ 1, 1, 1, 0, 0, 0, 1, 1, 0 },
					{ 1, 1, 1, 0, 0, 0, 1, 1, 1 },
					{ 1, 1, 1, 0, 0, 0, 0, 1, 1 }
				};

#if defined(__BORLANDC__)
	srand (time (NULL));
#else
	srand (time (NULL) + getpid ());
#endif

	if ((number_of_students =
			sysconfig->getint ("rdg-number-of-students")) <= 0) {
		number_of_students = 100000;
	}

	if ((gtr = sysconfig->getint ("rdg-have-grade-test")) <= 0) {
		gtr = 100;
	}

	if ((etr = sysconfig->getint ("rdg-have-exam-test")) <= 0) {
		etr = 100;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"\r\n[*] Generate random data for dispatching\r\n\r\n");

	misc->print (PRINT_LEVEL_SYSTEM,
			"Allocate space for %d students ... ",
			number_of_students);

	if ((st = misc->calloc (number_of_students,
				sizeof (struct student_score_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 1;
	}

	if ((stindex = misc->calloc (number_of_students,
						sizeof (int))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 1;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");

	for (i = 0; i < 5; i++) { vgrade[i] = (rand () % 6) * 10 + 50; }
	for (i = 0; i < 9; i++) { vtest[i]  = (rand () % 6) * 10 + 50; }

	for (i = 0; i < number_of_students; i++) {
		st[i].sid = 2000000 + i;

		have_grade_test = have_exam_test = have_skill_test = 0;

		// ゑ耕盽篈よΑ盢σネだΘ 0 ~ 10000 单
		studlevel = (rand () % 4001) + (rand () % 4001) +
	                 ((int) ((rand () % 10) / 8) * (rand () % 1501)) +
	                 ((int) ((rand () % 10) / 9) * (rand () %  501));
		variation = (int) (studlevel / 4);

		if ((rand () % 100) < gtr) have_grade_test = 1; //Τ把厩代
		if ((rand () % 100) < etr) have_exam_test  = 1; //Τ把﹚ヘ
		if ((rand () % 100) < 40)  have_skill_test = 1; //砃

		for (j = 0; j < 5; j++) { st[i].gradscore[j] = -1; }
		for (j = 0; j < 9; j++) { st[i].testscore[j] = -1; }

		if (have_grade_test) {
			for (j = 0; j < 5; j++) {
				st[i].gradscore[j] = studlevel - variation + 
						(rand () % variation);
				st[i].gradscore[j] =
					(int) ((st[i].gradscore[j] *
							vgrade[j]) / 100);
			}
		}

		if (have_exam_test) {
			k = rand () % 100;

			if (k < 10) {
				exam_sort = 1;
			} else if (k < 20) {
				exam_sort = 2;
			} else if (k < 50) {
				exam_sort = 3;
			} else if (k < 55) {
				exam_sort = 4;
			} else {
				exam_sort = 0;
			}

			for (j = 0; j < 9; j++) {
				if (score_take[exam_sort][j] == 0) continue;

				st[i].testscore[j] = studlevel - variation + 
						(rand () % variation);
				st[i].testscore[j] =
					(int) ((st[i].testscore[j] *
							vtest[j]) / 100);
			}

			if (have_skill_test) {
				skill_sort = rand () % 8 + 1;
			}
		}
	}

	for (i = 0; i < 5; i++) {
		generate_index (0, i);

		sum = 0L;
		for (j = 0; j < number_of_students; j++) {
			if (st[stindex[j]].gradscore[i] < 0) {
				break;
			}

			sum += st[stindex[j]].gradscore[i];
		}

		printf ("%d - %llu - %.2f\n", j, sum, (double) sum / j);
	}

	for (i = 0; i < 9; i++) {
		generate_index (1, i);

		sum = 0L;
		for (j = 0; j < number_of_students; j++) {
			if (st[stindex[j]].testscore[i] < 0) {
				break;
			}

			sum += st[stindex[j]].testscore[i];
		}

		printf ("%d - %llu - %.2f\n", j, sum, (double) sum / j);
	}

	return 0;
}

#endif
