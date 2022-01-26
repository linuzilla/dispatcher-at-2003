/*
 *	main.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#if defined(__BORLANDC__)
// #include <conio.h>
//---------------------------------------------------------------------------
#pragma hdrstop
//---------------------------------------------------------------------------
#pragma argsused

#else
#include <strings.h>
#include <unistd.h>
#endif

#ifdef __linux__
#include <getopt.h>
#endif
#include "predefine.h"

#define MAX_USED_PTHREAD	8

#if ENABLE_PTHREAD == 1
#include <pthread.h>
#endif

#include "student.h"
#include "department.h"
#include "dispunit.h"
#include "studlist.h"
#include "sys_conf.h"
#include "stdvalue.h"
#include "misclib.h"
#include "textio.h"
#include "postcheck.h"
#include "usot.h"


#define DO_LOGWISH(x,y)	if (du->dp->id == logwish) { \
				fprintf (logwishfp, "%s :%d\n", y, x); }
////////////////////////////////////////////////////////////////
static char			*default_basic_crs[] =
					{ "國", "英", "數", "社", "自" };
static char			*default_exam_crs[] =
					{ "國", "英", "甲", "乙", "歷", "地",
						"物", "化", "生" };
static char			*default_onsame_crs[] = {
					"無", "總", "志",
					"國", "英", "數", "社", "自",
					"國", "英", "甲", "乙", "歷", "地",
					"物", "化", "生", "術" };
////////////////////////////////////////////////////////////////

struct sysconf_t		*sysconfig = NULL;
struct misc_libraries_t		*misc = NULL;
int				debug_flag = 0;
int				verbose = 0;
struct standard_values_t	*standval = NULL;
struct postcheck_t		*pstck = NULL;
int				selected_algorithm = 1;
int				trace_sid    = 0;
int				trace_wishid = -1;
int				skill_relative_weight   = 0;
int				weighted_skill_standard = 0;
int				include_student_extend_info = 0;
int				disallow_same_instrument = 0;
int				check_for_duplicate = 0;
int				grade_on_skill_physical = 0;
int				use_ncu_format = -1;
int				use_disp92 = 0;
int				logwish = 0;
int				no_wish_order = 0;
FILE				*logwishfp = NULL;
FILE				*cklfp = NULL;
#if DULL_ELEPHANT_OPTION == 1
short				de_is_skip_first_line = 0;
short				de_wish_use_oct = 1;
short				have_level_2_info = 0;
short				check_all_rules = 0;
short				myschool_id = 0;
short				dispatching_finalized = 0;
int				x_option = 0;
int				highsch_only = 0;
#endif
#if ENABLE_DR_LEE_SPECIAL == 1
short				dr_lee_optionflag = 0;
#endif

char				**dpg_basic_crs  = default_basic_crs;
char				**dpg_exam_crs   = default_exam_crs;
char				**dpg_onsame_crs = default_onsame_crs;

////////////////////////////////////////////////////////////////

static struct student_module_t		*st = NULL;
static struct department_module_t	*dp = NULL;

////////////////////////////////////////////////////////////////

struct dispconf_variables {
	char	*str;
	char	***list;
};

static struct dispconf_variables	dcvlist[] = {
		{ "basic-course-list"	, &dpg_basic_crs	},
		{ "course-list"		, &dpg_exam_crs		},
		{ "on-same-course-list"	, &dpg_onsame_crs	},
		{ NULL			, NULL			}
};

////////////////////////////////////////////////////////////////

#if ENABLE_PARALLEL_ALGORITHM == 1

#define PA_STUDENT_OPTIMAL	1
#define PA_DEPARTMENT_OPTIMAL	0

extern int pa_main (struct student_module_t *st,
		struct department_module_t *dp,
		struct dispatcher_unit_t *du);
#endif

#if ENABLE_INCREMENTAL_ALGORITHM == 1
short		getstdid_only = 0;

extern int incdp_main (struct student_module_t *st,
			struct department_module_t *dp,
			struct dispatcher_unit_t *du,
			const int clflag,
			const int qflag);
#endif

extern int show_info (struct student_module_t *st,
			struct department_module_t *dp,
			struct dispatcher_unit_t *du);

void print_memory (void);

void initial_global_variables (void);
static int  show_program_info (void);

#if ENABLE_PTHREAD == 1
static void initial_for_multithreading (void);
static void mp_stk_consumer (int *thrid);
static pthread_mutex_t	stk_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#if WITH_RANDOM_DATA_GENERATING == 1
int  generate_random_data (void);
#endif

////////////////////////////////////////////////////////////////

#if ! defined (__BORLANDC__)
int main (int argc, char *argv[]) {
#else
int unixmain (int argc, char *argv[]) {
#endif

#if defined(__BORLANDC__)
	int				optind;
        char                            *cfgfile = NULL;
#else
	char				*cfgfile = "dispatcher.conf";
#endif
#if DEBUG_LEVEL > 0
	int				j, k;
#endif
	int				i, c;
	int				proto_wish = 0;
	int				proto_weight = 0;
	int8_t				proto_course_weight[9];
	char				*proto_crs_file = NULL;
#if ENABLE_PTHREAD == 1
	pthread_t			all_threads[MAX_USED_PTHREAD];
	int				num_of_pthread = 0;
	int				all_thr_idx = 0;
	int				thr_args[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
#endif
	int				qflag = 0;
	int				load_depname = 0;
	short				load_info_first = 0;
	short				cli_algorithm = 0;
	short				with_rotation = 0;
	short				checkonly = 0;
	short				fullwishtable = 0;
	short				wishtmp = 0;
	short				difftest = 0;
	short				errflag = 0;
	short				plevel = 0;
	short				ansi_color = 0;
	short				output_form = -1;
	short				gen_random_flag = 0;
	short				presort = 1;
	short				additional = 0;
	short				wish_minimal = 0;
	short				use_checklist = 0;
	short				post_check = 0;
	short				check_on_allstudent = 0;
	short				info_only = 0;
	short				media_file = 0;
	short				load_new_wishes = 0;
#if ENABLE_INCREMENTAL_ALGORITHM == 1
	short				count_level_flag = 0;
#endif
	int32_t				score, minreq = 0;
	int32_t				wish;
	int				mscboard = -1;
	int				user_opt = 0;
#if ENABLE_PRE_TESTING
	int				pretest_flag = 0;
#endif
	struct dispatcher_unit_t	*du = NULL;
	struct student_t		*student;
	struct lost_student_list_t	*lostlist;
	FILE				*fp = stdout;
	char				*output_file;
	unsigned long			accept_count     = 0L;
	unsigned long			pop_count        = 0L;
	unsigned long			stkpop_count     = 0L;
	unsigned long			dispatcher_count = 0L;
	unsigned long			lost_count       = 0L;
	unsigned long			wrong_wish_count = 0L;
	unsigned long			sex_error        = 0L;
	unsigned long			skill_error	 = 0L;
	struct utsname			utsname_buf;
#if ENABLE_DEPARTMENT_FIRST == 1
	short				valid_wish[MAX_PER_STUDENT_WISH];
#endif
	char				*program_name, *cp;
	char				*alg_name[] = {
						"Student optimal",
						"Department optimal",
						"Parallel Algorithm",
						"Incremental Dispatching "
						"Algorithm"
					};

	uname (&utsname_buf);
	fprintf (stderr, "\n%s %s %s [%s]\n",
				utsname_buf.sysname,
				utsname_buf.machine,
				utsname_buf.release,
				utsname_buf.version);

	program_name = ((cp = strrchr (argv[0], '/')) != NULL) ? cp+1 : argv[0];

	fprintf (stderr, "\n"
	    "Dispatcher Version %s (R) All rights reserved.\n"
	    "Copyright (C) 2002-2003, Written by  Jiann-Ching Liu  "
	    "at Computer Center of NCU\n\n", VERSION);

	if (strcmp (program_name, "usotdp") == 0) return usot_main (argc, argv);

#if ! defined(__BORLANDC__)
	while ((c = getopt (argc, argv,
			"12345abhcngrqNmvizwkKEpILRTCDFUB:M:f:o:t:u:l:P:x:H:"
			))!= EOF) {
		switch (c) {
		case '1':
			cli_algorithm = 1;
			break;
		case '3':
			with_rotation = 1;
		case '2':
			cli_algorithm = 2;
			presort = 0;
			break;
		case '4':
#if ENABLE_PARALLEL_ALGORITHM == 1
			cli_algorithm = 3;
			presort = 0;
#else
			fprintf (stderr, "Parallel algorithm not activated\n");
#endif
			break;
		case '5':
#if ENABLE_INCREMENTAL_ALGORITHM == 1
			cli_algorithm = 4;
#else
			fprintf (stderr,
				"Incremental algorithm not activated\n");
#endif
			break;
		case 'x':
			x_option = atoi (optarg);
			break;
		case 'z':
			load_new_wishes = 1;
			checkonly = 1;
			use_checklist = 3;
			load_info_first = 1;
			include_student_extend_info = 1;
			dispatching_finalized = 1;
			break;
		case 'H':
			highsch_only = atoi (optarg);
			break;
		case 'D':
#if ENABLE_DR_LEE_SPECIAL == 1
			dr_lee_optionflag++;
#endif
			break;
		case 'g':
#if ENABLE_INCREMENTAL_ALGORITHM == 1
			count_level_flag = 1;
#else
			fprintf (stderr,
				"Incremental algorithm not activated\n");
#endif
			break;
		case 'E':
			have_level_2_info = 1;
			include_student_extend_info = 1;
			break;
		case 'B':
			mscboard = atoi (optarg);
			break;
		case 'K':
			checkonly = 2;	// use wish minimal
			include_student_extend_info = 1;
			check_on_allstudent = 1;
			break;
		case 'U':
			return usot_main (argc, argv);
		case 'L':
			if (++use_checklist == 3) load_info_first = 1;
			include_student_extend_info = 1;
			break;
		case 'I':
			info_only = 1;
			break;
		case 'q':
			qflag++;
			break;
		case 'w':
			proto_weight = 1;
			break;
		case 'r':
			check_all_rules = 1;
			presort = 2;
			break;
		case 'P':
			{
				int	i, len;
				int	j = 0;

				if ((len = strlen (optarg)) == 4) {
					for (i = 0; i < len; i++) {
						j = j * 8;
						if (optarg[i] > '7') {
							j = 0;
							break;
						}
						if (optarg[i] < '0') {
							j = 0;
							break;
						}
					}

					j += (int)(optarg[i] - '0');
				} else if (len == 9) {
					j = -1;

					fprintf (stderr, "STANDARD:");

					proto_crs_file = optarg;

					for (i = 0; i < len; i++) {
						if (optarg[i] < '0') {
							j = 0;
							break;
						}
						if (optarg[i] > '4') {
							j = 0;
							break;
						}
						proto_course_weight[i] =
							(int) (optarg[i] - '0');

						if (proto_course_weight[i] == 0)
							continue;

						fprintf (stderr, "  %sx",
							default_exam_crs[i]);
								
						switch (proto_course_weight[i]){
						case 1:
							fprintf (stderr,
								"100%%");
							break;
						case 2:
							fprintf (stderr,
								"125%%");
							break;
						case 3:
							fprintf (stderr,
								"175%%");
							break;
						case 4:
							fprintf (stderr,
								"200%%");
							break;
						}
					}
					fprintf (stderr, "\n");
				}

				proto_wish = j;
			}
			break;
		case 'p':
#if ENABLE_PRE_TESTING
			pretest_flag = 1;
#else
			fprintf (stderr, "option not active\n");
#endif
			break;
		case 'u':
			user_opt = atoi (optarg);
			break;
		case 'i':
			return show_program_info ();
		case 'k':
			post_check = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'a':
			additional = 1;
			break;
		case 'n':
			presort = 0;
			break;
		case 'm':
			include_student_extend_info = 1;
			if (++media_file > 1) have_level_2_info = 1;
			break;
		case 'N':
			no_wish_order = 1;
			break;
		case 'o':
			output_form = atoi (optarg);

			if ((output_form < 0) || (output_form > 7)) {
				output_form = -1;
			}
			break;
		case 'R':
			gen_random_flag = 1;
			break;
		case 'b':
			output_form = 1;
			break;
		case 't':
#if ENABLE_PTHREAD == 1
			i = atoi (optarg);

			if ((i >= 0) && (i <= MAX_USED_PTHREAD)) {
				num_of_pthread = i;
			}
#else
			fprintf (stderr, "Pthread support not activate\n");
			errflag++;
#endif
			break;
		case 'T':
			wishtmp = 1;
			break;
		case 'F':
			fullwishtable = 1;
			break;
		case 'M':
			wish_minimal = atoi (optarg);
			break;
		case 'f':
			cfgfile = optarg;
			break;
		case 'C':
			difftest = 1;
		case 'c':
			checkonly++;
			break;
		case 'l':
			while (1) {
				int	i, len;
				int	j = 0;

				if ((len = strlen (optarg)) != 4) break;

				for (i = 0; i < len; i++) {
					j = j * 8;
					if (optarg[i] > '7') { j = 0; break; }
					if (optarg[i] < '0') { j = 0; break; }

					j += (int)(optarg[i] - '0');
				}

				logwish = j;
				break;
			}
			break;
		case 'h':
		default:
			errflag++;
			break;
		}
	}
#else
	for (optind = 1; optind < argc; optind++) {
		if (argv[optind][0] == '-') {
			for (i = 1; argv[optind][i] != '\0'; i++) {
				switch (argv[optind][i]) {
				case '1':
					cli_algorithm = 1;
					break;
				case '3':
					with_rotation = 1;
				case '2':
					cli_algorithm = 2;
					break;
				case 'a':
					additional = 1;
					break;
				case 'v':
					verbose++;
					break;
				case 'n':
					presort = 0;
					break;
				case 'b':
					output_form = 1;
					break;
				case 'M':
					wish_minimal = 1;
					break;
				case 'R':
					gen_random_flag = 1;
					break;
				case 'T':
					wishtmp = 1;
					break;
				case 'F':
					fullwishtable = 1;
					break;
				case 'C':
					difftest = 1;
				case 'c':
					checkonly++;
					break;
				case 'h':
				default:
					errflag++;
					break;
				}
			}
		} else {
			break;
		}
	}
#endif        

	if (errflag > 0) {
		fprintf (stderr,
		    "%s [-12345ianbhkgqKcvpDMRTCF][-t thr][-o form]"
		    "[-u u_opt][-f cfgfile]\n\n"
		    "\t-1        using student optimal algorithm\n"
		    "\t-2        using department optimal algorithm\n"
		    "\t-3        using department optimal algorithm "
		    "(with rotation)\n"
		    "\t-4        using parallel algorithm\n"
		    "\t-5        using incremental algorithm\n"
		    "\t-t [0~%d]  Number of Thread to run\n"
		    "\t-u 0~n    User option, (see dispatcher.conf)\n"
		    "\t-v        verbose\n"
		    "\t-p        pre-testing\n"
		    "\t-P wish   use wish as proto\n"
		    "\t-i        show program infomation\n"
		    "\t-L        Include check-list\n"
		    "\t-g        Count score level list\n"
		    "\t-a        enable addition student wish, info, score "
		    "file\n"
		    "\t-n        do not pre-sort data before dispatching\n"
		    "\t-N        disable wish order\n"
		    "\t-c        check only (from students\' point of view)\n"
		    "\t-C        check only and difference testing\n"
		    "\t-K        check on all student "
		    		"(as check-list in check mode)\n"
		    "\t-k        post check\n"
		    "\t-q        SQL output\n"
		    "\t-R        generate random testing data (obsolete)\n"
		    "\t-T        generate wish tmp file\n"
		    "\t-F        generate full wish table (no dispatching)\n"
		    "\t-m        generate media files\n"
		    "\t-E        Student Extend info\n"
		    "\t-M [1~4]  generate wishminimal file\n"
		    "\t-o [0~7]  output format\n"
		    "\t            0: user-defined output format\n"
		    "\t            1: base on department format\n"
		    "\t            2: list format (wishfinal)\n"
		    "\t            3: list format (short)\n"
		    "\t            4: list, with on same reference, can be "
		    "check using \"-c\"\n"
		    "\t            5: list, same as \"-o 4\", with name\n"
		    "\t            6: list, with reject students (only for "
		    "algorithm 1)\n"
		    "\t            7: list, full for year 92\n"
		    "\t-B [0-x]  Minimal Score Order\n"
		    "\t            0: (not full)\n"
		    "\t            1: Table (no-sort)\n"
		    "\t            2: Table (sort by org score)\n"
		    "\t            3: Table (sort by not qualify)\n"
		    "\t            4: Table (sort by reject)\n"
		    "\t            5: Table (on-same use)\n"
		    "\t            6: Table (Minimal Require, Min/Max Score)\n"
		    "\t-x [n]    Working with other option for Miscellaneous "
		    		"purpose\n"
		    "\t-D        Enable Dr. Lee Special\n"
		    "\n",
		    program_name,
		    MAX_USED_PTHREAD
		    );
		return 1;
	}

	sysconfig = initial_sysconf_module (cfgfile, "cmdline-opt", user_opt);

	if (sysconfig == NULL) return 1;

	if ((ansi_color = sysconfig->getint ("ansi-color")) == -1) {
		ansi_color = 0;
	}

	misc  = initial_misc_libraries_modules (ansi_color);

#if DEBUG_LEVEL > 0
	for (k = optind; k < argc; k++) {
		for (i = j = 0; argv[k][i] != '\0'; i++) {
			if (((c = argv[k][i]) < '0') || (c > '9')) {
				j++;
				break;
			}
		}

		if (j == 0) {
			if (i == STUDENT_ID_LENGTH) {
				sysconfig->addint ("trace-student", argv[k]);
			} else if (i == DEPARTMENT_ID_LENGTH) {
				sysconfig->addint ("trace-department", argv[k]);
				// fprintf (stderr, "[%s]\n", argv[k]);
			}
		}
	}

	if ((trace_sid = sysconfig->getint ("trace-student")) > 0) {
#  if DEBUG_LEVEL > 0
		fprintf (stderr, "Trace on student: %d\n", trace_sid);
#  endif
	}

	if (logwish > 0) {
		char	buffer[50];

		sprintf (buffer, WISHID_FMT ".txt", logwish);
		fprintf (stderr, "Log on " WISHID_FMT " to %s\n",
					logwish, buffer);
		logwishfp = fopen (buffer, "w");
	}

	if ((trace_wishid = sysconfig->getint ("trace-department")) >= 0) {
#  if DEPARTMENT_ID_LENGTH == 4
		int	i, j, k;

		k = 1;
		j = trace_wishid;
		trace_wishid = 0;

		do {
			if ((i = j % 10) > 7) {
				trace_wishid = -1;
				break;
			}

			trace_wishid += i * k;
			j = (int) ((j - i) / 10);
			k = k * 8;
		} while (j != 0);
#  endif

#  if DEBUG_LEVEL > 0
		if (trace_wishid >= 0) {
			fprintf (stderr, "Trace on department: "
					WISHID_FMT "\n", trace_wishid);
		}
#  endif
	}
#endif


	if ((i = sysconfig->getint ("grade-on-skill-physical")) != -1) {
		grade_on_skill_physical = i;
	}

	if ((i = sysconfig->getint ("check-for-duplicate")) != -1) {
		check_for_duplicate = i;
	}

	if ((i = sysconfig->getint ("my-school-id")) != -1) myschool_id = i;

	if (sysconfig->getstr ("department-name-file") != NULL) {
		load_depname = 1;
	}

	if ((debug_flag = sysconfig->getint ("debug")) == -1) debug_flag = 0;

	if (verbose == 0) {
		if ((verbose = sysconfig->getint ("verbose")) == -1) {
			verbose = 0;
		}
	}

	if (skill_relative_weight == 0) {
		if ((skill_relative_weight = sysconfig->getint (
					"skill-relative-weight")) == -1) {
			skill_relative_weight = 0;
		}
	}

	if (weighted_skill_standard == 0) {
		if ((weighted_skill_standard = sysconfig->getint (
					"weighted-skill-standard")) == -1) {
			skill_relative_weight = 0;
		}
	}

	if ((disallow_same_instrument = sysconfig->getint (
				"disallow-same-instrument")) == -1) {
		disallow_same_instrument = 0;
	}

#if ENABLE_DISPATCHER92 == 1
	if ((use_disp92 = sysconfig->getint ("use-deplim92-format")) == -1) {
		use_disp92 = 0;
	}
#endif

#if DULL_ELEPHANT_OPTION == 1
	if (sysconfig->getint ("dull-elephant-first-line") > 0) {
		de_is_skip_first_line = 1;
		misc->print (PRINT_LEVEL_SYSTEM,
			"[DE] Skip First-Line for deplim and skillset\r\n");
	}

	if (sysconfig->getint ("dull-elephant-decimal-wishid") > 0) {
		de_wish_use_oct = 0;
		misc->print (PRINT_LEVEL_SYSTEM,
				"[DE] Using decimal as wish id\r\n");
	}
#endif

	if (use_ncu_format == -1) {
		if ((use_ncu_format = sysconfig->getint (
						"use-ncu-format")) == -1) {
			use_ncu_format = 1;
		}
	}


#if ENABLE_DEPARTMENT_FIRST == 1
	if (cli_algorithm != 0) {
		selected_algorithm = cli_algorithm;
	} else if ((selected_algorithm = sysconfig->getint (
					"dispatching-algorithm")) == -1) {
		if (fullwishtable || wishtmp) {
			selected_algorithm = 2;
		} else {
			selected_algorithm = 1;
		}
	}

#  if ENABLE_INCREMENTAL_ALGORITHM == 1
	if (selected_algorithm > 4) selected_algorithm = 1;
#  elif ENABLE_PARALLEL_ALGORITHM == 1
	if (selected_algorithm > 3) selected_algorithm = 1;
#  else
	if (selected_algorithm > 2) selected_algorithm = 1;
#  endif
#else
	if (fullwishtable || wishtmp) {
		fprintf (stderr, "Function not active\n");
		return 1;
	}
#endif

	// 只有學生榜目前接受 format 6 的輸出格式
	if ((selected_algorithm != 1) && (output_form == 6)) output_form = -1;

	if (output_form == -1) {
		if ((output_form = sysconfig->getint ("output-format")) < 0){
			output_form = 2;
		}
	}

	// 格式 5 的輸出包括姓名 ... so
	if ((output_form == 5) || (output_form == 0))
		include_student_extend_info = 1;


	initial_global_variables ();

	if (! gen_random_flag) {
		fprintf (stderr,
			"Dispatching algorithm:%s %s %s %s(form: %d)\n",
				misc->color (COLOR_BLUE, COLOR_YELLOW),
				alg_name[selected_algorithm - 1],
				misc->reset_color (),
				(with_rotation == 1 ? "[with rotation] " : ""),
				output_form);
	}

	if ((plevel = sysconfig->getint ("print-level")) <= 0) {
		plevel = PRINT_LEVEL_ALL;
	}

	misc->set_print_level (plevel);

	misc->print (PRINT_LEVEL_DEBUG,
		"[+] PL=%d, 92=%d, DASI=%d, GOSP=%d, DPCK=%d\n",
		plevel, use_disp92,
		disallow_same_instrument,
		grade_on_skill_physical,
		check_for_duplicate);


	if (gen_random_flag) {
#if WITH_RANDOM_DATA_GENERATING == 1
		generate_random_data ();
		print_memory ();
#else
		fprintf (stderr, "Option not active\n");
#endif
		return 0;
	}

	////////////////////////////////////////////////////////////////

	misc->timer_started ();
	misc->timer_started ();

	// 1. load all data file into memory with minor conversion
	//
	//	(a) memory allocate function
	//	(b) text file I/O library
	//
	// 2. sort student data into a index list as queue for dispatch
	//
	// 3. allocate space for each deprtment to save its students.
	//
	// 4. dispatch each student from queue until queue is empty

	// 程式開始先載入學生模組及志願(系組)模組

	st = initial_student_module ();
	dp = initial_department_module (output_form);


	if ((du = allocate_dispatcher_unit (dp, st)) == NULL) {
		perror ("allocate_dispatcher_unit");
		return 1;
	}

#if ENABLE_INCREMENTAL_ALGORITHM == 1
	if ((selected_algorithm == 4) || count_level_flag) {
		if (!incdp_main (st, dp, du, count_level_flag, qflag)) return 1;
		return 0;
	}
#endif
	///////////////////////////////////////////////////////////
	//

	// 載入志願檔
#if ENABLE_PRE_TESTING
	if (pretest_flag) {
		if (! st->load_all_students (1)) return 0;
	} else {
		if (! st->load_wishes (additional, checkonly > 1 ? 1 : 0)) {
			return 1;
		}
	}
#else
	if (load_info_first) {
		if (! st->load_all_students (0)) return 1;
		if (! st->load_info         (0)) return 1;
		if (! st->load_wishes       (0, 0)) return 1;
	} else {
		if (! st->load_wishes (additional, checkonly > 1 ? 1 : 0)) {
			return 1;
		}
	}

	if (load_new_wishes) st->load_new_wishes ();
#endif
	if (! load_info_first) {
		if (! st->load_info (additional)) return 1; // 載入學生資訊檔
	}

	if (! st->load_score    (additional)) return 1; // 載入學生成績檔
	if (! dp->load_limits   ()) return 1; // 載入志願(系組)限制檔

	dp->analy_deplim_grp ();

	if (load_depname) if (! dp->load_depna ()) return 0;
	if (! dp->load_skillset ()) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"Something error on loading skillset ... Ignore\r\n");
	}

	pstck = init_postcheck_module (st, dp);

	if (use_checklist) {
#if ENABLE_CHECKLIST == 1
		if (! st->load_checklist (use_checklist)) return 1;
		// if (! dp->load_wishminimal ()) return 1;
#else
		fprintf (stderr, "Option not active\n");
		return 1;
#endif
	} else if (check_on_allstudent) {
#if ENABLE_CHECKLIST == 1
		if (! st->load_checklist_all (use_checklist)) return 1;
		// if (! dp->load_wishminimal ()) return 1;
		use_checklist++;
#else
		fprintf (stderr, "Option not active\n");
		return 1;
#endif
	}
	
	if (checkonly != 0) {
		switch (checkonly) {
		case 1:
			if (! du->load_test (du, 0)) return 1;
			break;
		case 2:
		default:
			if (check_on_allstudent) {
				if (! du->load_test (du, 1)) return 1;
			}
			if (! dp->load_wishminimal ()) return 1;
			break;
		}
	}

#if ENABLE_DENYLIST == 1
	st->load_deny ();
#endif

	if ((standval = initial_standard_values ()) == NULL) {
		// 沒有標準檔, 當所有數值為 0
#if ! defined(__BORLANDC__)
		standval = misc->malloc (sizeof (struct standard_values_t));
		bzero ((void *) standval, sizeof (struct standard_values_t));
#else
                standval = misc->calloc (1, sizeof (struct standard_values_t));
#endif
	}

	if (info_only) return show_info (st, dp, du);

	// st->load_special ();	// 載入特種考生資料檔

	// 自此脫離 Disk 作業, 完全以 Memory 處理

	// Sort 學生資料, 建成堆疊
	if (! st->prepare_sorted_stack (presort)) return 1;


#if ENABLE_DEPARTMENT_FIRST == 1
	if (selected_algorithm == 1) {
		if (! dp->allocate_space ()) return 1; // 志願(系組)的名額空間
	}
#else
	if (! dp->allocate_space ()) return 1; // 志願(系組)的名額空間
#endif

	misc->print (PRINT_LEVEL_SYSTEM,
		"Elapsed time for data loading and "
		"preparing:%s %.2f %ssecond(s)\r\n",
		misc->color (COLOR_BLUE, COLOR_YELLOW),
		misc->timer_ended (),
		misc->reset_color ());

	if (selected_algorithm == 1) print_memory ();


#if ENABLE_DEPARTMENT_FIRST == 1
	if (selected_algorithm == 1) {
#endif
		if (checkonly) {
			misc->print (PRINT_LEVEL_SYSTEM, "\r\n"
				"Dispatching timer initialize ... "
				"checking in progress\r\n");
		} else {
			misc->print (PRINT_LEVEL_SYSTEM, "\r\n"
				"Dispatching timer initialize ... "
				"dispatching in progress\r\n");
		}
#if ENABLE_DEPARTMENT_FIRST == 1
	} else {
		misc->print (PRINT_LEVEL_SYSTEM, "\r\n"
			"Pre-parse all student\'s wishes to get "
			"memory requirements\r\n");
	}
#endif
	misc->timer_started ();

#if ENABLE_PTHREAD == 1
	if (num_of_pthread > 1) {
		selected_algorithm = 1;

		initial_for_multithreading ();
#ifdef SOLARIS
		// Solaris 需要設定 concurrency 才有用
		pthread_setconcurrency (num_of_pthread);

		misc->print (PRINT_LEVEL_SYSTEM,
			"Multi-threading support: pthread, "
			"curcurrency = %d\r\n",
			pthread_getconcurrency ());
#else
		misc->print (PRINT_LEVEL_SYSTEM,
			"Multi-threading support: enable pthread\n");
#endif


		// fprintf (stderr, "%d\n", pthread_getconcurrency ());

		for (all_thr_idx = 0; all_thr_idx < num_of_pthread;
							all_thr_idx++) {
			// fprintf (stderr, "%d thread cread\n", all_thr_idx);
			pthread_create (&all_threads[all_thr_idx],
					NULL, (void *) mp_stk_consumer,
					&thr_args[all_thr_idx]);
		}

		for (i = 1; i < all_thr_idx; i++) {
			pthread_join (all_threads[i], NULL);
			// fprintf (stderr, "%d thread joind\n", i);
		}
	}
#endif

	// misc->print (PRINT_LEVEL_INFO, "dispatching .......");

	while ((student = st->pop ()) != NULL) {
		stkpop_count++;

#if ENABLE_DEPARTMENT_FIRST == 1
		if (selected_algorithm != 1) {
			for (i = 0; i < MAX_PER_STUDENT_WISH; i++) {
				valid_wish[i] = 0;
			}
		}
#endif

#if ENABLE_PRE_TESTING
		if (! pretest_flag) {
#endif
		if ((student->sflag & STUDENT_FLAG_HAVE_SCORE) == 0) {
			// 跳過沒有成績的學生
			continue;
		}


		if ((student->sflag & STUDENT_FLAG_DISALLOW) != 0) {
#if DEBUG_LEVEL > 8
			// 跳過不符合登記規定的學生
			dprint ("student: %d is disallow\r\n", student->sid);
#endif

#if ENABLE_DEPARTMENT_FIRST == 1
			if (selected_algorithm != 1) {
				st->set_valid_wish_mask (student, valid_wish);
			}
#endif
			continue;
		}
#if ENABLE_PRE_TESTING
		}
#endif
#if DEBUG_LEVEL > 9
		if (student->sid == 22002908) {
			dprint ("Load student: %d\r\n", student->sid);
		}
#endif

#if ENABLE_POSTCHECK == 1
		// pstck->start (student);
#endif
		du->bind (du, student);

		while ((wish = st->nextwish (student)) != -1) {
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			student->not_qualify_flag = 0;
#endif

			// loop till no more wishes
#if DEBUG_LEVEL > 9
			if (student->sid == 22002908) {
				dprint ("%d - %d (" WISHID_FMT ")\r\n",
					student->sid,
					student->wish_index,
					wish);
			}
#endif

			// 未填此志願, 再看下個志願
			if (wish <= 0) {
#if ENABLE_POSTCHECK == 1
				pstck->wish_not_exists (student,
						student->wish_index,
						wish);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				student->not_qualify_flag |=
							STNQF_WISH_NOT_FOUND;
				if (check_all_rules) {
					pstck->write_resultcheck (student,
							student->wish_index,
							wish);
				}
#endif
				continue;
			}

#if DEBUG_LEVEL > 1
			if (trace_sid == student->sid) {
				dprint ("student: %d %s%d%s%s wish %s"
						WISHID_FMT
						"%s started\r\n", 
					student->sid,
					misc->color (COLOR_RED, COLOR_WHITE),
					student->wish_index,
					misc->number_order_postfix (
						student->wish_index),
					misc->reset_color (),
					misc->color (COLOR_BLUE, COLOR_YELLOW),
					wish,
					misc->reset_color ());
			}
#endif

			if (! du->load (du, wish)) {
				// 所填的志願不存在

#if ENABLE_POSTCHECK == 1
				pstck->wish_not_exists (student,
						student->wish_index,
						wish);
#endif

#if DEBUG_LEVEL > 9
				dprint ("student: %d wish " WISHID_FMT
						" not found\r\n", 
						student->sid, wish);
#endif
				wrong_wish_count++;

#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				student->not_qualify_flag |=
							STNQF_WISH_NOT_FOUND;
				if (check_all_rules) {
					pstck->write_resultcheck (student,
							student->wish_index,
							wish);
				}
#endif
				continue;
#if ENABLE_DEPARTMENT_FIRST == 1
			} else if (selected_algorithm != 1) {
				// 利用已經查出的志願索引, 將志願索引填入
				// wishidx 的陣列中, 主要是為加速用,
				// 免得每次要重新 search 一次
				du->set_wishidx (du);
#endif
			}

#if DEBUG_LEVEL > 1
			if (trace_sid == student->sid) {
				if (du->dp->cname != NULL) {
					dprint (WISHID_FMT ":[%s][%s]\r\n",
						du->dp->id,
						du->dp->univ->sch_name,
						du->dp->cname);

				}
			}
#endif
#if ENABLE_PRE_TESTING
			if (pretest_flag) {
				// pretest_flag = 1;
				if (du->pre_checkall (du, 0)) {
					// printf ("%d\n", student->sid);
				}
				break;
			}
#endif

			dispatcher_count++;
			// misc->print (PRINT_LEVEL_INFO, "\b%c",


			// 先檢查是否有性別限制 ...

			if (! du->check_sex (du)) {
				sex_error++;
				du->dp->n_qualify_cnt++;
#if DEBUG_LEVEL > 2
				if (trace_wishid == du->dp->id) {
					printf ("SEX:" WISHID_FMT ", %d\n",
						trace_wishid, du->st->sid);
				}
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				student->not_qualify_flag |= STNQF_GENDER;
#endif
				if (! check_all_rules) continue;
			}

			// 術科檢定
			if (! du->check_skill (du)) {
				skill_error++;
				du->dp->n_qualify_cnt++;
#if DEBUG_LEVEL > 2
				if (trace_wishid == du->dp->id) {
					printf ("SKL:" WISHID_FMT ", %d\n",
						trace_wishid, du->st->sid);
				}
#endif
				if (! check_all_rules) continue;
			}

			// 特殊限制檢定

			if (! du->check_special (du)) {
				du->dp->n_qualify_cnt++;
#if DEBUG_LEVEL > 2
				if (trace_wishid == du->dp->id) {
					printf ("SPL:" WISHID_FMT ", %d\n",
						trace_wishid, du->st->sid);
				}
#endif
				if (! check_all_rules) continue;
			}

			// 學測檢定 ... 若不合格立刻跳下個志願
			if (! du->check_basic (du)) {
				du->dp->n_qualify_cnt++;
#if DEBUG_LEVEL > 2
				if (trace_wishid == du->dp->id) {
					printf ("BSK:" WISHID_FMT ", %d\n",
						trace_wishid, du->st->sid);
				}
#endif
				if (! check_all_rules) continue;
				// continue;
			}

			// 指定科目檢定 ... 若不合格立刻跳下個志願
			if (! du->check_exam (du)) {
				du->dp->n_qualify_cnt++;
#if DEBUG_LEVEL > 1
				if (trace_wishid == du->dp->id) {
					printf ("EXM:" WISHID_FMT ", %d\n",
						trace_wishid, du->st->sid);
				}
#endif
				if (! check_all_rules) continue;
			}

			// 計算得分
			score = du->get_score (du);

			// 成績小於零, 表某些科目沒報名 ...
			if (score < 0) {
#if DEBUG_LEVEL > 2
				if (trace_wishid == du->dp->id) {
					printf ("NOP:" WISHID_FMT ", %d\n",
						trace_wishid, du->st->sid);
				}
#endif
				du->dp->n_qualify_cnt++;
				DO_LOGWISH(student->sid, "!*z");

				if (! check_all_rules) continue;
			}
#if ENABLE_DENYLIST == 1
			// 除錯用的, 看看這個志願是否拒收此學生
			if (st->deny (student->sid, wish)) {
#  if DEBUG_LEVEL > 1
				misc->print (PRINT_LEVEL_DEBUG, 
				       "Deny %d to " WISHID_FMT " by force\r\n",
						student->sid, wish);
#  endif
#if DEBUG_LEVEL > 2
				if (trace_wishid == du->dp->id) {
					printf ("DNY:" WISHID_FMT ", %d\n",
						trace_wishid, du->st->sid);
				}
#endif
				du->dp->n_qualify_cnt++;
				continue;
			}
#endif

#if ENABLE_DEPARTMENT_FIRST == 1
			if (selected_algorithm == 1) {
#endif
				// 取得目前的最低錄取分數
				minreq = du->min_require (du);

				if (checkonly == 1) {
					if (du->dp->num > du->dp->inlistcnt) {
						minreq = -1;
					}
				}

				// 比較是否達錄取標準
				if (score < minreq) {
					// 檢查同分參酌方式
					//accept = dp->check_on_same (wish,
					//	student, special, score);
#if DEBUG_LEVEL > 9
					dprint ("not accept, score = %d, "
							"min = %d\r\n",
						score, minreq);
#endif
					du->dp->reject_cnt++;
					DO_LOGWISH(student->sid, "!-x");
#if DEBUG_LEVEL > 2
					if (trace_wishid == du->dp->id) {
						printf ("REJ:" WISHID_FMT
							", %d (%d < %d)\n",
							trace_wishid,
							du->st->sid,
							score, minreq);
					}
#endif
#if ENABLE_POSTCHECK == 1
					pstck->score_not_enough (student,
						student->wish_index,
						wish, score, minreq);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
					student->not_qualify_flag |=
							STNQF_SCORE_NOT_ENOUGH;
#endif
					if (! check_all_rules) continue;
					// continue;	// 看下個志願
				}

#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				if (check_all_rules &&
					     (student->not_qualify_flag != 0)) {
					pstck->write_resultcheck (student,
							student->wish_index,
							wish);
					if (wish == du->st->matched_wish) {
						fprintf (stderr,
							"OOPS!!! for %d "
							WISHID_FMT
							" (match %d)\n",
							du->st->sid,
							wish,
							student->
							   not_qualify_flag);
					}
					continue;
				}
#endif

				if (checkonly) {
					if (difftest) {
						if (wish !=
							du->st->matched_wish) {
							continue;
						}
						break;
					}

					if (wish == du->st->matched_wish) {
#if ENABLE_CHECKLIST == 1
						if (! use_checklist) break;
						if (student->ck == NULL) break;
#endif
#if ENABLE_POSTCHECK == 1
						pstck->found_match (student,
							student->wish_index,
							wish,
							du->st->still_checking,
							score,
							minreq);

						du->st->still_checking = 0;
#  if ENABLE_FULL_FAIL_RESULT_CHECK == 1
						if (check_all_rules) {
							pstck->
							     write_resultcheck (
								student,
							    student->wish_index,
							    wish);
							break;
						}
#  endif
						continue;
#else
						break;
#endif
					}

					if (score == minreq) {
						int	i, j;
						int32_t	*list;
#if DEBUG_LEVEL > 1
						if (trace_sid == student->sid) {
							dprint ("%d on same "
								WISHID_FMT
								" (%d/%d)\n",
								trace_sid,
								du->dp->id,
								du->dp->cstgrp,
							       du->dp->take_sort
								);
						}
#endif

						//fprintf (stderr, "[%d]\n",
						//		du->dp->cstgrp);

						// 同分 -- 比同分參酌
						list = du->get_onsame (du);

						if ((j = misc->compare_list (
							list,
							du->dp->min_onsame,
						     MAXIMUM_ON_SAME_REFERENCE))
								< 0) {
							// printf ("[] ");
							j = -j;

							if (du->dp->
							   max_onsame_use < j) {
								du->dp->
								  max_onsame_use
								= j;
							}
#if ENABLE_POSTCHECK == 1
							pstck->fail_onsame (
							    student,
							    student->wish_index,
							    wish, j,
							    du->dp->
							    onsame[j-1]);
#endif
#if ENABLE_ONSAME_FAILECNT == 1
							student->onsame_failcnt
									++;
							student->
							   grp_onsame_failcnt[
							      du->dp->cstgrp]++;
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
							if (check_all_rules) {
								pstck->
							     write_resultcheck (
								student,
							    student->wish_index,
							    wish);
							}
#endif
							continue;
						}

#if ENABLE_POSTCHECK == 1
						pstck->challenge (
							student,
							student->wish_index,
							wish);
#endif
#if ENABLE_CHECKLIST == 1
						if (student->ck == NULL) {
#endif
#if ENABLE_CHALLENGE_CNT == 1
						    du->st->challenge_cnt++;
#endif
						    // 確定該生驗榜挑戰成功 ....
						    printf ("%d " WISHID_FMT
							" %d = %d [",
							du->st->sid, du->dp->id,
							score, minreq);

						    for (i = 0;
						      i <
						      MAXIMUM_ON_SAME_REFERENCE;
						      i++) {
							printf ("(%d,%d)",
								du->dp->
							 	min_onsame[i],
								list[i]);

							if (list[i] >
							  du->dp->min_onsame[i])
								break;
						    }

						    printf ("]\n");
#if ENABLE_CHECKLIST == 1
						}
#endif
					} else {
#if ENABLE_POSTCHECK == 1
						pstck->challenge_score (
							student,
							student->wish_index,
							wish, score, minreq);
#endif
#if ENABLE_CHECKLIST == 1
						if (student->ck == NULL) {
#endif
						// Challenge
							printf ("%d " WISHID_FMT
								" %d > %d\n",
								du->st->sid,
								du->dp->id,
								score, minreq);
#if ENABLE_CHECKLIST == 1
						}
#endif
					}

					// 已經找到適合的, 不再繼續了
					if (checkonly == 3) break;
					continue;
				}  // end if (check_only)

				if (du->try_accept (du)) {	// 試著錄取
#if DEBUG_LEVEL > 3
					int	poped = 0;

					dprint ("Accept student: %d "
						"on %o (%d), \r\n",
						student->sid,
						wish,
						student->wish_index);
#endif
					accept_count++;
					lostlist = du->lostlist (du);

					while ((student = lostlist->get ())
								!= NULL) {
#if DEBUG_LEVEL > 3
						poped++;
#endif
						pop_count++;
						st->push (student);

						du->dp->reject_cnt++;
						DO_LOGWISH(student->sid, "!-r");
#if DEBUG_LEVEL > 2
						if (trace_wishid == du->dp->id){
							printf ("Rej:"
								WISHID_FMT
								", %d (r)\n",
								trace_wishid,
								student->sid
								);
						}
#endif
					}
#if DEBUG_LEVEL > 3
					dprint ("s=%d, and %d poped\r\n",
							score, poped);
#endif

					// 跳出學生的志願 loop, 從 Stack 中找
					// 下個學生
					wish = 0; // 故意把 wish 放 0 以避免記錯
					break;
				} else {
					du->dp->reject_cnt++;
					DO_LOGWISH(student->sid,"!-x");
#if DEBUG_LEVEL > 2
					if (trace_wishid == du->dp->id) {
						printf ("REJ:" WISHID_FMT
							", %d (on same)\n",
							trace_wishid,
							du->st->sid);
					}
#endif
				}

#if ENABLE_DEPARTMENT_FIRST == 1
			} else if (selected_algorithm != 1) { // 學系榜演算法
				// 先求得每個志願的合格學生數
				valid_wish[st->get_wish_index(student)-1] = 1;
				du->inc_valid_student (du);
			}
#endif
		}

		if (wish == -1) lost_count++;

#if ENABLE_DEPARTMENT_FIRST == 1
		if (selected_algorithm != 1) {
			// 去掉無效的志願 ... 免得重覆檢定
			st->set_valid_wish_mask (student, valid_wish);
		}
#endif
	}

#if ENABLE_POSTCHECK == 1
	if (use_checklist > 1) {
		if (! st->prepare_sorted_stack (2)) return 1;

		while ((student = st->pop ()) != NULL) {
			pstck->start (student);
		}
	}
#endif

#if ENABLE_PRE_TESTING
	if (pretest_flag) exit (0);
#endif

#if ENABLE_DEPARTMENT_FIRST == 1
	while (selected_algorithm != 1) {
		// 先擴張名額錄取名額, 使同於有效志願名額
		dp->extend_space ();

		// 先配置每個學系的學生空間

		// 志願(系組)的名額空間
		if (! dp->allocate_space ()) return 1;

		// Sort 學生資料, 建成堆疊
		if (! st->prepare_sorted_stack (0)) return 1;

		print_memory ();

		// 重新讀一次學生的志願, 把成績算出來填好

		misc->print (PRINT_LEVEL_SYSTEM,
				"Calculate for all (%d) students ... ",
				st->number_of_students ());
		misc->timer_started ();

		while ((student = st->pop ()) != NULL) {
			// 學系榜 ... reset 學生的志願序

			st->reset_wish (student);
			du->bind (du, student);

#if NO_MULTIPLE_SELECT_ON_SAME_WISH == 1
			if (! wishtmp) dp->clear_use_log ();
#endif

			while ((wish = st->nextwish_with_order
						(student, &i)) != -1) {
				if (wish <= 0) continue;
				if (! du->load_wish_order (du, i)) continue;

				// 注意: 由於學生可能重複填寫相同志願多次,
				//       所以要設法踢除

				// 強迫錄取

#if NO_MULTIPLE_SELECT_ON_SAME_WISH == 1
				if (! wishtmp) {
					if (dp->test_and_set (du->dp->index)) {
						du->do_accept (du);
#  if DEBUG_LEVEL > 8
					} else {
						// 重覆志願
						printf ("%d, " WISHID_FMT "\n",
								du->st->sid,
								du->dp->id);
#  endif
					}
				} else {
					du->do_accept (du);
				}
#else
				du->do_accept (du);
#endif
			}
		}

		misc->print (PRINT_LEVEL_SYSTEM,
				"ok. (elapsed time: %.4f seconds)\n",
				misc->timer_ended ());

		st->free_stack ();


		// 對所有的名次表進行排序
		dp->sort_all_ranklist ();

		// [*] 此刻之前, 如果把整個 memory dump 出來,
		//     即可得到每個學生的每個有效志願成績清單
		if (fullwishtable) break;
		
		// 建立志願交插比對表
		dp->set_wish_cross_reference ();

		// 產生志願-成績暫時檔
		if (wishtmp) break;

#if ENABLE_PARALLEL_ALGORITHM == 1
		if (selected_algorithm == 3) {
			if (! pa_main (st, dp, du)) return 1;
			dp->re_arrange_ranklist ();
			break;
		}
#endif
		// ------- 以下是學系榜的準算法 ....

		// 將名額還原, 並在每個學系上畫上一條錄取線
		dp->fix_all_department_member ();

		// 把後面的那些志願四處去塗消
		dp->invalid_wishes_after ();

		// 都塞好了, 接下來就是要在名次表上遊走 ...

		misc->print (PRINT_LEVEL_SYSTEM,
				"Walking on ranklist table ... ");
		misc->timer_started ();
		
		// 一直嘗試去塗消, 直到沒有需要維止
		i = 0; while (dp->do_require_fix ()) i++;

		misc->print (PRINT_LEVEL_SYSTEM, 
			"ok.  [ %d time(s),  %.2f second(s) ]\r\n",
			i, misc->timer_ended ());

		// 看看是否要嘗試 rotate

		if (with_rotation) {
			/*
			misc->print (PRINT_LEVEL_SYSTEM,
				"Rotation Engine: Version %s\r\n",
				dp->rotation_engine_version ());
				*/

			dp->do_rotate ();
		}

		// 最後 ... 結束前的最後一刻 .... 把 array 重排一下

		dp->re_arrange_ranklist ();
		
		break;
	}
