/*
 *	show_info.c
 *
 *	Copyright (c) 2003, Jiann-Ching Liu
 */

#include <stdio.h>

#include "predefine.h"
#include "global_var.h"
#include "misclib.h"
#include "student.h"
#include "department.h"
#include "dispunit.h"
#include "sys_conf.h"
#include "stdvalue.h"

static struct student_t		*std = NULL;
static struct department_t	*dep = NULL;
static int			stdnum = 0;
static int			depnum = 0;

static void show_studinfo (const int idx) {
	struct student_t	*st;
	int			i;

	st = &std[idx];

	printf ("ID: %d\n", st->sid);

	printf ("學測: ");
	for (i = 0; i < 5; i++) {
		printf (" %2d", st->gradscore[i]);
	}
	printf (", 總級分: %2d\n", st->gradsum);

	printf ("指科: ");
	for (i = 0; i < 9; i++) {
		printf (" %5d", st->testscore[i]);
	}
	printf ("\n");
}

static void show_depinfo (const int idx) {
	struct department_t	*dp;

	dp = &dep[idx];
}

int show_info (struct student_module_t *st,
					struct department_module_t *dp,
					struct dispatcher_unit_t *du) {
	int			i, j;
	struct department_t	*d;

	std	= st->get_stdbuf ();
	stdnum	= st->number_of_students ();
	dep	= dp->get_depbuf ();
	depnum	= dp->number_of_departments ();

	printf ("------------------------------------------------"
		"-------------------------\n");

	if (trace_sid > 0) {
		for (i = j = 0; i < stdnum; i++) {
			if (std[i].sid == trace_sid) {
				show_studinfo (i);
				j++;
				break;
			}
		}

		if (j == 0) {
			printf ("Student ID: %d Not found\n", trace_sid);
		}
	}

	if (trace_wishid > 0) {
		if ((d = dp->find (trace_wishid)) != NULL) {
			show_depinfo (d->id);
		} else {
			printf ("Wish ID:" WISHID_FMT " Not found!\n",
					trace_wishid);
		}
	}
	return 0;
}
