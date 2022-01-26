/*
 *	department.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "predefine.h"

#if ENABLE_PTHREAD == 1
#include <pthread.h>
#endif 

#include "department.h"
#include "global_var.h"
#include "student.h"
#include "studlist.h"
#include "misclib.h"
#include "textio.h"
#include "sys_conf.h"
#include "skillset.h"
#include "rotation.h"
#include "dtstruct.h"

// #define DEPARTMENT_ID_LENGTH		4
#define DEPARTMENT_NUMBER_LENGTH	3
#define MAX_SKILL_SORT			9


struct department_indexes_t {
	int	id;
	int	idx;
};

#if ENABLE_DEPARTMENT_FIRST == 1
/*
struct admirer_chain_t {
	int	depidx;
	int	stdidx;
	int	admirer_flag;
//	int	srcdep;
//	int	srcstd;
};
*/

struct rotation_algorithm_list_t {
	struct rotation_algorithm_t		*algorithm;
	struct rotation_algorithm_list_t	*next;
};

static struct rotation_algorithm_list_t		*rotalglist = NULL;
#endif

////////////////////////////////////////////////////////////////////

static struct university_t		*univ = NULL;
static struct textfile_io_t		*tio;
static struct department_t		*depbuf = NULL;
static int				depnum = 0;
static struct department_indexes_t	*depsortedindex = NULL;
static struct lost_student_list_t	*lslptr = NULL;

static char				*userdefine_format =
						"$w, $s,  $o, $t\n";

static char			*basic_standard_name[] = {
					"無", "頂", "前", "均", "後", "底"
				};

static char			*exam_weight_name[] = {
					"0",
					"1.00", "1.25", "1.50", "1.75", "2.00"
				};

static char			*exam_standard_name[] = {
					"無", "高", "均", "低"
				};

static char			*exam_test_cname[] = {
					"無",
					"一般檢定 (一)",
					"一般檢定 (二)",
					"一般檢定 (三)",
					"一般檢定 (四)"
				};

static char			*skill_test_cname[] = {
					"無",
					"音樂",
					"國樂",
					"體育",
					"舞蹈",
					"美術",
					"國劇",
					"戲劇"
				};


////////////////////////////////////////////////////////////////////

static int				skill_bitmap[MAX_SKILL_SORT][64];
static int				skill_bitmap_idx[MAX_SKILL_SORT];

////////////////////////////////////////////////////////////////////

static int			load_limits	(void);
static int			load_depna	(void);
static int			load_skillset	(void);
static int			load_wishminimal(void);
static int			allocate_space	(void);
static int			sort_students	(struct department_t *dp);
static struct department_t *	find_by_id	(const int16_t);
static struct department_t *	find_by_index	(const int);
static int16_t			reset_wish	(void);
static int16_t			next_wish	(void);
static int			print_list	(FILE *,const int16_t);
static int			print_minimal_proto (FILE *, const int16_t,
							const int, const int,
							const int8_t *);
static int			print_minimal_special (FILE *, const int16_t);
static int			print_minimal(FILE *,const int16_t,
							const int, const int);
static int			analyze_deplim_group (void);
static int			search_department (const int wish);
static int64_t			course_to_bitmap (const int skst,
							const int crs);
static int64_t			skill_bitmapfcn (const int skst, const int crs);
// static void			write_student_matched_wish(void);
#if ENABLE_DEPARTMENT_FIRST == 1
static int			do_require_fix		 (void);
static int			do_rotate		 (void);
#  if NO_MULTIPLE_SELECT_ON_SAME_WISH == 1
static void			clear_use_log		 (void);
static int			test_and_set		 (const int idx);
#  endif
static void			re_arrange_ranklist	 (void);
static void			extend_space	         (void);
static void			invalid_wishes_after     (void);
static void			set_wish_cross_reference (void);
static void			sort_all_ranklist	 (void);
static void			fix_all_department_member(void);
//static const char *		rotation_engine_version  (void);
static void			calculate_statistic	 (const int16_t idx);
static int			order_by_did (
					const struct department_indexes_t *i,
					const struct department_indexes_t *j);


static char			*use_log = NULL;
#endif

static int			output_format = 0;
static int			use_onsame = 0;

#if ENABLE_DR_LEE_SPECIAL == 1
static int	dr_lee_flag = 0;
int		dr_lee_rjstcnt = 0;
int32_t		dr_lee_studid;
int16_t		dr_lee_wishid;
extern int 	finding_reject_possible	(void);
#endif

#if ENABLE_DISPATCHER92 == 1
#include "code92/dept92.c"
#endif

////////////////////////////////////////////////////////////////////

#if ENABLE_PARALLEL_ALGORITHM == 1
static struct department_t * get_depbuf (void) { return depbuf; }
static int  number_of_departments (void) { return depnum; }
#endif
static int compare_order (struct student_on_wish_t *st1,
					struct student_on_wish_t *st2);

struct department_module_t * initial_department_module (const int outform) {
	static struct department_module_t	dpm;

	tio = initial_textfile_io_module ();
	lslptr = initial_lostlist_module ();

#if ENABLE_DISPATCHER92 == 1
	if (use_disp92) {
		dpm.load_limits		= load_limits92;
		dpm.load_skillset	= load_skillset92;
	} else {
		dpm.load_limits		= load_limits;
		dpm.load_skillset	= load_skillset;
	}
#else
	dpm.load_limits			= load_limits;
	dpm.load_skillset		= load_skillset;
#endif
	dpm.analy_deplim_grp		= analyze_deplim_group;
	dpm.load_depna			= load_depna;
	dpm.load_wishminimal		= load_wishminimal;
	dpm.allocate_space		= allocate_space;
	dpm.find			= find_by_id;
	dpm.find_index			= find_by_index;
	dpm.sort_students		= sort_students;
	dpm.reset_wish			= reset_wish;
	dpm.next_wish			= next_wish;
	dpm.print_list			= print_list;
	dpm.print_minimal		= print_minimal;
	dpm.print_minimal_special	= print_minimal_special;
	dpm.print_minimal_proto		= print_minimal_proto;
	dpm.skill_bitmap		= skill_bitmapfcn;
	dpm.calculate_statistic		= calculate_statistic;
//	dpm.write_student_matched_wish	= write_student_matched_wish;
#if ENABLE_PARALLEL_ALGORITHM == 1
	dpm.get_depbuf			= get_depbuf;
	dpm.number_of_departments	= number_of_departments;
	dpm.compare_order		= compare_order;
#endif
#if ENABLE_DEPARTMENT_FIRST == 1
	dpm.do_require_fix		= do_require_fix;
	dpm.do_rotate			= do_rotate;
#  if NO_MULTIPLE_SELECT_ON_SAME_WISH == 1
	dpm.clear_use_log		= clear_use_log;
	dpm.test_and_set		= test_and_set;
#  endif
	dpm.re_arrange_ranklist		= re_arrange_ranklist;
	dpm.extend_space		= extend_space;
	dpm.invalid_wishes_after	= invalid_wishes_after;
	dpm.set_wish_cross_reference	= set_wish_cross_reference;
	dpm.sort_all_ranklist		= sort_all_ranklist;
	dpm.fix_all_department_member	= fix_all_department_member;
	//dpm.rotation_engine_version	= rotation_engine_version;
#endif

	if ((output_format = outform) == 0) {
		char	*ptr;

		if ((ptr = sysconfig->getstr ("user-define-format")) != NULL) {
			userdefine_format = ptr;
		}
	}

	/*
	if (outform == 0) {
		if ((output_format = sysconfig->getint ("output-format")) <= 0){
			output_format = 1;
		}
	} else {
		output_format = outform;
	}
	*/

	return &dpm;
}

////////////////////////////////////////////////////////////////////

static int compare_order (struct student_on_wish_t *st1,
					struct student_on_wish_t *st2) {
	int	i;

	use_onsame = 0;

	// 請注意, 由於是要用 reverse order (成績高的在前)
	// 所以勒, 正負號相反

	if (st1->score < st2->score) {
		return 1;
	} else if (st1->score > st2->score) {
		return -1;
	}

	// 同分參酌

	for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
		use_onsame++;

		if (st1->onsame[i] < st2->onsame[i]) {
			return 1;
		} else if (st1->onsame[i] > st2->onsame[i]) {
			return -1;
		}
	}

	return 0;
}

static int sort_students (struct department_t *dp) {
	int			i, j, k;
	int			order;
	int			addorder  = 1;	// 先假定需要增加名次
	int			retval    = 0;
	int			lastorder;	// 最後一名的名次
	int			myorder = 0;
	//int			onsame_cnt = 0;
	struct student_list_t	*sl;
#if DEBUG_LEVEL > 2
	int			debug_on = trace_wishid;
#endif

	// 只使用在學生榜上, 其實是使用 Insertion sort ....
	//
	// 這個演算法有點小慢 .....

	// 注意原則 ... 只要能塞得下, 就不折
	// 一但折了之後, 要出去就一起出去, 沒有折了之後再塞的事 ...

	k = dp->lastidx;

#if DEBUG_LEVEL > 2
	if (dp->id == debug_on) {
		dprint ("### Wish (%o), Min=%d, Num=%d/%d, Same=%d\n",
			dp->id, dp->minreq, dp->inlistcnt - 1, dp->num,
			dp->llindex);
		dprint ("Incoming: Sid = %8d, Score=%d, TS=%d",
				dp->stdlist[k].st->sid,
				dp->stdlist[k].score,
				dp->stdlist[k].onsame[0]);

		for (i = 1; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
			dprint (",%d", dp->stdlist[k].onsame[i]);
		}
		dprint ("\n");
	}
#endif

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

#if DEBUG_LEVEL > 2
	if (dp->id == debug_on) dprint ("Insert to order %d\n", myorder);
#endif

	for (j = k - 1; j > i; j--) {
		// 往後推
		memcpy (&dp->stdlist[j], &dp->stdlist[j-1],
				sizeof (struct student_on_wish_t));
		// 看看要不要增加名次
		if (addorder) dp->stdlist[j].order++;
	}

	// 真正塞入位置
	memcpy (&dp->stdlist[i], &dp->stdlist[k],
					sizeof (struct student_on_wish_t));

	// 看看名額限制 ...
	
	if (dp->inlistcnt  < dp->num) return 1;	// 名額未滿
	if (dp->inlistcnt == dp->num) {		// 名額剛好 ... 寫最低錄取分
		dp->minreq      = dp->stdlist[k-1].score;
		/*
		dp->used_onsame = 0;

		for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
			dp->min_onsame[i] = 0;
		}
		*/
#if DEBUG_LEVEL > 7
		dprintw ("Just full ... set Min-Requirement = %d\n",
				dp->minreq);
#endif
#if DEBUG_LEVEL > 2
		if (dp->id == debug_on) {
			dprint ("Just full ... set Min-Requirement = %d\n",
						dp->minreq);
		}
#endif
		return 1;
	}

	// 她沒有比別人高分, 又沒有和別人同分, 名額又滿了
	if (! retval) {
		dp->lastidx--;
		dp->inlistcnt--;
#if DEBUG_LEVEL > 3
		dprint ("Not accept: min = %d, only = %d\n",
				dp->minreq, dp->stdlist[k].score);
		// if (dp->id == debug_on) dprintw ("");
#endif
#if DEBUG_LEVEL > 3
		if (dp->id == debug_on) {
			dprint ("Not accept: min = %d, only = %d\n",
				dp->minreq, dp->stdlist[k].score);
		}
#endif

		if (dp->max_onsame_use < use_onsame) {
			dp->max_onsame_use= use_onsame;
			dp->fail_onsame_sid = dp->stdlist[k].st->sid;
		}
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
			sl->next = dp->lastlist;
			sl->st   = dp->stdlist[--dp->lastidx].st;
			dp->lastlist = sl;
			dp->llindex++;	// 在 lastlist 的人數
					// array 使用的指標
					// 降回

#if DEBUG_LEVEL > 2
			if (dp->id == debug_on) {
				dprint ("Just Accept\n");
			}
#endif
#if DEBUG_LEVEL > 7
			dprintw ("Just Accept\n");
		        if (dp->id == debug_on) dprintw ("");
#endif
			return 1;	// 暫時入選
		} else if (k - 1 >= dp->num) {

			// 踢掉一堆同分最後一名的人
			// -- 同分最後一名的代表 user
			sl = allocate_student_list (1);
			sl->next = dp->lastlist;
			sl->st   = dp->stdlist[--dp->lastidx].st;

			// [?] 重算一下同分參酌到第幾序
			order = compare_order (
					&dp->stdlist[dp->lastidx - 1],
					&dp->stdlist[dp->lastidx]);

			dp->max_onsame_use  = use_onsame;
			dp->fail_onsame_sid = dp->stdlist[dp->lastidx].st->sid;

			dp->inlistcnt = dp->inlistcnt - dp->llindex - 1;
			// 將 lastlist 丟回 queue
			// lslptr->put_list (dp->lastlist);
			lslptr->put_list (sl);
			dp->lastlist = NULL;
			dp->llindex = 0;
			// ??
			dp->minreq = dp->stdlist[dp->lastidx-1].score;
			
#if DEBUG_LEVEL > 7
			dprint ("Accept and kick a list\n");
		        // if (dp->id == debug_on) dprintw ("");
#endif
#if DEBUG_LEVEL > 2
			if (dp->id == debug_on) {
				dprint ("Accept and kick a list\n");
			}
#endif
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
			lslptr->put (dp->stdlist[--dp->lastidx].st);
			// 把這個人丟回 queue

			// [?] 重算一下同分參酌到第幾序
			order = compare_order (
					&dp->stdlist[dp->lastidx - 1],
					&dp->stdlist[dp->lastidx]);

			dp->max_onsame_use = use_onsame;
			dp->fail_onsame_sid = dp->stdlist[dp->lastidx].st->sid;

			// ??
			dp->minreq = dp->stdlist[dp->lastidx-1].score;
#if DEBUG_LEVEL > 7
			dprint ("Accept and kick one, min = %d\n",
					dp->minreq);
		        if (dp->id == debug_on) dprintw ("");
#endif
#if DEBUG_LEVEL > 2
			if (dp->id == debug_on) {
				dprint ("Accept and kick one\n");
			}
#endif
			return 2;
		} else {
			// 把同名次的這堆人折起來

			dp->lastlist = sl = allocate_student_list (j - 1);

			while (sl != NULL) {
				sl->st   = dp->stdlist[--dp->lastidx].st;
				sl = sl->next;
				dp->llindex++;
			}

			// ??
			dp->minreq = dp->stdlist[dp->lastidx-1].score;
#if DEBUG_LEVEL > 7
			dprintw ("Accept and fold, min = %d\n", dp->minreq);
		   //     if (dp->id == debug_on) dprintw ("");
#endif
#if DEBUG_LEVEL > 2
			if (dp->id == debug_on) {
				int	i;
				struct student_list_t	*sl;

				i = dp->lastidx - 1;
				dprint ("Accept and fold [%d",
						dp->stdlist[i].st->sid);
				sl = dp->lastlist;

				while (sl != NULL) {
					dprint (",%d", sl->st->sid);
					sl = sl->next;
				}
				dprint ("]\n");
			}
#endif
			return 1;
		}
	}
	// return retval;
}