#endif

	misc->print (PRINT_LEVEL_SYSTEM, 
			"Dispatching elapsed time:%s %.2f %ssecond(s)\r\n\r\n",
			misc->color (COLOR_BLUE, COLOR_YELLOW),
			misc->timer_ended (), misc->reset_color ());


	if (post_check) {
		int	found_matched;

		misc->print (PRINT_LEVEL_SYSTEM, "Post checking in progress\n");
		if (! st->prepare_sorted_stack (presort)) return 1;

		while ((student = st->pop ()) != NULL) {
			if (student->wish_index > MAX_PER_STUDENT_WISH) {
				// 跳過落榜生 ...
				continue;
			}

			// printf ("%8d " WISHID_FMT " %d\n", student->sid,
			//		student->wish[student->wish_index - 1],
			//		student->wish_index);

			wish = student->wish[student->wish_index - 1];
			du->bind (du, student);
			du->load (du, wish);

			if (du->dp->take_sort <= 4) {
				// 錄取到丙案的學系
				continue;
			}
#if ENABLE_PRE_TESTING
			// pretest_flag = 1;
			if (du->pre_checkall (du, 1)) {
				// printf ("%d\n", student->sid);
				continue;
			}
#endif

			found_matched = 0;

			for (wish = dp->reset_wish (); wish != -1;
						wish = dp->next_wish ()) {
				du->load (du, wish);
				if (du->dp->take_sort > 4) continue;

				if (! du->check_sex     (du)) continue;
				if (! du->check_special (du)) continue;
				if (! du->check_skill   (du)) continue;
				if (! du->check_special (du)) continue;
				if (! du->check_basic   (du)) continue;
				if (! du->check_exam    (du)) continue;

				score = du->get_score (du);

				if (score < 0) continue;

				minreq = du->min_require (du);

				if (score < minreq) continue;

				if (score == minreq) {
					int32_t	*list;

					list = du->get_onsame (du);

					if (misc->compare_list (
						list, du->dp->min_onsame,
						MAXIMUM_ON_SAME_REFERENCE) < 0){

						continue;
					}
				}

				found_matched = 1;
				break;
			}

			if (found_matched == 0) {
				printf ("%8d\n", student->sid);
			}
		}
	}


