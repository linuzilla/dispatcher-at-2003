/*
 *	usot.c	(University System of Taiwan)
 *
 *	Copyright (c) 2003, Jiann-Ching Liu
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include "predefine.h"
#include "global_var.h"
#include "sys_conf.h"
#include "misclib.h"
#include "studlist.h"
//////////////////////////////////////////////////////////////////
#include "usot.h"
#include "usot/usot_dtst.h"
#include "usot/usot_stud.h"
#include "usot/usot_dept.h"
#include "usot/usot_du.h"

static struct usot_student_module_t	*st = NULL;
static struct usot_department_module_t	*dp = NULL;

int usot_main (int argc, char *argv[]) {
	short				plevel = 0;
	char				*cfgfile = "usot.conf";
	struct usot_student_t		*student;
	struct usot_dispatcher_unit_t	*du = NULL;
	int16_t				wish;
	struct lost_student_list_t	*lostlist;
	int				write_wishtemp = 1;
	int				tryres;

#if 0
	int				c, errflag = 0;

	while ((c = getopt (argc, argv, "h"))!= EOF) {
		switch (c) {
		case 'h':
			errflag = 1;
			break;
		case '?':
		default:
			errflag = 1;
			break;
		}
	}

	if (errflag) {
		fprintf (stderr, "usot [-h][-T][-M]\n");
		return 1;
	}
#endif

	sysconfig = initial_sysconf_module (cfgfile, "cmdline-opt", 0);
	if (sysconfig == NULL) return 1;
	misc = initial_misc_libraries_modules (1);

	initial_global_variables ();

	if ((plevel = sysconfig->getint ("print-level")) <= 0) {
		plevel = PRINT_LEVEL_ALL;
	}

	misc->set_print_level (plevel);

	misc->timer_started ();
	misc->timer_started ();

	st = initial_usot_student_module ();
	dp = initial_usot_department_module ();

	if ((du = allocate_usot_dispatcher_unit (dp, st)) == NULL) {
		perror ("allocate_dispatcher_unit");
		return 1;
	}

	if (! st->load_wishes	  ()) return 1;
	if (! st->load_info	  ()) return 1;
	if (! st->load_score	  ()) return 1;
	if (! dp->load_limits	  ()) return 1;
	if (! dp->load_course_set ()) return 1;
	if (! dp->load_depna	  ()) return 1;
	if (! dp->load_courses	  ()) return 1;

	if (! st->prepare_sorted_stack ()) return 1;
	if (! dp->allocate_space ()) return 1;

	misc->print (PRINT_LEVEL_SYSTEM,
			"Elapsed time for data loading and "
			"preparing:%s %.2f %ssecond(s)\r\n",
			misc->color (COLOR_BLUE, COLOR_YELLOW),
			misc->timer_ended (),
			misc->reset_color ());
	print_memory ();

	misc->print (PRINT_LEVEL_SYSTEM, "\r\n"
			"Dispatching timer initialize ... "
			"dispatching in progress\r\n");
	misc->timer_started ();

	while ((student = st->pop ()) != NULL) {
		if ((student->sflag & USOT_STUDENT_FLAG_HAVE_INFO) == 0) {
			continue;
		}

		du->bind (du, student);

		while ((wish = st->nextwish (student)) != -1) {
			do {
				if (! du->load (du, wish)) break;
				if (! du->check_all_req (du)) break;
			} while (0);

			tryres = 1;

			if (du->is_acceptable (du, 0)) {
				if (du->try_accept (du, 0)) {
					lostlist = du->lostlist (du);

					while ((student = (void*)
							lostlist->get ())
								!= NULL) {
						--student->wish_index;
						st->push (student);
					}
					tryres = 0;
				}
			}

			if (tryres) {
				if (du->is_acceptable (du, 1)) {
					if (du->try_accept (du, 1)) {
						lostlist = du->lostlist (du);

						while (lostlist->get ()
								!= NULL) {
							// st->push (student);
						}
					}
				}
			}
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM,
		"Dispatching elapsed time:%s %.2f %ssecond(s)\r\n\r\n",
		misc->color (COLOR_BLUE, COLOR_YELLOW),
		misc->timer_ended (), misc->reset_color ());

	dp->write_wishfinal ();
	dp->write_wishminimal ();

	while (write_wishtemp) {
		int	fd;
		char	*fname;

		if ((fname = sysconfig->getstr ("wishtemp-file")) == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable (wishtemp-file)"
				" not define!!\r\n");
			break;
		}

		if ((fd = open (fname, O_CREAT|O_WRONLY|O_TRUNC, 0644)) < 0) {
			misc->perror (PRINT_LEVEL_SYSTEM);
			break;
		}

		if (! st->prepare_sorted_stack ()) return 1;

		misc->print (PRINT_LEVEL_SYSTEM,
				"Write wishtemp [ %s ] ... ", fname);

		while ((student = st->pop ()) != NULL) {
			if ((student->sflag &
					USOT_STUDENT_FLAG_HAVE_INFO) == 0) {
				continue;
			}

			st->reset_wish (student);

			du->bind (du, student);

			fdprintf (fd, "%06d  ", student->sid);

			while ((wish = st->nextwish (student)) != -1) {
				do {
					if (! du->load (du, wish)) break;
					if (! du->check_all_req (du)) break;
				} while (0);

				if (! student->have_wish[
						student->wish_index - 1]) {
					fdprintf (fd, "-----------");
				} else {
					// printf ("%d%6d",
					fdprintf (fd, "%04d%d%6d",
						wish,
						du->is_pass (du),
						du->score);
						//misc->toInt ((double)
						//	du->score
						//	/ 1000));
				}
			}

			fdprintf (fd, "\r\n");
		}
		close (fd);

		misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");
		break;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"\r\nTotal elapsed time: %.2f second(s)\r\n\r\n",
			misc->timer_ended ());

	return 0;
}