static int order_by_did (const struct department_indexes_t *i,
				const struct department_indexes_t *j) {
	if (i->id < j->id) {
		return -1;
	} else if (i->id > j->id) {
		return 1;
	} else {
		return 0;
	}
}

static int64_t course_to_bitmap (const int skst, const int crs) {
	int		i;
	int64_t		bm;

	if (skst >= MAX_SKILL_SORT - 1) return -1;

	bm = 1;
	for (i = 0; i < skill_bitmap_idx[skst]; i++) {
		if (skill_bitmap[skst][i] == crs) return bm;
		bm <<= 1;
	}

	if (skill_bitmap_idx[skst] < 62) {
		skill_bitmap[skst][i] = crs;
		skill_bitmap_idx[skst] = i + 1;
		return bm;
	} else {
		return -1;
	}
}

static int64_t skill_bitmapfcn (const int skst, const int crs) {
	int		i;
	int64_t		bm;

	if (skst >= MAX_SKILL_SORT - 1) return 0;

	for (i = 0, bm = 1; i < skill_bitmap_idx[skst]; i++) {
		if (skill_bitmap[skst][i] == crs) return bm;
		bm <<= 1;
	}

	return 0;
}

static int load_skillset (void) {
	char			*filename;
	int			i, j, k, len, err, depid;
	int			majorn, minorn, crs;
	int64_t			major, minor;
	int64_t			bitmap;
	int			count = 0;
	struct skillset_t	*skptr;
	struct ncu_skillset	*ptr;
	int			have_error;

	if ((filename = sysconfig->getstr ("skill-set-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (skill-set-file)"
			" not define!!\r\n");
		return 0;
	}

	for (i = 0; i < MAX_SKILL_SORT; i++) skill_bitmap_idx[i] = 0;

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load skill-set file [ %s ] ... ", filename);

	if (tio->open (filename)) {
		have_error = count = 0;

		while ((len = tio->read ((char **) &ptr)) >= 0) {
			if (len < sizeof (struct ncu_skillset)) continue;
			count++;
			err = 0;

			depid = misc->oct (ptr->wishid, DEPARTMENT_ID_LENGTH);

			if ((i = search_department (depid)) == -1) {
				have_error = 1;
				misc->print (PRINT_LEVEL_INFO,
						"[" WISHID_FMT "]", depid);
				continue;
			}

			majorn = misc->dec (ptr->major, 2);
			minorn = misc->dec (ptr->minor, 2);

			if ((majorn == 0) && (minorn == 0)) continue;

			major = minor = 0;

			for (k = j = 0; k < majorn; k++, j++) {
				crs = misc->dec (ptr->code[j], 4);

				bitmap = course_to_bitmap (
						depbuf[i].skillgroup, crs);

				if (bitmap == -1) {
					err = 1;
					break;
				}

				// dprint ("%04d - %llu\n", crs, bitmap);

				major |= bitmap;
			}

			for (k = 0; k < minorn; k++, j++) {
				crs = misc->dec (ptr->code[j], 4);

				bitmap = course_to_bitmap (
						depbuf[i].skillgroup, crs);

				if (bitmap == -1) {
					err = 1;
					break;
				}

				// dprint ("%04d - %llu\n", crs, bitmap);

				minor |= bitmap;
			}

			if (err) {
				have_error = 1;
				continue;
			}

			// allocate space for struct skillset_t
			skptr = misc->malloc (sizeof (struct skillset_t));

			if (skptr == NULL) {
				have_error = 1;
				continue;
			}

			skptr->major = major;
			skptr->minor = minor;
			skptr->next  = depbuf[i].skillset;

			depbuf[i].skillset = skptr;
		}

		if (have_error) {
			misc->print (PRINT_LEVEL_SYSTEM, "error\r\n");
			return 0;
		} else {
			misc->print (PRINT_LEVEL_INFO,
					"%lu line(s) read\r\n", count);
		}
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	return 1;
}

static int load_wishminimal (void) {
	char			*filename;
	int			i, j, depid, len;
	struct wishminimal_t	*ptr;
	int			count;
	int			err = 0;

	if ((filename = sysconfig->getstr ("wishminimal-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (wishminimal-file)"
			" not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load wishminimal file [ %s ] ... ", filename);

	if (tio->open (filename)) {
		count = 0;
		while ((len = tio->read ((char **) &ptr)) >= 0) {
			if (len < sizeof (struct wishminimal_t)) continue;

			depid = misc->oct (ptr->wishid, DEPARTMENT_ID_LENGTH);

			if ((i = search_department (depid)) == -1) {
				err = 1;
				misc->print (PRINT_LEVEL_INFO,
						"[" WISHID_FMT "]", depid);
				continue;
			}

			depbuf[i].minreq = misc->dec (ptr->score, 6);

			for (j = 0; j < MAXIMUM_ON_SAME_REFERENCE; j++) {
				depbuf[i].min_onsame[j] =
					misc->dec (ptr->onsame[j], 5);

				if (depbuf[i].onsame[j] == 2) {
					depbuf[i].min_onsame[j] =
						MAX_PER_STUDENT_WISH -
						depbuf[i].min_onsame[j];
				}
			}

			depbuf[i].max_onsame_use = misc->dec (ptr->use, 1);

			count++;
		}

		tio->close ();
		misc->print (PRINT_LEVEL_INFO, "%lu line(s) read\r\n", count);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	return 1;
}

static int load_limits (void) {
	char			*filename;
	int			i, k, idx, len;
#if SKILL_WEIGHT_ON_MAIN_WEIGHT == 1
	double			ccnt;
	double			multiply[] = { 0.0, 1.0, 1.25, 1.5, 1.75, 2.0 };
#else
	int			ccnt;
#endif
	int			crsnum_cnt;
	int			count = 0;
	struct ncu_deplim	*ptr;

	if ((filename = sysconfig->getstr ("department-limit-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (department-limit-file)"
			" not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load department limit file [ %s ] ... ", filename);

	if (tio->open (filename)) {
		count = 0;
		while ((len = tio->read ((char **) &ptr)) >= 0) {
			if (len < sizeof (struct ncu_deplim)) continue;
			count++;
		}

		tio->close ();
		misc->print (PRINT_LEVEL_INFO, "%lu [ pass one ]\r\n", count);
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_INFO, "Allocate %lu memory buffers ... ",
			count * sizeof (struct department_t));

	if ((depbuf = misc->calloc (count,
				sizeof (struct department_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	} else {
		depnum = count;
		misc->print (PRINT_LEVEL_INFO, "ok\r\n");
	}

#if ENABLE_DEPARTMENT_FIRST == 1
	if (selected_algorithm != 1) {
		if (use_log == NULL) {
			use_log = misc->malloc (depnum * sizeof (char));

			if (use_log == NULL) {
				misc->perror (PRINT_LEVEL_SYSTEM);
				return 0;
			}
		}
	}
#endif

	misc->print (PRINT_LEVEL_INFO,
			"Load department limit into memory  ... ");

	if (tio->open (filename)) {
		for (idx = 0; (len = tio->read ((char **) &ptr)) >= 0; ){
			if (len < sizeof (struct ncu_deplim)) continue;

			depbuf[idx].id = misc->oct (ptr->wishid,
						DEPARTMENT_ID_LENGTH);
			depbuf[idx].num = misc->dec (ptr->num,
						DEPARTMENT_NUMBER_LENGTH);

#if ENABLE_DEPARTMENT_FIRST == 1
			depbuf[idx].real_num = depbuf[idx].num;
#endif

			depbuf[idx].cname = NULL;
			depbuf[idx].univ = NULL;

			depbuf[idx].sex    = misc->dec (&ptr->sex, 1);
			depbuf[idx].check1 = misc->dec (
						&ptr->grade_check, 1);

#if DEBUG_LEVEL > 8
			if (depbuf[idx].id == 0706) {
				dprintw (WISHID_FMT " - sex = %d\r\n",
						depbuf[idx].id,
						depbuf[idx].sex);
			}
#endif

			for (i = 0; i < 5; i++) {
				depbuf[idx].test1[i] =
					misc->dec (&ptr->grade_std[i], 1);
			}

			depbuf[idx].check2 =
					misc->dec (&ptr->exam_check, 1);
#if DEBUG_LEVEL > 0
			// dprint ("%d\r\n", depbuf[idx].check2);
#endif
			depbuf[idx].checkskill =
				misc->dec (&ptr->skill_check, 1);

			for (i = k = 0; i < 9; i++) {
				if ((depbuf[idx].test2[i] =
					misc->dec (&ptr->exam_std[i], 1))
						!= 0) {
					k++;
				}
			}

#if SKILL_WEIGHT_ON_MAIN_WEIGHT == 1
			ccnt = 0.0;
#else
			ccnt = 0;
#endif
			crsnum_cnt = 0;

			for (i = 0; i < 9; i++) {
				if ((k = misc->dec
					    (&ptr->exam_use[i], 1)) > 0) {
#if SKILL_WEIGHT_ON_MAIN_WEIGHT == 1
					if (skill_relative_weight) {
						ccnt += multiply[k];
					} else {
						ccnt++;
					}
#else
					ccnt++;
#endif
					crsnum_cnt++;
				}
				depbuf[idx].weight[i] = k;
			}

			depbuf[idx].crsnum_cnt = crsnum_cnt;
			depbuf[idx].ccnt = ccnt;	// 算採計科目數

			depbuf[idx].skillweight =
					misc->dec (&ptr->skillweight, 1);

			depbuf[idx].skillgroup  =
					misc->dec (&ptr->skillgroup, 1);
			depbuf[idx].skillway    =
					misc->dec (&ptr->skill_how, 1);
			depbuf[idx].skill_main  =
					misc->dec (ptr->skill_main, 2);
			depbuf[idx].skill_skill =
					misc->dec (ptr->skill_skill, 2);

			depbuf[idx].skill_major = misc->dec (ptr->major, 4);
			depbuf[idx].special  = misc->dec (&ptr->special, 1);

			depbuf[idx].skillset = NULL;
			depbuf[idx].max_onsame_use = 0;
			depbuf[idx].fail_onsame_sid = 0;

			depbuf[idx].max_orgsum   = 0;
			depbuf[idx].min_orgsum   = 0;
			depbuf[idx].min_orgsum_n = 0;
			depbuf[idx].avg_orgsum_n = 0;

			depbuf[idx].deplim_grp       = -1;
			depbuf[idx].number_of_refcrs = 0;
#if DEBUG_LEVEL > 9
			dprint ("on same: [");
#endif
			for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
#if DEBUG_LEVEL > 9
				dprint ("%c", buffer[j]);
#endif
				depbuf[idx].onsame[i] = (ptr->onsame[i] - 'A');
			}
#if DEBUG_LEVEL > 9
			dprint ("]\r\n");
#endif

			// printf ("%s\r\n", buffer);

			depbuf[idx].minreq    = 0;
			depbuf[idx].inlistcnt = 0;
			depbuf[idx].lastidx   = 0;
			depbuf[idx].llindex   = 0;
			depbuf[idx].stdlist   = NULL;
			depbuf[idx].lastlist  = NULL;
			depbuf[idx].rflag     = 0;
			depbuf[idx].reject_cnt		= 0;
			depbuf[idx].n_qualify_cnt	= 0;
#if ENABLE_DEPARTMENT_FIRST == 1
			depbuf[idx].index     = idx;
			depbuf[idx].student_cnt = 0;
#endif
#if ENABLE_PTHREAD == 1
			pthread_mutex_init (&depbuf[idx].mutex, NULL);
#endif
			for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
				depbuf[idx].min_onsame[i] = 0;
			}

			// depbuf[idx].used_onsame = 0;

			if (depbuf[idx].check2 > 0) {
				// 丙 (一二三四)
				depbuf[idx].take_sort = depbuf[idx].check2;
				depbuf[idx].cstgrp = 3;
			} else {
				if (depbuf[idx].onsame[3] == 0) {
					// 乙案
					depbuf[idx].take_sort = 6;
					depbuf[idx].cstgrp = 2;
				} else {
					// 甲案
					depbuf[idx].take_sort = 5;
					depbuf[idx].cstgrp = 1;

					if (crsnum_cnt == 0) {
						depbuf[idx].cstgrp = 0;
					}
				}
			}

#if DEBUG_LEVEL > 1
			if (trace_wishid == depbuf[idx].id) {
				misc->print (PRINT_LEVEL_DEBUG,
						"Sex(%d)",
						depbuf[idx].sex);
				for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++){
					misc->print (PRINT_LEVEL_DEBUG,
						"(%s)",
						dpg_onsame_crs[
							depbuf[idx].onsame[i]]);
				}
				misc->print (PRINT_LEVEL_DEBUG, "\r\n");
			}
#endif

			if (logwish == depbuf[idx].id) {
				int	i;

				fprintf (logwishfp,
					"#志願號      : " WISHID_FMT "\n"
					"#名額        : %d\n"
					"#性別限制    : %s\n"
					"#學測一般檢定: %s\n"
					"#學測標準    :",
					depbuf[idx].id,
					depbuf[idx].num,
					((depbuf[idx].sex == 0) ? "無" :
					((depbuf[idx].sex == 1) ? "男" :
					"女")),
					(depbuf[idx].check1 ? "Y" : "N")
				);

				for (i = 0; i < 5; i++) {
					fprintf (logwishfp, "   %s:[%s]",
						dpg_basic_crs[i],
						basic_standard_name[
						depbuf[idx].test1[i]]
					);
				}

				fprintf (logwishfp,
					"\n"
					"#(丙)一般檢定: %s\n"
					"#術科檢定    : %s\n"
					"#指定科目標準:",
					exam_test_cname[depbuf[idx].check2],
					skill_test_cname[
						depbuf[idx].checkskill]
				);

				for (i = 0; i < 9; i++) {
					if (depbuf[idx].test2[i] != 0) {
						fprintf (logwishfp,
							"   %s:[%s]",
							dpg_exam_crs[i],
							exam_standard_name[
							depbuf[idx].test2[i]]
						);
					}
				}

				fprintf (logwishfp,
					"\n"
					"#採計        : "
				);

				for (i = 0; i < 9; i++) {
					if (depbuf[idx].weight[i] != 0) {
						fprintf (logwishfp,
							"  %sx%s",
							dpg_exam_crs[i],
							exam_weight_name[
							depbuf[idx].weight[i]]
						);
					}
				}

				if (depbuf[idx].skillgroup > 0) {
					fprintf (logwishfp, "\n"
							"#術科採計    : ");
					if (depbuf[idx].skillway == 1) {
						fprintf (logwishfp,
							"單科加權: %s x %s",
							skill_test_cname[
							depbuf[idx].skillgroup],
						       	exam_weight_name[
							depbuf[idx].skillweight]
						);
					} else {
						fprintf (logwishfp,
							"佔固定比: [學科]:[%s]="
							"%d:%d",
							skill_test_cname[
							depbuf[idx].skillgroup],
							depbuf[idx].skill_main,
							depbuf[idx].skill_skill
							);
					}

					if (depbuf[idx].skill_major != 0) {
						fprintf (logwishfp,
							"\n"
							"#術科主修限制: %d 分",
							depbuf[idx].skill_major
						);
					}
				}

				fprintf (logwishfp,
					"\n"
					"#同分參酌    : "
				);

				for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++){
					if (depbuf[idx].onsame[i] == 0) break;
					fprintf (logwishfp, "[%s]",
						dpg_onsame_crs[
							depbuf[idx].onsame[i]]);
				}

				fprintf (logwishfp,
					"\n"
				);
			}

			++idx;
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

	if ((depsortedindex = misc->malloc (depnum *
			sizeof (struct department_indexes_t))) == NULL) {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	for (i = 0; i < depnum; i++) {
		depsortedindex[i].idx = i;
		depsortedindex[i].id  = depbuf[i].id;
	}

	qsort (depsortedindex, depnum,
			sizeof (struct department_indexes_t),
			(int (*)(const void *, const void *)) order_by_did);

	misc->print (PRINT_LEVEL_SYSTEM,
			"ok.  (elapsed time: %.4f seconds)\r\n",
			misc->timer_ended ());

	return 1;
}

static int analyze_deplim_group (void) {
	int		i, j, found;
	int32_t		bitmap, skmap;
	int32_t		*gsort;
	int		*gsort_sort;
	int		sidx = 0;
	int		ssidx = 0;
	int		noc;

	gsort      = alloca (sizeof (int32_t) * depnum);
	gsort_sort = alloca (sizeof (int) * depnum);

	misc->print (PRINT_LEVEL_SYSTEM, "Analysis deplim ... ");

	for (i = 0; i < depnum; i++) {
		bitmap = 0;
		skmap  = 0;

		for (j = noc = 0; j < 9; j++) {
			bitmap <<= 1;
			if (depbuf[i].weight[j] != 0) {
				++noc;
				bitmap |= 1;
			}
		}

		if (depbuf[i].skillgroup > 0) {
			skmap = 1;
			skmap <<= (12 + depbuf[i].skillgroup);
			bitmap |= skmap;
		}

		for (j = found = 0; j < sidx; j++) {
			if (gsort[j] == bitmap) {
				found = 1;
				break;
			}
		}

		depbuf[i].deplim_grp = j;
		depbuf[i].number_of_refcrs = noc;

		if (! found) {
			gsort_sort[sidx] = sidx;
			gsort[sidx++] = bitmap;

			if (depbuf[i].skillgroup > 0) ssidx++;
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%d group (%d + %d)\n",
					sidx, sidx - ssidx, ssidx);

	return sidx;
}

static int read_depna_record (void *buf) {
	struct ncu_depname_t	*ptr = buf;
	struct university_t	*uptr;
	char			sname[128];
	char			dname[128];
	int			i, found, depid, univid;

	depid  = misc->oct (ptr->wishid, DEPARTMENT_ID_LENGTH);
	univid = misc->dec (ptr->schid, sizeof ptr->schid);

	if ((i = search_department (depid)) == -1) return 0;

	memcpy (sname, ptr->sch_name, sizeof ptr->sch_name);
	memcpy (dname, ptr->dep_name, sizeof ptr->dep_name);

	sname[sizeof ptr->sch_name] = '\0';
	dname[sizeof ptr->dep_name] = '\0';

	misc->rtrim (sname);
	misc->rtrim (dname);

	depbuf[i].cname = strdup (dname);

	for (uptr = univ, found = 0; uptr != NULL; uptr = uptr->next) {
		if (uptr->schid == univid) {
			found = 1;
			break;
		}
	}

	if (! found) {
		if ((uptr = misc->malloc (sizeof (struct university_t)))
							== NULL) {
			misc->perror (PRINT_LEVEL_SYSTEM);
			return 0;
		}

		uptr->next = univ;
		uptr->sch_name = strdup (sname);
		uptr->schid = univid;
		univ = uptr;
	}

	depbuf[i].univ = uptr;

	return 1;
}

static int load_depna (void) {
	char		*filename;
	int		cnt = 0;
	int		len;
	char		*ptr;

	if (depbuf == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM, "Load limit first, please\n");
		return 0;
	}

	if ((filename = sysconfig->getstr ("department-name-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (department-name-file)"
			" not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load department name file [ %s ] ... ", filename);

	if (tio->open (filename)) {
#if DULL_ELEPHANT_OPTION == 1
		if (de_is_skip_first_line) tio->read (&ptr);
#endif
		while ((len = tio->read (&ptr)) >= 0) {
			if (len < sizeof (struct ncu_depname_t)) continue;
			if (read_depna_record (ptr)) cnt++;
		}
		tio->close ();
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%d entries\r\n", cnt);

	return 1;
}

#if ENABLE_DEPARTMENT_FIRST == 1


#  if NO_MULTIPLE_SELECT_ON_SAME_WISH == 1
static void clear_use_log (void) {
	int	i;

	for (i = 0; i < depnum; i++) use_log[i] = 0;
}

static int  test_and_set (const int i) {
	if (use_log[i] == 1) return 0;
	use_log[i] = 1;

	return 1;
}
#  endif

static void invalid_rest_wishes (struct student_on_wish_t *sw) {
	int	k, m;
	int	widx;

	// sw = &depbuf[i].stdlist[j];


#if DEBUG_LEVEL > 0
	if (sw->st->sid == trace_sid) {
		dprint ("Student: %d, invalid wish-order from %d to %d\n",
			sw->st->sid,
			sw->wish_order, sw->st->atleast_wish);
	}
#endif
	if (sw->wish_order > sw->st->atleast_wish) return;

	for (k = sw->wish_order; k < sw->st->atleast_wish; k++){

		// 因該生至少本學系可以錄取, 所以
		// 該生後面的志願的學系上的記錄可以塗消

		// 先取得後面志願的 ID
		if ((widx = sw->st->wishidx[k]) < 0) continue;

		// 取得志願名次
		if ((m = sw->st->order_on_wish[k]) == -1) continue;
#if DEBUG_LEVEL > 0
		if (sw->st->sid == trace_sid) {
			dprint ("Student: %d, invalid %d " WISHID_FMT
				" (%d) ... ",
					sw->st->sid,
					k,
					sw->st->wish[k],
					widx);
		}
#endif

		// 註記成無效

		depbuf[widx].stdlist[m].aflag |= SWF_INVALID;

		// 註記該學系因失去學生而可能需要名額遞補
		//if ((m < depbuf[widx].llindex) &&
		 //     ((depbuf[widx].stdlist[m].aflag & SWF_INVALID) == 0)) {
		if (m < depbuf[widx].llindex) {
			
			depbuf[widx].fix_require++;

#if DEBUG_LEVEL > 0
			if (sw->st->sid == trace_sid) {
				dprint ("yes on %d (%d)\n",
						depbuf[widx].index,
						depbuf[widx].fix_require);
			}
		} else {
			if (sw->st->sid == trace_sid) {
				dprint ("ooh\n");
			}
#endif
		}
	}

	sw->st->atleast_wish = sw->st->matched_wish = sw->wish_order;
}

static void re_arrange_ranklist (void) {
	int	i, j, k;

	misc->print (PRINT_LEVEL_SYSTEM, "Re-arrange ranklist ... ");
	misc->timer_started ();

	for (i = 0; i < depnum; i++) {
		for (j = k = 0; j < depbuf[i].llindex; j++) {
#if DEBUG_LEVEL > 0
			if (depbuf[i].stdlist[j].st->sid == trace_sid) {
				dprint ("Student: %d, Wish order: %d, "
					"Wish: " WISHID_FMT " (%d) Flag=%d\n",
					depbuf[i].stdlist[j].st->sid,
					depbuf[i].stdlist[j].wish_order,
					depbuf[i].index, i,
					depbuf[i].stdlist[j].aflag);
			}
#endif
			if ((depbuf[i].stdlist[j].aflag & SWF_INVALID) == 0) {
				if (j != k) {
					memcpy (&depbuf[i].stdlist[k],
						&depbuf[i].stdlist[j],
					     sizeof (struct student_on_wish_t));
				}
				k++;
			}
		}

		depbuf[i].lastidx   = k;
		depbuf[i].inlistcnt = depbuf[i].real_num;
		depbuf[i].llindex   = 0;

		if (k-- != 0) depbuf[i].minreq = depbuf[i].stdlist[k].score;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"ok.  (elapsed time: %.4f seconds)\r\n",
			misc->timer_ended ());
}


static int do_require_fix (void) {
	int	i, j, k;
	int	need_fix;
#if DEBUG_LEVEL > 0
	int	debug_on = 0;
#endif

	for (i = need_fix = 0; i < depnum; i++) {
		if (depbuf[i].fix_require == 0) continue;

		// fprintf (stderr, "require fix on %d\n", i);

		//////////////////////////////////////////////////////////

		// 補上一些人, 順便往後去註記

		// 實際人數:   depbuf[i].inlistcnt, depbuf[i].lastidx
		// 原錄取數:   depbuf[i].real_num
		// 設定名額:   depbuf[i].num
		// 原孫山之後: depbuf[i].llindex
		// 先扣掉原先誤以為要增額錄取的名額

		// 目前的錄取人數

		depbuf[i].real_num -= depbuf[i].fix_require;
		depbuf[i].fix_require = 0;

		if (depbuf[i].real_num >= depbuf[i].num) continue;

		need_fix = 1;

		for (j = depbuf[i].llindex; j < depbuf[i].inlistcnt; j++) {
			if ((depbuf[i].stdlist[j].aflag & SWF_INVALID) != 0) {
				// 跳過那些已經高就的 ....
				continue;
			}


#if DEBUG_LEVEL > 0
			debug_on = 0;

			if (depbuf[i].stdlist[j].st->sid == trace_sid) {
				debug_on = 1;
				dprint ("Student: %d, on " WISHID_FMT " (%d) "
					"[ %d, %d, %d ]\n",
						depbuf[i].stdlist[j].st->sid,
						depbuf[i].id,
						i, depbuf[i].real_num,
						depbuf[i].num, j);
			}
#endif
			invalid_rest_wishes (&depbuf[i].stdlist[j]);

			depbuf[i].llindex = j + 1;

			if (++depbuf[i].real_num >= depbuf[i].num) break;
		}

		// 補了人之後, 檢查是否有新的同分增額錄取的情形 ...

		j = depbuf[i].llindex;

		for (k = j--; k < depbuf[i].inlistcnt; k++) {
			if ((depbuf[i].stdlist[k].aflag & SWF_INVALID) != 0) {
				// 跳過那些已經高就的 ....
				continue;
			}

			if (compare_order (&depbuf[i].stdlist[j],
					   &depbuf[i].stdlist[k]) == 0) {
#if DEBUG_LEVEL > 0
				if (debug_on) {
					dprint ("Same score: %d, %d on "
							WISHID_FMT
							" (%d) [%d,%d]\n",
						depbuf[i].stdlist[j].st->sid,
						depbuf[i].stdlist[k].st->sid,
						depbuf[i].id, i,
						j, k);
				}
#endif
				// 同分增額
				invalid_rest_wishes (&depbuf[i].stdlist[k]);
				// depbuf[i].llindex++;
				depbuf[i].llindex = k + 1;
				depbuf[i].real_num++;
			} else {
				break;
			}
		}

		//////////////////////////////////////////////////////////
	}

	return need_fix;
}

static void invalid_wishes_after (void) {
	int	i, j;
	
	for (i = 0; i < depnum; i++) {
#if DEBUG_LEVEL > 9
		dprint ("Current wish: " WISHID_FMT " (0 to %d)\n",
				depbuf[i].id,
				depbuf[i].llindex);
#endif
		for (j = 0; j < depbuf[i].llindex; j++) {
			invalid_rest_wishes (&depbuf[i].stdlist[j]);
		}
	}
}

static void fix_department_member (const int idx) {
	int	i, j;

	// 先將借用的名額還原
	depbuf[idx].num = depbuf[idx].real_num;

	// 先假設無需重跑
	depbuf[idx].fix_require = 0;

	if (depbuf[idx].inlistcnt >= depbuf[idx].num) {
		// 有效志願人數大於或等於錄取名額
		depbuf[idx].llindex = depbuf[idx].num;
	} else {
		// 有效志願人數小於錄取名額
		depbuf[idx].llindex  = depbuf[idx].real_num = 
						depbuf[idx].inlistcnt;
	}


	if ((j = depbuf[idx].llindex) > 0) {
		// 先確定有人填此志願
		//
		// 去看看是否有因同分參酌而增額錄取的 ...
		for (i = j--; i < depbuf[idx].inlistcnt; i++) {
			if (compare_order (&depbuf[idx].stdlist[j],
					&depbuf[idx].stdlist[i]) == 0) {
				// 同分增額
				depbuf[idx].llindex++;
				depbuf[idx].real_num++;
			} else {
				break;
			}
		}
	}
}

static void sort_ranklist (const int idx) {
	qsort (depbuf[idx].stdlist, depbuf[idx].inlistcnt,
			sizeof (struct student_on_wish_t),
			(int (*)(const void *, const void *)) compare_order);
}

static void fix_all_department_member (void) {
	int	i;

	for (i = 0; i < depnum; i++) fix_department_member (i);
}

static void sort_all_ranklist (void) {
	int	i;

	misc->print (PRINT_LEVEL_SYSTEM,
			"Sort for all department ranklist ... ");

	misc->timer_started ();

	for (i = 0; i < depnum; i++) sort_ranklist (i);

	misc->print (PRINT_LEVEL_SYSTEM,
			"ok.  (elapsed time: %.4f seconds)\r\n",
			misc->timer_ended ());
}

static void extend_space (void) {
	int	i;

	for (i = 0; i < depnum; i++) {
		depbuf[i].num = depbuf[i].student_cnt;
	}
}


static void set_wish_cross_reference (void) {
	int	idx, i, j;

	misc->print (PRINT_LEVEL_SYSTEM,
			"Generate cross reference wish order indexes ... ");

	for (idx = 0; idx < depnum; idx++) {
		for (i = 0; i < depbuf[idx].inlistcnt; i++) {
			// wish_order 以 1 起算 ... 先修正
			j = depbuf[idx].stdlist[i].wish_order - 1;

			// order_on_wish 以 零起算 ... 不修正
			depbuf[idx].stdlist[i].st->order_on_wish[j] = i;
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM, "ok.\r\n");
}


//   以下是關於 rotation 的部份

#if 0
static const char * rotation_engine_version (void) {
	static const char	*version = "3.0";
	return version;
}
#endif

//	一些 rotation algorithm 共用的

static void write_matched_wish (void) {
	int     i, j;

	for (i = 0; i < depnum; i++) {
		for (j = 0; j < depbuf[i].llindex; j++) {
			if ((depbuf[i].stdlist[j].aflag & SWF_INVALID) == 0) {
				depbuf[i].stdlist[j].st->matched_wish =
						depbuf[i].stdlist[j].wish_order;
				/*
				  depbuf[i].stdlist[j].st->matched_wish = i;
				*/
#if DEBUG_LEVEL > 0
				if (depbuf[i].stdlist[j].st->sid == trace_sid) {
					dprint ("%d Matched on " WISHID_FMT
						" (%d)\r\n",
						trace_sid, depbuf[i].id,
						depbuf[i].stdlist[j].st->
							matched_wish);
				}
#endif
			}
		}
	}
}

static int first_admirer (const int i, const int from, int *rejno) {
	int	found;
	int	j, k = 0, retval = -1;

	/*
	 *	Return Value:
	 *		-1:	沒有 Best Admirer	(Type 0)
	 *		-2:	Best Admirer 是落榜生	(Type 1)
	 *		 n:	Best Admirer 找到了
	 */

	if ((j = from) == -1) j = depbuf[i].llindex;
	if (rejno != NULL) *rejno = 0;

	for (found = 0; j < depbuf[i].inlistcnt; j++) {
		if ((depbuf[i].stdlist[j].aflag & SWF_INVALID) != 0) {
			continue; // 跳過那些已經高就的 ....
		}

		if (found) {
			if (compare_order (&depbuf[i].stdlist[j],
					&depbuf[i].stdlist[k]) != 0) {
				break;
			}
		}

		if (depbuf[i].stdlist[j].st->matched_wish == -1) {
			// 落榜生
#if ENABLE_DR_LEE_SPECIAL == 1
			if (dr_lee_flag == 1) {
				if (dr_lee_rjstcnt++ == 0) {
					dr_lee_studid =
						depbuf[i].stdlist[j].st->sid;
					dr_lee_wishid = depbuf[i].id;
					continue;
				} else if (dr_lee_rjstcnt <= dr_lee_optionflag){
					if (dr_lee_wishid == depbuf[i].id) {
						continue;
					}
				}
				// 先忽略落榜生 !!
				// 把落榜生加入 List 中
				// (depbuf[i].stdlist[j].st, depbuf[i].id)
				/*
				fprintf (stderr, "Ignore (%d," WISHID_FMT ")\n",
						depbuf[i].stdlist[j].st->sid,
						depbuf[i].id);
				*/
			}
#endif
			found = 1;
			k = j;
			retval = -2;
			if (rejno != NULL) (*rejno)++;
		} else if (depbuf[i].stdlist[j].wish_order <
					depbuf[i].stdlist[j].st->matched_wish) {
			found  = 1;
			retval = j;
#if ENABLE_DR_LEE_SPECIAL == 1
			if (dr_lee_flag == 1) {
				/*
				fprintf (stderr, "Got (%d," WISHID_FMT ")\n",
						depbuf[i].stdlist[j].st->sid,
						depbuf[i].id);
				*/
			}
#endif
			break;
		}
	}

#if DEBUG_LEVEL > 0
	if (found && (retval >= 0)) {
		if ((depbuf[i].stdlist[retval].st->sid == trace_sid) ||
				       (depbuf[i].id == trace_wishid))	{
			dprint ("%d is best admirer on " WISHID_FMT
					" (%d,%d)\r\n",
				depbuf[i].stdlist[retval].st->sid,
				depbuf[i].id,
				i, retval);
		}
	}
#endif

	return retval;
}

static int next_admirer (const int i, const int j, int *rejno) {
	int	k;

	for (k = j + 1; k < depbuf[i].inlistcnt; k++) {
#if DEBUG_LEVEL > 1
		if (depbuf[i].stdlist[k].st->sid == trace_sid) {
			dprint ("Find next admirer on " WISHID_FMT " for %d ",
					depbuf[i].id,
					trace_sid);
		}
#endif
		if (compare_order (&depbuf[i].stdlist[j],
					&depbuf[i].stdlist[k]) != 0) {
#if DEBUG_LEVEL > 1
			if (depbuf[i].stdlist[k].st->sid == trace_sid) {
				dprint ("under\r\n");
			}
#endif
			break;
		}

		if ((depbuf[i].stdlist[k].aflag & SWF_INVALID) != 0) {
#if DEBUG_LEVEL > 1
			if (depbuf[i].stdlist[k].st->sid == trace_sid) {
				dprint ("over\r\n");
			}
#endif
			continue; // 跳過那些已經高就的 ....
		}

		if (depbuf[i].stdlist[k].st->matched_wish == -1) {
			if (rejno != NULL) (*rejno)++;
#if DEBUG_LEVEL > 1
			if (depbuf[i].stdlist[k].st->sid == trace_sid) {
				dprint ("reject\r\n");
			}
#endif
			continue; // 跳過落榜生 ...
		}

		if (depbuf[i].stdlist[k].wish_order <
				depbuf[i].stdlist[k].st->matched_wish) {
#if DEBUG_LEVEL > 1
			if (depbuf[i].stdlist[k].st->sid == trace_sid) {
				dprint ("got it\r\n");
			}
#endif
			return k;
		}
	}

	return -1;
}

int regist_rotation_algorithm (struct rotation_algorithm_t *alg) {
	static struct rotation_algorithm_list_t	**tail = &rotalglist;
	static struct department_internal_data_t	didt;
	static int					initialized = 0;
	struct rotation_algorithm_list_t	*ptr;

	if ((ptr = misc->malloc (
		sizeof (struct rotation_algorithm_list_t))) == NULL) return 0;

	if (! initialized) {
		didt.depbuf = depbuf;
		didt.depnum = depnum;

		didt.write_matched_wish		= write_matched_wish;
		didt.first_admirer		= first_admirer;
		didt.next_admirer		= next_admirer;
		didt.invalid_rest_wishes	= invalid_rest_wishes;
		didt.compare_order		= compare_order;
		didt.do_require_fix		= do_require_fix;

		initialized = 1;
	}

	ptr->algorithm = alg;
	ptr->next      = NULL;
	// rotalglist     = ptr;
	*tail = ptr;
	tail  = &ptr->next;

	alg->init (&didt);

	return 1;
}

static int init_rotation_algorithms (void) {
	int	i, j, flen, len;
	int	*list;

	struct {
		int  flag;
		void (*func)(void);
	} funclist[] = {
		{ 0, regist_rotation_algorithm_version_1 },
		{ 0, regist_rotation_algorithm_version_2 },
		{ 0, regist_rotation_algorithm_version_3 },
		{ 0, regist_rotation_algorithm_version_4 },
		{ 0, regist_rotation_algorithm_version_5 },
		{ 0, NULL }
	};

	misc->print (PRINT_LEVEL_SYSTEM, "Regist rotation algorithms:");

	if ((list = sysconfig->intlist (
				"enable-rotation-algorithms", &len)) != NULL) {
		for (i = flen = 0; funclist[i].func != NULL; i++) {
			funclist[i].flag = 0;
			flen++;
		}

		// printf ("function length = %d, list len = %d\n", flen, len);

		for (i = 0; i < len; i++) {
			j = list[i] - 1;

			if ((j >= 0) && (j < flen)) {
				if (funclist[j].flag == 0) {
					funclist[j].flag = 1;
					funclist[j].func ();

					misc->print (PRINT_LEVEL_SYSTEM,
							" %d", j + 1);
				}
			}
		}

	} else {
		for (i = 0; funclist[i].func != NULL; i++) funclist[i].func ();

		misc->print (PRINT_LEVEL_SYSTEM, " all");
	}

	misc->print (PRINT_LEVEL_SYSTEM, "\r\n");

	return 1;
}


static int do_rotate (void) {
	struct rotation_algorithm_list_t	*ptr;
	int					still_have_rotation;
	int					total_rotation;
	int					total_shift;
	int					total_rescue;
	int					ro, sf, rc;

	misc->timer_started ();

	misc->print (PRINT_LEVEL_SYSTEM,
			"\r\nRotation Engine: Version 3.1,   "
			"rotation timer initialized ...\r\n");

	total_rotation = total_shift = total_rescue = 0;
	// 註冊各種 rotation algorithms 然後一一去做 rotation
	// 直到每個 rotation algorithm  都沒有辦法再產生新的 rotation
	init_rotation_algorithms ();

	// write_matched_wish ();

#if DEBUG_LEVEL > 0
	if (debug_flag) {
		int	i;

		for (i = 0; i < depnum; i++) {
			if (trace_wishid == depbuf[i].id) {
				misc->print (PRINT_LEVEL_DEBUG,
						"On " WISHID_FMT " : ",
						trace_wishid);

				first_admirer (i, -1, NULL);
			}
		}
	}
#endif

	do {
		still_have_rotation = 0;

		for (ptr = rotalglist; ptr != NULL; ptr = ptr->next) {
			ptr->algorithm->prepare ();

			if (ptr->algorithm->try_rotation (&ro, &sf, &rc) > 0) {
				still_have_rotation = 1;

				total_rotation += ro;
				total_shift    += sf;
				total_rescue   += rc;
			}
		}
	} while (still_have_rotation);

	misc->print (PRINT_LEVEL_SYSTEM,
		"Rotation finished [ %d rotate, %d shift, %d rescue. "
		"%.3f sec. ]\r\n\r\n",
		total_rotation,
		total_shift,
		total_rescue,
		misc->timer_ended ());

#if ENABLE_DR_LEE_SPECIAL == 1
	if (dr_lee_optionflag) {
		misc->print (PRINT_LEVEL_SYSTEM,
				"Starting Dr. Lee's special ... ");
		dr_lee_flag = 1;
		finding_reject_possible ();
		dr_lee_flag = 0;
		misc->print (PRINT_LEVEL_SYSTEM, "ok.\r\n\r\n");
	}
#endif

#if DEBUG_LEVEL > 9
	if (debug_flag) {
		//286,917   ok
		//283,1240  fail (why) (0477,22020416)(0474,22036939)[283,1240]
		//590,1358  ok
		int	i = 283;
		int	j = 1240;

		dprint ("Force Invalid on [%d,%d] " WISHID_FMT
				", student %d\r\n",
				i, j,
				depbuf[i].id,
				depbuf[i].stdlist[j].st->sid);

		invalid_rest_wishes (&depbuf[i].stdlist[j]);
		i = 0; while (do_require_fix ()) i++;
		dprint ("%d walk(s)\r\n", i);
	}
#endif

	return 0;
}

#endif

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
				sizeof (struct student_on_wish_t);

		freshman += depbuf[i].num;
		depbuf[i].stdlist = misc->malloc (allocate);

		total += allocate;

		if (depbuf[i].stdlist == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM,
					"failed");
			return 0;
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%lu bytes allocated (%d)\r\n",
			total, freshman);

	return 1;
}

static int search_department (const int wish) {
	struct department_indexes_t	*ptr;
	struct department_indexes_t	key;

	key.id = wish;

	ptr = bsearch (&key, depsortedindex, depnum,
			sizeof (struct department_indexes_t),
			(int (*)(const void *, const void *)) order_by_did);

	if (ptr == NULL) return -1;

	return ptr->idx;
}

static struct department_t *  find_by_id (const int16_t idx) {
	int	i;

	if ((i = search_department (idx)) == -1) return NULL;

	return &depbuf[i];
}

static struct department_t * find_by_index (const int idx) {
	if ((idx < 0) || (idx >= depnum)) return NULL;

	return &depbuf[idx];
}

////////////////////////////////////////////////////////////////

static int	current_wish_index = 0;

static int16_t reset_wish (void) {
	current_wish_index = 0;

	return next_wish ();
}

static int16_t next_wish (void) {
	if ((current_wish_index >= 0) && (current_wish_index < depnum)) {
		return depbuf[current_wish_index++].id;
	}

	return -1;
}

static void user_format_output (FILE *fp, char *name, int *list) {
	int		i, j, k, m, state, len;
	char		fmt[8192];
	char		*format = userdefine_format;

//	format = "$w, $s, $3o, $t, $q1, $q2, $q3, $q4, $q5, $q6, $q7, $N\r\n";
//	format = "$s$w$t$o\r\n";

	len = strlen (format);

	for (i = j = m = state = 0; (i < len) && (j < 4095); i++) {
		if (state == 0) {
			switch (format[i]) {
			case '$':
				state = 1;
				break;
			case '\\':
				state = 2;
				break;
			default:
				state = 0;
				fmt[j++] = format[i];
				break;
			}
		} else if (state == 2) {
			switch (format[i]) {
			case '\\':
				fmt[j++] = '\\';
				break;
			case 'n':
				fmt[j++] = '\n';
				break;
			case 'r':
				fmt[j++] = '\r';
				break;
			case 't':
				fmt[j++] = '\t';
				break;
			case '$':
				fmt[j++] = '$';
				break;
			default:
				fmt[j++] = '\\';
				i--;
				break;
			}
			state = 0;
		} else {
			//	N	12 考生姓名
			//	s	 8 準考證號
			//	n	 6 考生序號
			//	w	 4 志願代號
			//	o	 2 志願序
			//	t	 5 加權總分
			//	g1 ~ 5	 2 學測五科級分
			//	e1 ~ 9	 5 指定科目九科分數
			//	q1 ~ 7	 5 同分參酌七科分數
			//	k	 5 術科成績
			//	m	 5 術科主修成績
			//	u	 1 同分參酌使用序

			k = (int) (format[i + 1] - '0');

			switch (format[i]) {
			case 'N':	// 12 考生姓名
				j += sprintf (&fmt[j], "%12s", name);
				break;
			case 's':	// 8 準考證號
				j += sprintf (&fmt[j], "%8d", list[0]);
				break;
			case 'n':	// 6 考生序號
				j += sprintf (&fmt[j], "%6d", list[1]);
				break;
			case 'w':	// 4 志願代號
				j += sprintf (&fmt[j], WISHID_FMT, list[2]);
				break;
			case 'o':	// 2 志願序
				j += sprintf (&fmt[j], "%2d", list[3]);
				break;
			case 't':	// 6 加權總分
				j += sprintf (&fmt[j], "%6d", list[4]);
				break;
			case 'T':	// 6 加權總分
				j += sprintf (&fmt[j], "%7.2f",
						(float) list[4] / 100.);
				break;
			case 'g':	// 2 學測五科級分 (g1 - g5)
				if ((k >= 1) && (k <= 5)) {
					j += sprintf (&fmt[j], "%2d",
							list[4 + k]);
					i++;
				}
				break;
			case 'e':	// 5 指定科目九科分數 (e1 - e9)
				if ((k >= 1) && (k <= 9)) {
					j += sprintf (&fmt[j], "%5d",
							list[9 + k]);
					i++;
				}
				break;
			case 'q':	// 5 同分參酌七科分數 (q1 - q7)
				if ((k >= 1) && (k <= 7)) {
					j += sprintf (&fmt[j], "%5d",
							list[18 + k]);
					i++;
				}
				break;
			case 'r':	// 5 同分參酌七科分數 (q1 - q7)
				if ((k >= 1) && (k <= 7)) {
					if (list[25 + k] >= 8) {
						j += sprintf (&fmt[j], "%6.2f",
							(float)
							list[18 + k] / 100.0);
					} else {
						j += sprintf (&fmt[j], "%6d",
							list[18 + k]);
					}
					i++;
				}
				break;
			case 'Q':	// 5 同分參酌七科科目 (Q1 - Q7)
				if ((k >= 1) && (k <= 7)) {
					if (list[25 + k] == 0) {
						j += sprintf (&fmt[j],
								"        ");
					} else {
						j += sprintf (&fmt[j], "%-8s",
						  dpg_onsame_crs[list[25 + k]]);
					}
					i++;
				}
				break;
			case 'u':	// 同分參酌
				j += sprintf (&fmt[j], "%d", list[33]);
				break;
			case 'K':	// 術科組別
				j += sprintf (&fmt[j], "%d", list[34]);
				break;
			case 'k':	// 5 術科成績
				j += sprintf (&fmt[j], "%5d", list[35]);
				break;
			case 'm':	// 5 術科主修成績
				j += sprintf (&fmt[j], "%5d", list[36]);
				break;
			case 'M':	// 術科主修科目
				j += sprintf (&fmt[j], "%04d", list[37]);
				break;
			case 'I':	// 術科副修科目
				j += sprintf (&fmt[j], "%04d", list[38]);
				break;
			}
			state = 0;
		}
	}

	fwrite (fmt, j, 1, fp);
	// fmt[j]     = '\0';
	// arglist[m] = -1;

	// userdefine_format  = fmt;
	// userdefine_arglist = arglist;
}

static void prepare_list (int *list, const struct department_t *ptr,
				const struct student_t *st, const int order,
				const int score, const int *onsame) {
	int		j, k = 0;

	//  0	s	 8 準考證號
	//  1	n	 6 考生序號
	//  2	w	 4 志願代號
	//  3	o	 2 志願序
	//  4	t	 5 加權總分
	//  5	g1 ~ 5	 2 學測五科級分
	// 10	e1 ~ 9	 5 指定科目九科分數
	// 19	q1 ~ 7	 5 同分參酌七科分數
	// 26	Q1 ~ 7     同分參酌七科科目
	// 19	r1 ~ 7	 6 同分參酌七科分數
	// 33	u	 1 最高使用同分參酌序
	// 34	K        1 術科組別
	// 35	k	 5 術科成績
	// 36	m	 5 術科主修成績
	// 37	M	 4 術科主修
	// 38	I	 4 術科副修

	k = 0;
	list[k++] = st->sid;
#if ENABLE_CEEC_FORMAT == 1
#  if ENABLE_CEEC_MERGE_SN == 1
	list[k++] = st->sn;
#  else
#    if TRY_CEEC_MERGE_SN == 1
	list[k++] = st->serial;
#    else
	list[k++] = 0;	// 沒有考生序號
#    endif
#  endif
#else
	list[k++] = 0;
#endif
	list[k++] = ptr->id;
	list[k++] = order;
	list[k++] = score;

	for (j = 0; j < 5; j++) {
		list[k++] = st->gradscore[j];
	}

	for (j = 0; j < 9; j++) {
		list[k++] = st->testscore[j];
	}

	for (j = 0; j < MAXIMUM_ON_SAME_REFERENCE; j++){
		if (ptr->onsame[j] == 2) {
			list[k++] = MAX_PER_STUDENT_WISH - onsame[j];
		} else {
			list[k++] = onsame[j];
		}
	}

	for (j = 0; j < MAXIMUM_ON_SAME_REFERENCE; j++){
		list[k++] = ptr->onsame[j];
	}

	list[k++] = ptr->max_onsame_use;

	list[k++] = st->skill_group;

	list[k++] = st->skillscore;
	list[k++] = st->skillmajor_score;

	list[k++] = st->skillmajor;
	list[k++] = st->skillminor;
}

static int force_takesort (struct department_t *ptr, int *org) {
	int	i, sort;

	if ((*org = sort = ptr->check2) == 0) {
		if (ptr->onsame[3] == 0) {
			*org = sort = 6;	// 乙案
		} else {
			*org = sort = 5;	// 甲案
		}

		if (ptr->weight[4] != 0) {		// 歷史
			sort = 1;
		} else if (ptr->weight[5] != 0) {	// 地理
			sort = 1;
		} else if (ptr->weight[8] != 0) {	// 生物
			sort = 3;
		} else if (ptr->weight[6] != 0) {	// 物理
			sort = 2;
		} else if (ptr->weight[7] != 0) {	// 化學
			sort = 2;
		} else if (ptr->weight[2] != 0) {	// 數甲
			sort = 2;
		} else if (ptr->weight[3] != 0) {	// 數乙
			sort = 1;
		}

		if (sort == 5) {			// 甲案
			for (i = 0; i < 7; i++) {
				switch (ptr->onsame[i]) {
				case 6:	// 社會
					sort = 1;
					break;
				case 7:	// 自然
					sort = 2;
					break;
				}

				if (sort != 5) break;
			}
		} else if (sort == 6) {			// 乙案
			for (i = 0; i < 3; i++) {
				switch (ptr->onsame[i]) {
				case 10:		// 數甲
					sort = 2;
					break;
				case 11:	// 數乙
					sort = 1;
					break;
				case 12:	// 歷史
					sort = 1;
					break;
				case 13:	// 地理
					sort = 1;
					break;
				case 14:	// 物理
					sort = 2;
					break;
				case 15:	// 化學
					sort = 2;
					break;
				case 16:	// 生物
					sort = 3;
					break;
				}

				if (sort != 6) break;
			}
		}
	}

	if (sort > 0) sort--;

	return sort;
}

static int print_minimal_proto (FILE *fp, const int16_t wish, const int proto,
					const int w, const int8_t *pweight) {
	struct department_t	*ptr;
	int			i, j;
	struct student_t	*st;
	const double		weight[] = { 0.0, 1.0, 1.25, 1.5, 1.75, 2.0 };
	double			dsum;
	int			sum;
	int			score[1024];
	int			max, min, avg;
	int			cnt, mid;
	unsigned long long	var;
	int			total;
	int			sort, org;
	const int8_t		*course_weight = NULL;

	if (fp == NULL) return 0;

	/*
	if (pptr == NULL) {
		if ((pptr = find_by_id (proto)) == NULL) return 0;

		fprintf (stderr, WISHID_FMT ": ", pptr->id);

		for (i = 0; i < 9; i++) {
			fprintf (stderr, "[%d]", pptr->weight[i]);
		}

		fprintf (stderr, "\n");
	}
	*/
	if ((ptr  = find_by_id (wish )) == NULL) return 0;

	if (proto >= 0) {
       		struct department_t	*pptr = NULL;
		if ((pptr = find_by_id (proto)) == NULL) return 0;

		course_weight = pptr->weight;
	} else if (proto == -1) {
		course_weight = pweight;
	}


	sort = force_takesort (ptr, &org);

	total = cnt = max = min = var = mid = avg = 0;
	var = 0;

	for (i = 0; i < ptr->lastidx; i++) {
		st = ptr->stdlist[i].st;
		if (st->ratio != 0) continue;

		dsum = 0.0;

		for (j = 0; j < 9; j++) {
			if (course_weight[j] == 0) continue;

			if (st->testscore[j] < 0) {
				dsum = -1;
				break;
			}

			if (w) {
				dsum += st->testscore[j] *
					weight[course_weight[j]];
			} else {
				dsum += st->testscore[j];
			}
		}

		if (dsum <= 0) continue;

		/*
		if (ptr->id == 01005) {
			fprintf (stderr, "%d:", st->sid);

			for (j = 0; j < 9; j++) {
				fprintf (stderr, "[%d]", st->testscore[j]);
			}
			fprintf (stderr, "\n");
		}
		*/

		sum = dsum;

		if (cnt == 0) max = min = sum;

		total += sum;

		max = max > sum ? max : sum;
		min = min < sum ? min : sum;

		score[cnt++] = sum;
	}

	if (cnt > 0) {
		avg = total / cnt;

		for (i = 0; i < cnt; i++) {
			sum = score[i];
			var += ((sum - avg) * (sum - avg));
		}

		i = cnt / 2;

		if (cnt % 2 == 0) {
			mid = (score[i] + score[i - 1]) / 2;
		} else {
			mid = score[i];
		}
	}

	fprintf (fp, WISHID_FMT ",%d,%d,%d,%d,%d,%d,%d,%llu\n",
			ptr->id, sort + 1, org, max, min, avg, mid, cnt, var);

	return 1;
}

static int print_minimal_special (FILE *fp, const int16_t wish) {
	struct student_t	*st;
	struct department_t	*ptr;
	int			i, j, k, sum, sort, org, avg, var, mid, cnt;
	int			min = -1, max = -1;
	static const int	course[4][7] = {
					{ 1, 2, 4, 5, 6, 0, 0 },
					{ 1, 2, 3, 7, 8, 0, 0 },
					{ 1, 2, 3, 7, 8, 9, 0 },
					{ 1, 2, 3, 8, 9, 0, 0 }
				};

	if (fp == NULL) return 0;
	if ((ptr = find_by_id (wish)) == NULL) return 0;

	sort = force_takesort (ptr, &org);

	/*
	if ((sort = org = ptr->check2) == 0) {
		if (ptr->onsame[3] == 0) {
			org = sort = 6;	// 乙案
		} else {
			org = sort = 5;	// 甲案
		}

		if (ptr->weight[4] != 0) {		// 歷史
			sort = 1;
		} else if (ptr->weight[5] != 0) {	// 地理
			sort = 1;
		} else if (ptr->weight[8] != 0) {	// 生物
			sort = 3;
		} else if (ptr->weight[6] != 0) {	// 物理
			sort = 2;
		} else if (ptr->weight[7] != 0) {	// 化學
			sort = 2;
		} else if (ptr->weight[2] != 0) {	// 數甲
			sort = 2;
		} else if (ptr->weight[3] != 0) {	// 數乙
			sort = 1;
		}

		if (sort == 5) {			// 甲案
			for (i = 0; i < 7; i++) {
				switch (ptr->onsame[i]) {
				case 6:	// 社會
					sort = 1;
					break;
				case 7:	// 自然
					sort = 2;
					break;
				}

				if (sort != 5) break;
			}
		} else if (sort == 6) {			// 乙案
			for (i = 0; i < 3; i++) {
				switch (ptr->onsame[i]) {
				case 10:		// 數甲
					sort = 2;
					break;
				case 11:	// 數乙
					sort = 1;
					break;
				case 12:	// 歷史
					sort = 1;
					break;
				case 13:	// 地理
					sort = 1;
					break;
				case 14:	// 物理
					sort = 2;
					break;
				case 15:	// 化學
					sort = 2;
					break;
				case 16:	// 生物
					sort = 3;
					break;
				}

				if (sort != 6) break;
			}
		}
	}

	if (sort > 0) sort--;
	*/

	avg = var = cnt = mid = 0;

	if (sort < 4) {
		int	total  = 0;
		int	score[1024];

		for (i = 0; i < ptr->lastidx; i++) {
			st = ptr->stdlist[i].st;

			if (st->ratio != 0) continue;

			for (sum = j = 0; (k = course[sort][j]) != 0; j++) {
				if (st->testscore[k - 1] < 0) {
					sum = -1;
					break;
				}

				sum += st->testscore[k - 1];
			}

			if (sum <= 0) continue;
			if (cnt == 0) max = min = sum;

			total += sum;

			max = max > sum ? max : sum;
			min = min < sum ? min : sum;

			score[cnt++] = sum;
		}


		if (cnt > 0) {
			avg = total / cnt;

			for (i = 0; i < cnt; i++) {
				sum = score[i];
				var += ((sum - avg) * (sum - avg));
			}

			i = cnt / 2;

			if (cnt % 2 == 0) {
				mid = (score[i] + score[i - 1]) / 2;
			} else {
				mid = score[i];
			}
		}
	}

	fprintf (fp, WISHID_FMT ",%d,%d,%d,%d,%d,%d,%d,%d\n",
			ptr->id, sort + 1, org, max, min, avg, mid, cnt, var);

	return 1;
}

static int print_minimal (FILE *fp, const int16_t wish,
					const int flag, const int check) {
	struct department_t	*ptr;
	struct student_t	*student;
	int			i, ii, j, k;
	int			sort = 0, sum;
	int			have_score = 0;
	static const int	course[4][7] = {
					{ 1, 2, 4, 5, 6, 0, 0 },
					{ 1, 2, 3, 7, 8, 0, 0 },
					{ 1, 2, 3, 7, 8, 9, 0 },
					{ 1, 2, 3, 8, 9, 0, 0 }
				};

	if (fp == NULL) return 0;
	if ((ptr = find_by_id (wish)) == NULL) return 0;

#if 0
	if (algorithm > 1) {
		if (ptr->lastidx != ptr->inlistcnt) {
			printf ("on " WISHID_FMT
					" lastidx = %d, inlistcnt = %d\n",
					ptr->id, ptr->lastidx, ptr->inlistcnt);
		}
	}
#endif

	for (i = 0; i < 9; i++) {
		if (ptr->weight[i] != 0) {
			have_score = 1;
			break;
		}
	}

	if (! have_score) {
		if ((ptr->skillgroup > 0) && (ptr->skillweight > 0)) {
			have_score = 1;
		}
	}

	sum = 0;

	fprintf (fp, WISHID_FMT, ptr->id);

	if (flag >= 2) {
		if ((sort = ptr->check2) == 0) {
			if (ptr->onsame[3] == 0) {
				sort = 6;	// 乙案
			} else {
				sort = 5;	// 甲案
			}
		}

		if (check) {
			fprintf (fp, ", ");
		} else {
			fprintf (fp, ", %d, %3d, %3d, %5d, %5d, ",
				sort,
				ptr->num,
				ptr->inlistcnt,
				ptr->reject_cnt,
				ptr->n_qualify_cnt);
		}

		// 算一下平均成績 ...
	}

	if (flag == 3) {
		fprintf (fp, "%3d, ", ptr->number_of_special);

		if (ptr->inlistcnt == 0) {
			fprintf (fp, "%5d, %5d, %5d, %5d, %5d, ",
					0, 0, 0, 0, 0);
		} else {
			sum = 0;
			student = ptr->stdlist[0].st;

			/*
			if ((k = sort - 1) < 4) {
				// 丙案
				for (j = 0; (ii = course[k][j]) != 0; j++) {
					sum += student->testscore[ii-1];
				}
			} else {
				sum = student->gradsum;
			}
			*/
			i = ptr->lastidx - 1;
			sum = ptr->stdlist[i].score;

			fprintf (fp, "%5d, %5d, %5d, %5d, %5d, ",
					ptr->max_orgsum,
					ptr->min_orgsum,
					ptr->min_orgsum_n,
					ptr->stdlist[0].score,
					sum);
		}
	}

	if (ptr->num > ptr->inlistcnt) {
		i = ptr->lastidx - 1;

		// 不足額 ...
		if (flag == 1) {
			fprintf (fp, "******");
		} else if (flag == 0) {
			// 找最後一名的人的成績 ?

			if (have_score) {
				// fprintf (fp, "*******");
				if (ptr->inlistcnt == 0) {
					// 沒有錄取到任何人
					// fprintf (fp, "  0.00");
					fprintf (fp, "從缺   ");
				} else {
					// 抓最後一名的人成績充數
					fprintf (fp, "%7.2f",
						(float) ptr->stdlist[i].score /
						100.0);
				}
			} else {
				// fprintf (fp, "*******");
				fprintf (fp, "不採計 ");
			}
		} else {
			fprintf (fp, "%6d", 0);

			if (flag >= 2) fprintf (fp, ", %5d", 0);
		}

		for (j = 0; j < MAXIMUM_ON_SAME_REFERENCE; j++) {
			if (flag >= 2) fprintf (fp, ", ");

			if (flag == 0) {
				if (ptr->onsame[j] == 0) {
					fprintf (fp, "--------------");
				} else {
					fprintf (fp, "%-8s******",
						dpg_onsame_crs[ptr->onsame[j]]);
				}
			} else if (flag == 1) {
				if (ptr->onsame[j] == 0) {
					fprintf (fp, "-----");
				} else {
					fprintf (fp, "*****");
				}
			} else {
				fprintf (fp, "%5d", 0);
			}
		}
	} else if (check) {
		if ((flag == 1) && (ptr->minreq == 0)) {
			fprintf (fp, "******");
		} else {
			fprintf (fp, "%6d", ptr->minreq); 
		}

		for (j = 0; j < MAXIMUM_ON_SAME_REFERENCE; j++) {
			if (flag >= 2) fprintf (fp, ", ");

			if ((flag == 1) && (ptr->onsame[j] == 0)) {
				fprintf (fp, "-----");
			} else if (ptr->onsame[j] == 2) {
				fprintf (fp, "%5d",
						MAX_PER_STUDENT_WISH -
						ptr->min_onsame[j]);
			} else {
				fprintf (fp, "%5d", ptr->min_onsame[j]);
			}
		}
	} else {
		i = ptr->lastidx - 1;

		if (flag == 0) {
			if (have_score) {
				fprintf (fp, "%7.2f",
					(float) ptr->stdlist[i].score / 100.0);
			} else {
				fprintf (fp, "不採計 ");
			}
		} else if ((flag == 1) && (ptr->stdlist[i].score == 0)) {
			fprintf (fp, "******");
		} else {
			fprintf (fp, "%6d", ptr->stdlist[i].score);
		}


		if (flag >= 2) {
			sum = 0;
			student = ptr->stdlist[i].st;

			if ((k = sort - 1) < 4) {
				for (j = 0; (ii = course[k][j]) != 0; j++) {
					sum += student->testscore[ii-1];
				}
			} else {
				sum = student->gradsum;
			}
			fprintf (fp, ", %5d", sum);
		}

		for (j = 0; j < MAXIMUM_ON_SAME_REFERENCE; j++) {
			if (flag >= 2) fprintf (fp, ", ");

			if (flag == 0) {
				if (ptr->onsame[j] == 0) {
					fprintf (fp, "--------------");
				} else {
					fprintf (fp, "%-8s",
						dpg_onsame_crs[ptr->onsame[j]]);
					if (j < ptr->max_onsame_use) {
						if (ptr->onsame[j] >= 8) {
							fprintf (fp, "%6.2f",
								(float)
								ptr->
								stdlist[i].
								onsame[j] /
								100.0);
						} else if (ptr->onsame[j]==2) {
							fprintf (fp, "%6d",
							  MAX_PER_STUDENT_WISH -
								ptr->
								stdlist[i].
								onsame[j]);
						} else {
							fprintf (fp, "%6d",
								ptr->
								stdlist[i].
								onsame[j]);
						}
					} else {
						fprintf (fp, "******");
					}
				}
			} else if ((flag == 1) && (ptr->onsame[j] == 0)) {
				fprintf (fp, "-----");
			} else if (ptr->onsame[j] == 2) {
				fprintf (fp, "%5d",
						MAX_PER_STUDENT_WISH -
						ptr->stdlist[i].onsame[j]);
			} else {
				fprintf (fp, "%5d",
						ptr->stdlist[i].onsame[j]);
			}
		}
	}

	if (flag == 0) {
		// fprintf (fp, "\r\n");
		fprintf (fp, "%d%3d\r\n",
				ptr->max_onsame_use,
				ptr->inlistcnt);
	} else if (flag >= 2) {
		fprintf (fp, ", %d\n", ptr->max_onsame_use);

#if 0
		if ((ptr->cstgrp > 0) && (ptr->max_onsame_use > 0)) {
			static int	i = 0;

			printf ("C%04d%d" WISHID_FMT "\n",
					++i, ptr->fail_onsame_sid,
					ptr->id);
		}
#endif
	} else {
		fprintf (fp, "%d\r\n", ptr->max_onsame_use);
	}

	return 1;
}

static void calculate_statistic	(const int16_t wish) {
	struct student_list_t	*sl;
	int			i;
	int32_t			score;
	int32_t			maxa = 0, mina = 0, minn = 0;
	int32_t			total_n = 0;
	int16_t			nspcnt = 0;
	int16_t			spcnt = 0;
	int32_t			scrlist[1024];
	int8_t			mings = 0;
	struct department_t	*ptr;
	double			avg;

	//	var += ((sum - avg) * (sum - avg));

	if ((ptr = find_by_id (wish)) == NULL) return;

	for (i = 0; i < ptr->lastidx; i++) {
		score = ptr->stdlist[i].st->org_score;

		maxa = maxa > score ? maxa : score;

		if (mina == 0) {
			mina = score;
		} else {
			mina = mina < score ? mina : score;
		}

		if (ptr->stdlist[i].st->gradsum > 0) {
			if (mings == 0) {
				mings = ptr->stdlist[i].st->gradsum;
			} else if (mings > ptr->stdlist[i].st->gradsum) {
				mings = ptr->stdlist[i].st->gradsum;
			}
		}

		if (ptr->stdlist[i].st->ratio == 0) {
			if (minn == 0) {
				minn = score;
			} else {
				minn = minn < score ? minn : score;
			}
			total_n += score;
			scrlist[nspcnt++] = score;
		} else {
			spcnt++;
		}
	}

	for (sl = ptr->lastlist; sl != NULL; sl = sl->next, i++) {
		score = sl->st->org_score;

		maxa = maxa > score ? maxa : score;

		if (mina == 0) {
			mina = score;
		} else {
			mina = mina < score ? mina : score;
		}

		if (sl->st->gradsum > 0) {
			if (mings == 0) {
				mings = sl->st->gradsum;
			} else if (mings > sl->st->gradsum) {
				mings = sl->st->gradsum;
			}
		}

		if (sl->st->ratio == 0) {
			if (minn == 0) {
				minn = score;
			} else {
				minn = minn < score ? minn : score;
			}
			total_n += score;
			scrlist[nspcnt++] = score;
		} else {
			spcnt++;
		}
	}

	ptr->max_orgsum		= maxa;
	ptr->min_orgsum		= mina;
	ptr->min_orgsum_n	= minn;

	if (nspcnt == 0) {
		ptr->avg_orgsum_n = 0;
		avg = 0.0;
	} else {
		avg = (double) total_n / (double) nspcnt;
		ptr->avg_orgsum_n = misc->toInt (avg);
	}

	ptr->variation = 0.0;

	for (i = 0; i < nspcnt; i++) {
		ptr->variation += (((double) (scrlist[i] - avg) / 100.) *
				((double) (scrlist[i] - avg) / 100.));
	}

	ptr->number_of_special	= spcnt;

	ptr->min_gradesum	= mings;
}

static void print_score_info (FILE *fp, struct student_t *st, const int mc) {
	int				i;
#if ENABLE_DISPATCHER92 == 1
	int				j;
	struct student_skill_info_t	*skinf;
#endif

	for (i = 0; i < 5; i++) {
		fprintf (fp, ",%2d", st->gradscore[i]);
	}

	for (i = 0; i < 9; i++) {
		if (mc && st->testmiss[i]) {
			fprintf (fp, ",-2");
		} else {
			fprintf (fp, ",%5d", st->testscore[i]);
		}
	}

#if ENABLE_DISPATCHER92 == 1
	if (use_disp92) {
		if ((skinf = st->skinfo) == NULL) {
			fprintf (fp, ",-99");
			return;
		} else if (st->skill_group > 3) {
			fprintf (fp, ",-99");
			return;
		}

		for (i = 0; i < NUMBER_OF_SKILL92; i++) {
			fprintf (fp, ",%d", (st->skill_sid[i] == 0) ? -1 : 1);

			if (i < SK92_SUBSCR_SET) {
				for (j = 0; j < SK92_SUBSCR_NUM; j++) {
					if (st->skill_sid[i] == 0) {
						// 未報考
						fprintf (fp, ",-1");
					} else if (skinf->sk_miss[i][j] == 1) {
						fprintf (fp, ",-2");
					} else {
						fprintf (fp, ",%d",
							skinf->sk_score[i][j]);
					}
				}
			} else {
				if (st->skill_sid[i] == 0) {
					fprintf (fp, ",-1,-1");
				} else if (skinf->physical_miss == 1) {
					fprintf (fp, ",-2,-2");
				} else {
					fprintf (fp, ",%d,%d",
						skinf->physical_score,
						skinf->physical_grade);
				}
			}
		}

		if (st->skill_sid[0] == 0) {
			fprintf (fp, ",%04d,%04d", 0, 0);
		} else {
			fprintf (fp, ",%04d,%04d",
				st->skillmajor,
				st->skillminor);
		}
	} else {
#endif
		fprintf (fp, ",%d,%d", st->skillscore,
					st->skillmajor_score);
		fprintf (fp, ",%d,%04d,%04d", st->skill_group,
					st->skillmajor,
					st->skillminor);
#if ENABLE_DISPATCHER92 == 1
	}
#endif
}

// 尋找加考數乙而佔數乙的人 ...
#define FIND_MATH_AB	1
#define PRINT_STUDENT_WISH_NUMBER 0

static int print_list (FILE *fp, const int16_t wish) {
	struct department_t	*ptr;
	struct student_list_t	*sl;
	int			i, j;
	double			score;
	char			*name;
	int			parmlist[70];
#if FIND_MATH_AB == 1
	int			dep_want  = 0;
	int			is_math_a = 0;
	int			is_math_b = 0;
	int			cflag = 0;
	int			math_on = 0;
#endif
	short			of3457 = 0;
	short			of45 = 0;
	short			miss_care = 0;


	switch (output_format) {
	case 4:
	case 5:
		of45 = 1;
	case 7:
		miss_care = 1;
	case 3:
		of3457 = 1;
	}

	if (fp == NULL) return 0;
	if ((ptr = find_by_id (wish)) == NULL) return 0;

	if (selected_algorithm > 1) {
		if (ptr->lastidx != ptr->inlistcnt) {
			/*
			printf ("on " WISHID_FMT
				" lastidx = %d, inlistcnt = %d (fixed)\n",
				ptr->id, ptr->lastidx, ptr->inlistcnt);
			*/
			// ptr->lastidx = ptr->inlistcnt;
		}
	}

#if FIND_MATH_AB == 1
	// 採計數乙
	if ((x_option == 2) && (ptr->weight[3] != 0)) {
		if ((ptr->weight[4] == 0) && (ptr->weight[5] == 0)) {
			if (ptr->check2 == 0) is_math_b = dep_want = 1;
		} else {
			dep_want = 2;
		}
	}

	// 採計數甲
	if ((x_option == 1) && (ptr->weight[2] != 0)) {
		if ((ptr->weight[6] == 0) && (ptr->weight[7] == 0)
			 && (ptr->weight[8] == 0)) {
			if (ptr->check2 == 0) is_math_a = dep_want = 1;
		} else {
			dep_want = 2;
		}
	}
#endif

	if (output_format == 1) {
		fprintf (fp, "DEPARTMENT: " WISHID_FMT
				" %3d/%3d %s SCORE=%6.2f\n",
				ptr->id,
				ptr->num,
				ptr->inlistcnt,
				ptr->llindex ? "+" : " ",
				ptr->minreq / 100.0);
	}

	if (ptr->id == logwish) {
		fprintf (logwishfp,
				"%%實際錄取人數: %d\n"
				"%%未入選人數  : %d\n"
				"%%檢定不合人數: %d\n",
				ptr->inlistcnt,
				ptr->reject_cnt,
				ptr->n_qualify_cnt);
	}


	for (i = 0; i < ptr->lastidx; i++) {
		score = (double) ptr->stdlist[i].score;

#if PRINT_STUDENT_WISH_NUMBER
		printf ("%d\n", ptr->stdlist[i].st->number_of_wishes);
#endif

#if FIND_MATH_AB == 1
		if (dep_want == 1) {
			math_on = 0;

			if (is_math_a ||
				(ptr->stdlist[i].st->testscore[3] > 0)) {
				// 數乙有考
				math_on = 1;
			} else if (is_math_b ||
				(ptr->stdlist[i].st->testscore[2] > 0)){
				// 數甲有考
				math_on = 1;
			}

			cflag = 0;

			if (math_on) {
				if (ptr->stdlist[i].st->testscore[4] > 0) {
					// 有考歷史
					cflag |= 1;
				}
				if (ptr->stdlist[i].st->testscore[5] > 0) {
					// 有考地理
					cflag |= 2;
				}
				if (ptr->stdlist[i].st->testscore[6] > 0) {
					// 有考物理
					cflag |= 4;
				}
				if (ptr->stdlist[i].st->testscore[7] > 0) {
					// 有考化學
					cflag |= 8;
				}
				if (ptr->stdlist[i].st->testscore[8] > 0) {
					// 有考生物
					cflag |= 16;
				}

				if (is_math_a && ((cflag & 28) == 0) 
							&& ((cflag & 3) != 0)) {
					printf (WISHID_FMT ",%d,%d\n",
						ptr->id,
						ptr->stdlist[i].st->sid,
						ptr->stdlist[i].st->
							testscore[2]);
				} else if (is_math_b && ((cflag & 3) == 0)
						       && ((cflag & 28) != 0)) {
					printf (WISHID_FMT ",%d,%d\n",
						ptr->id,
						ptr->stdlist[i].st->sid,
						ptr->stdlist[i].st->
							testscore[3]);
				} else {
					printf (WISHID_FMT ",%d,--\n",
						ptr->id,
						ptr->stdlist[i].st->sid);
				}
			}
		} else if (dep_want == 2) {
			printf (WISHID_FMT ",%d,**\n",
				ptr->id,
				ptr->stdlist[i].st->sid);
		}
#endif

// #define PRINT_STUDENT_BIRTHDAY
#ifdef PRINT_STUDENT_BIRTHDAY
		printf ("%d %d " WISHID_FMT " %6.2f %s %s\n",
			ptr->stdlist[i].st->info->birthday,
			ptr->stdlist[i].st->sid,
			ptr->id,
			((double) ptr->stdlist[i].score / 100.),
			ptr->univ->sch_name,
			ptr->cname);
#endif

		if (output_format == 0) {
#if ENABLE_DEPARTMENT_FIRST == 1
			j = ((selected_algorithm == 1)
					? ptr->stdlist[i].st->wish_index
					: ptr->stdlist[i].wish_order);
#else
			j = ptr->stdlist[i].st->wish_index;
#endif
			name = ptr->stdlist[i].st->info->name;

			prepare_list (parmlist, ptr, ptr->stdlist[i].st,
					j, misc->toInt (score),
					ptr->stdlist[i].onsame);

			user_format_output (fp, name, parmlist);
		} else if (output_format == 1) {
			if ((i % 4) == 0) fprintf (fp, "\n");

			fprintf (fp, "  %8d (%6.2f)",
				ptr->stdlist[i].st->sid, score / 100.);
		} else if (output_format == 2) {
			fprintf (fp, "%8d" WISHID_FMT "%6d%2d\r\n",
				ptr->stdlist[i].st->sid,
				ptr->id,
				misc->toInt (score),
#if ENABLE_DEPARTMENT_FIRST == 1
				((selected_algorithm == 1)
					? ptr->stdlist[i].st->wish_index
					: ptr->stdlist[i].wish_order)
#else
				ptr->stdlist[i].st->wish_index
#endif
				);
		} else if (of3457) {
			fprintf (fp, WISHID_FMT ", %8d, %3d, %6d",
				ptr->id, ptr->stdlist[i].st->sid,
#if ENABLE_DEPARTMENT_FIRST == 1
				((selected_algorithm == 1)
					? ptr->stdlist[i].st->wish_index
					: ptr->stdlist[i].wish_order),
#else
				ptr->stdlist[i].st->wish_index,
#endif
				misc->toInt (score));

			if (of45) {
				for (j = 0; j < MAXIMUM_ON_SAME_REFERENCE; j++){
					if (ptr->onsame[j] == 2) {
						fprintf (fp, ", %5d",
						    MAX_PER_STUDENT_WISH -
						    ptr->stdlist[i].onsame[j]);
					} else {
						fprintf (fp, ", %5d",
						    ptr->stdlist[i].onsame[j]);
					}
				}
			}
			
			if (output_format == 5) {
				fprintf (fp, ", %s",
					ptr->stdlist[i].st->info->name);
			}

			if (output_format == 7) {
				print_score_info (fp, ptr->stdlist[i].st,
							miss_care);
			}

			fprintf (fp, "\n");
		}
	}

	for (sl = ptr->lastlist; sl != NULL; sl = sl->next, i++) {
		score = (double) ptr->stdlist[ptr->lastidx-1].score;
#if PRINT_STUDENT_WISH_NUMBER
		printf ("%d\n", ptr->stdlist[i].st->number_of_wishes);
#endif

		if (output_format == 0) {
			name = sl->st->info->name;

			prepare_list (parmlist, ptr, sl->st,
					sl->st->wish_index,
					misc->toInt (score),
					ptr->stdlist[ptr->lastidx-1].onsame);

			user_format_output (fp, name, parmlist);
		} else if (output_format == 1) {
			if ((i % 4) == 0) fprintf (fp, "\n");

			fprintf (fp, "  %8d (%6.2f)",
					sl->st->sid, score / 100.);
			fprintf (fp, "[ %d, %d ]",
				ptr->stdlist[ptr->lastidx-1].st->testscore[0],
				sl->st->testscore[0]);
		} else if (output_format == 2) {
			fprintf (fp, "%8d" WISHID_FMT "%6d%2d\r\n",
				sl->st->sid,
				ptr->id,
				misc->toInt (score),
				sl->st->wish_index
				);
		} else if (of3457) {
			// 這裡對 wish_index 不能改 ...
			// 如果採用學系榜, 並不會有 lastlist 的問題
			// 所以無所謂
			fprintf (fp, WISHID_FMT ", %8d, %3d, %6d",
					ptr->id, sl->st->sid,
					sl->st->wish_index,
					misc->toInt (score));

			if (of45) {
				// 沒辦法, 因為跟這個人同分 ... so ...
				i = ptr->lastidx - 1;

				for (j = 0; j < MAXIMUM_ON_SAME_REFERENCE; j++){
					if (ptr->onsame[j] == 2) {
						fprintf (fp, ", %5d",
						    MAX_PER_STUDENT_WISH -
						    ptr->stdlist[i].onsame[j]);
					} else {
						fprintf (fp, ", %5d",
						    ptr->stdlist[i].onsame[j]);
					}
				}
			}

			if (output_format == 5) {
				fprintf (fp, ", %s", sl->st->info->name);
			}


			if (output_format == 7) {
				print_score_info (fp, sl->st, miss_care);
			}

			fprintf (fp, "\n");
		}
	}

	if (output_format == 1) fprintf (fp, "\n\n");

	return 1;
}