#if ENABLE_PTHREAD == 1
	if ((selected_algorithm == 1) && (num_of_pthread <= 1)) {
#else
	if (selected_algorithm == 1) {
#endif
		// dp->write_student_matched_wish ();

		misc->print (PRINT_LEVEL_SYSTEM,
			"Dispatcher C=%lu, A=%lu, L=%lu, STK=%lu, Re=%lu\r\n",
			dispatcher_count, accept_count, lost_count,
			stkpop_count, pop_count);

		misc->print (PRINT_LEVEL_SYSTEM,
			"Accept=%d + Lost=%d = %d.  "
			"Error Wish:%lu, Sex:%lu, Skill:%lu\r\n",
			accept_count - pop_count, lost_count,
			accept_count - pop_count + lost_count,
			wrong_wish_count, sex_error, skill_error);
	}

	//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if ENABLE_CHALLENGE_CNT == 1
	if (checkonly) {
		int	fail = 0;
		int	ftotal = 0;
		int	lostfail = 0;
		int	lftotal = 0;
		int	fcnt[MAX_PER_STUDENT_WISH + 1];
		int	lfcnt[MAX_PER_STUDENT_WISH + 1];

		for (i = 0; i <= MAX_PER_STUDENT_WISH; i++) {
			fcnt[i] = lfcnt[i] = 0;
		}

		if (st->prepare_sorted_stack (0)) {
			while ((student = st->pop ()) != NULL) {

				fcnt[student->challenge_cnt]++;

				if (student->matched_wish == -1) {
					lfcnt[student->challenge_cnt]++;
				}

				if (student->challenge_cnt > 0) {
					if (student->matched_wish == -1) {
						lftotal +=
							student->challenge_cnt;
						lostfail++;
					}
					fail++;
					ftotal += student->challenge_cnt;
				}

				if (student->challenge_cnt > 3) {
					printf ("%2d %d\n",
							student->challenge_cnt,
							student->sid);
				}
			}
		}

		fprintf (stderr,
			"Challenge count = %d, %d, %d, %d, %3.2f, %3.2f\n",
			fail, lostfail,
			ftotal, lftotal,
			fail == 0 ? 0 :
				(double) ftotal  / (double) fail,
			lostfail == 0 ? 0 :
		       		(double) lftotal / (double) lostfail);

		for (i = 0; i <= MAX_PER_STUDENT_WISH; i++) {
			if ((fcnt[i] == 0) && (lfcnt[i] == 0)) continue;

			fprintf (stderr, "%3d. %6d  %6d\n",
							i, fcnt[i], lfcnt[i]);
		}
	}
#endif

#if ENABLE_ONSAME_FAILECNT == 1
	if (checkonly) {
		int	i, k;
		int	fail = 0;
		int	ftotal = 0;
		int	lostfail = 0;
		int	lftotal = 0;
		int	fcnt[MAX_PER_STUDENT_WISH + 1];
		int	lfcnt[MAX_PER_STUDENT_WISH + 1];
		int	gmfail[4] = { 0, 0, 0, 0 };
		int	gmlost[4] = { 0, 0, 0, 0 };
		int	gffail[4] = { 0, 0, 0, 0 };
		int	gflost[4] = { 0, 0, 0, 0 };

		for (i = 0; i <= MAX_PER_STUDENT_WISH; i++) {
			fcnt[i] = lfcnt[i] = 0;
		}

		if (st->prepare_sorted_stack (0)) {
			while ((student = st->pop ()) != NULL) {
				fcnt[student->onsame_failcnt]++;

				if (student->matched_wish == -1) {
					lfcnt[student->onsame_failcnt]++;
				}

				if (student->onsame_failcnt > 0) {
					if (student->matched_wish == -1) {
						lftotal +=
							student->onsame_failcnt;
						lostfail++;

						for (i = 0; i < 4; i++) {
							if ((k = student->
							   grp_onsame_failcnt[i]
							   ) > 0) {
								gmlost[i]++;
								gflost[i] += k;
							}
						}
					}
					fail++;
					ftotal += student->onsame_failcnt;

					for (i = 0; i < 4; i++) {
						if ((k = student->
						    grp_onsame_failcnt[i])> 0) {
							gmfail[i]++;
							gffail[i] += k;
						}
					}
				}

				if (student->onsame_failcnt >= 15) {
					printf ("%2d %d\n",
							student->onsame_failcnt,
							student->sid);
				}
			}
		}

		fprintf (stderr,
			"OnSame faile count = %d, %d, %d, %d, %3.2f, %3.2f\n",
			fail, lostfail,		// 人數
			ftotal, lftotal,	// 人次
			(double) ftotal  / (double) fail,
			(double) lftotal / (double) lostfail);

		for (i = 0; i < 4; i++) {
			fprintf (stderr, "[%d %d %d %d]",
					gmfail[i], gmlost[i],
					gffail[i], gflost[i]);
		}
		fprintf (stderr, "\n");

		for (i = 0; i <= MAX_PER_STUDENT_WISH; i++) {
			if ((fcnt[i] == 0) && (lfcnt[i] == 0)) continue;

			printf ("%3d. %6d  %6d\n", i, fcnt[i], lfcnt[i]);
		}
	}
#endif
	if (checkonly) {
		fp = NULL;
	} else {
		// 計算每系的最高、最低錄取原始分數 ...
		if (1) {
			for (wish = dp->reset_wish (); wish != -1;
						wish = dp->next_wish ()) {

				dp->calculate_statistic (wish);
			}
		}

		if ((output_file = sysconfig->getstr ("output-file")) != NULL) {
			if ((fp = fopen (output_file, "w")) == NULL) {
				misc->print (PRINT_LEVEL_SYSTEM,
						"%s: ", output_file);
				misc->perror (PRINT_LEVEL_SYSTEM);
			}
		}
	}


#if ENABLE_DEPARTMENT_FIRST == 1
	if (wishtmp && (selected_algorithm == 2)) {
		if (! st->prepare_sorted_stack (0)) return 1;

		while ((student = st->pop ()) != NULL) {
			st->reset_wish (student);
			du->bind (du, student);

			fprintf (fp, "%d", student->sid);

			while ((wish = st->nextwish_with_order
						(student, &i)) != -1) {
				if (wish < 0) {
					fprintf (fp, WISHID_FMT "%d%6d",
							-wish - 2, 0, 0);
#if ENABLE_DISPATCHER92 == 1
					fprintf (fp, "    0");
#endif
					continue;
				} else if (wish == 0) {
					fprintf (fp, WISHID_FMT "%d%6d",
							0, 0, 0);
#if ENABLE_DISPATCHER92 == 1
					if (use_disp92) fprintf (fp, "    0");
#endif
					continue;
				}

				if (! du->load_wish_order (du, i)) continue;

				fprintf (fp, WISHID_FMT "%d%6d", wish, 1,
					du->dp->stdlist[
					    du->st->order_on_wish[i]].score);
#if ENABLE_DISPATCHER92 == 1
				if (use_disp92) {
					if (du->dp->skillgroup != 0) {
						i = du->dp->stdlist[
							du->st->order_on_wish[
								i]].skill_score;
						if (i < 0) i = 0;

						fprintf (fp, "%5d",
							misc->toInt ((double) i
								/ 100.));
					} else {
						fprintf (fp, "    0");
					}
				}
#endif
			}
			fprintf (fp, "\r\n");
		}
	} else {
#endif
		if (output_form == 6) {
			st->print_all (fp);
		} else {
			for (wish = dp->reset_wish (); wish != -1;
						wish = dp->next_wish ()) {
				dp->print_list (fp, wish);
			}
		}
#if ENABLE_DEPARTMENT_FIRST == 1
	}
#endif

	if (fp != NULL) fclose (fp);

	if (wish_minimal) {
		fp = NULL;

		if ((output_file = sysconfig->getstr("wish-minimal")) != NULL) {
			if ((fp = fopen (output_file, "w")) == NULL) {
				misc->print (PRINT_LEVEL_SYSTEM,
						"%s: ", output_file);
				misc->perror (PRINT_LEVEL_SYSTEM);
			}
		} else {
			misc->print (PRINT_LEVEL_SYSTEM,
				"%% system variable (wish-minimal) "
				"not define!!\r\n");
		}

		if (wish_minimal == 4) wish_minimal = 0;

		if (fp != NULL) {
			for (wish = dp->reset_wish (); wish != -1;
						wish = dp->next_wish ()) {
				dp->print_minimal (fp, wish,
						wish_minimal, checkonly);
			}
			fclose (fp);
		}
	} else if (proto_wish != 0) {
		char	filename[256];

		if (proto_wish < 0) {
			sprintf (filename, "/tmp/%s", proto_crs_file);
		} else {
			sprintf (filename, "/tmp/" WISHID_FMT, proto_wish);
		}

		if ((fp = fopen (filename, "w")) != NULL) {
			for (wish = dp->reset_wish ();
					wish != -1; wish = dp->next_wish ()) {
				dp->print_minimal_proto (fp, wish, proto_wish,
						proto_weight,
						proto_course_weight);
			}
			fclose (fp);
		}
	} else if (! fullwishtable && ! wishtmp) {
		if ((fp = fopen ("/tmp/wish_minimal.txt", "w")) != NULL) {
			for (wish = dp->reset_wish ();
					wish != -1; wish = dp->next_wish ()) {
				dp->print_minimal_special (fp, wish);
			}
			fclose (fp);
		}
	}

#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
	if (check_all_rules) pstck->write_resultcheck (NULL, 0, 0);
#endif
#if ENABLE_POSTCHECK == 1
	if (mscboard >= 0) pstck->print_mscboard (mscboard);
#endif

	if (media_file > 0) pstck->write_media (media_file);

	if (x_option == 99) {
		/* 填寫志願數 */
		int	sum_in = 0;
		int	cnt_in = 0;
		int	sum_out = 0;
		int	cnt_out = 0;

		if (! st->prepare_sorted_stack (presort)) return 1;

		while ((student = st->pop ()) != NULL) {
			for (i = MAX_PER_STUDENT_WISH - 1; i >= 0; i--) {
				if (student->wish[i] != 0) {
					break;
				}
			}

			if (student->matched_wish > 0) {
				sum_in += i;
				cnt_in++;
			} else {
				sum_out += i;
				cnt_out++;
			}
		}

		printf ("%5.2f %5.2f %5.2f\n",
				(double) sum_in / (double) cnt_in,
				(double) sum_out / (double) cnt_out,
				(double) (sum_in + sum_out) /
				(double) (cnt_in + cnt_out));
	}

	misc->print (PRINT_LEVEL_SYSTEM, 
			"Total elapsed time: %.2f second(s)\r\n\r\n",
			misc->timer_ended ());

	if (logwishfp != NULL) fclose (logwishfp);
	if (cklfp     != NULL) fclose (cklfp);

	pstck->close ();


#if defined(x__BORLANDC__)
        fprintf (stderr, "\nPress any key to continue ...\n");
        getch ();
#endif

	//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	return 0;
}

void initial_global_variables (void) {
	int	i;

	for (i = 0; dcvlist[i].str != NULL; i++) {
		*dcvlist[i].list = sysconfig->strlist (dcvlist[i].str, NULL);
	}
}

void print_memory (void) {
	misc->print (PRINT_LEVEL_SYSTEM,
		"Total memory usage: %lu bytes [%s %.2f Mbytes %s]\r\n",
		misc->bytes_use (),
		misc->color (COLOR_BLUE, COLOR_YELLOW),
		(double ) misc->bytes_use () / 1024. / 1024.,
		misc->reset_color ());
}

static int  show_program_info (void) {
	fprintf (stderr,
		"Program version       : " VERSION "\r\n"
		"Support Algorithms    : [Student Optimal]"
#if ENABLE_DEPARTMENT_FIRST == 1
					" [Dept. Optimal]"
#endif
#if ENABLE_PARALLEL_ALGORITHM == 1
					" [Parallel]"
#endif
#if ENABLE_INCREMENTAL_ALGORITHM == 1
					" [IncDP]"
#endif
					"\r\n"
		"Pthread support       : "
#if ENABLE_PTHREAD == 1
					"enable\r\n"
#else
					"disable\r\n"
#endif
		"Debugging level       : %d\r\n"
		"Dispatcher 92 Format  : "
#if ENABLE_DISPATCHER92 == 1
#  if ENABLE_EXTEND_DISPATCHER92 == 1
					"enable (extended)\r\n"
#  else
					"enable\r\n"
#  endif
#else
					"disable\r\n"
#endif
		"CEEC format data      : "
#if ENABLE_CEEC_FORMAT
		"enable"
#  if ENABLE_CEEC_MERGE_SN == 1
		" (use pre-merge serial number)"
#  else
		" (use per-test id)"
#  endif
#else
		"disable"
#endif
		"\r\n"
		"Wish Multiple select  : "
#if NO_MULTIPLE_SELECT_ON_SAME_WISH == 1
					"removed\r\n"
#else
					"allow\r\n"
#endif
		"Deny list             : "
#if ENABLE_DENYLIST == 1
					"enable\r\n"
#else
					"disable\r\n"
#endif
		"Check list            : "
#if ENABLE_CHECKLIST == 1
					"enable\r\n"
#else
					"disable\r\n"
#endif
		"Pre-dispatching test  : "
#if ENABLE_PRE_TESTING == 1
					"enable\r\n"
#else
					"disable\r\n"
#endif
		"Dull Elephant's Option: "
#if DULL_ELEPHANT_OPTION == 1
					"Yes (enable)\r\n"
#else
					"No (disable)\r\n"
#endif
		"Miscellaneous         : "
					"skz="
#if CARE_ABOUT_SKILL_MISS_OR_ZERO == 1
					"1"
#else
					"0"
#endif
					", nuc="
#if DO_NOT_CARE_ABOUT_SCORE_ON_NO_USED == 1
					"0"
#else
					"1"
#endif
					", igcos="
#if IGNORE_CASE_ON_SPECIAL == 1
					"1"
#else
					"0"
#endif
					"\r\n"
		"Maximum student wishes: %d\r\n"
		"Build date            : " __DATE__ ", " __TIME__ "\r\n"
		"Using GNU C Compiler  : "
#ifdef __GNUC__
					"GNU C Compiler Version %d.%d\r\n"
#else
					"no\r\n"
#endif
		"Author                : Jiann-Ching Liu\r\n"
		"Author\'s Email        : center5@cc.ncu.edu.tw\r\n"
		"\r\n",
		DEBUG_LEVEL,
		MAX_PER_STUDENT_WISH
#ifdef __GNUC__
		,
		__GNUC__, __GNUC_MINOR__
#endif
		);
	return 1;
}

#if ENABLE_PTHREAD == 1

static struct student_t	*mp_stkpop (void) {
	struct student_t	*student;

	PTHREAD_MUTEX_LOCK   (&stk_mutex);

	student = st->pop ();

	PTHREAD_MUTEX_UNLOCK (&stk_mutex);

	return student;
}

static void initial_for_multithreading (void) {
}

static void mp_stk_consumer (int *thrid) {
	struct dispatcher_unit_t	*du = NULL;
	struct student_t		*student;
	int				cnt = 0;
	int				ok;
	int32_t				wish;
	struct lost_student_list_t	*lostlist;

	// fprintf (stderr, "Consumer: %lu\n", pthread_self ());

	if ((du = allocate_dispatcher_unit (dp, st)) == NULL) {
		pthread_exit (NULL);
	}

	while ((student = mp_stkpop ()) != NULL) {
		if ((student->sflag & STUDENT_FLAG_DISALLOW) != 0) continue;

		/*
		fprintf (stderr, "Consumer[%lu]: %d, try %d\n",
				pthread_self (), cnt,
				student->sid);
				*/
		// PTHREAD_MUTEX_LOCK (&bsh_mutex); 

		du->bind (du, student);	// 怕 bsearch 不是 pthread self
					// 所以要 lock
		// PTHREAD_MUTEX_UNLOCK (&bsh_mutex); 

		while ((wish = st->nextwish (student)) != -1) {
			if (wish <= 0) continue;

			// PTHREAD_MUTEX_LOCK (&bsh_mutex); 
			ok = du->load (du, wish);
			// PTHREAD_MUTEX_UNLOCK (&bsh_mutex); 

			if (! ok) continue;

			if (! du->check_sex (du)) continue;
			if (! du->check_skill (du)) continue;
			if (! du->check_basic (du)) continue;
			if (! du->check_exam (du)) continue;
			if (du->get_score (du) < 0) continue;

			PTHREAD_MUTEX_LOCK (&du->dp->mutex);
			// PTHREAD_MUTEX_LOCK (&lst_mutex);
			ok = du->try_accept (du);
			// PTHREAD_MUTEX_UNLOCK (&lst_mutex);
			PTHREAD_MUTEX_UNLOCK (&du->dp->mutex);

			if (ok) {	// 試著錄取
				PTHREAD_MUTEX_LOCK (&stk_mutex); 

				lostlist = du->lostlist (du);

				while ((student = lostlist->get ()) != NULL) {
					st->push (student);
				}

				PTHREAD_MUTEX_UNLOCK (&stk_mutex); 
				wish = 0; // 故意把 wish 放 0 以避免記錯
				break;
			}
		}

		cnt++;
	}

	fprintf (stderr, "Thread [%d]: %d\n", *thrid, cnt);

	pthread_exit (NULL);
}
#endif
