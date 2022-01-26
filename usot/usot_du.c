/*
 *	usot/usot_du.c
 *
 *	Copyright (c) 2003, Jainn-Ching Liu
 */

#include <stdlib.h>
#include <string.h>

#include "predefine.h"
#include "global_var.h"
#include "sys_conf.h"
#include "misclib.h"
#include "textio.h"
#include "studlist.h"

#include "usot_du.h"
#include "usot_dept.h"
#include "usot_stud.h"

static struct lost_student_list_t	*lslptr = NULL;
static struct usot_department_module_t	*dmptr  = NULL;
static struct usot_student_module_t	*stmptr = NULL;


static void state_cleanup (struct usot_dispatcher_unit_t *du) {
	int	i;

	for (i = 0; i < MAX_USOT_CRS_TYPE; i++) du->orgsc[i] = 0;

	du->passok = 0;
	du->score = du->orgscore = 0;
	du->flags = 0;
}

static void bind_student (struct usot_dispatcher_unit_t *du,
					struct usot_student_t *stud) {
	du->st = stud;
	state_cleanup (du);
}

static int load (struct usot_dispatcher_unit_t *du, const int16_t wish) {
//	int	idx;
//	if ((idx = search_department (wish)) == -1) return 0;
//	du->dp = &depbuf[idx];

	state_cleanup (du);
	if ((du->dp = dmptr->find (wish)) == NULL) return 0;

	return 1;
}

static int check_all_req (struct usot_dispatcher_unit_t *du) {
	int				i, j, k, m;
	int				crs_match, dep_match, match;
	struct usot_courseset_t		*crset;
	int32_t				score;
	int32_t				o_score;
	int				debug_flag = 0;

	/*
	if (du->st->sid == 10004 || du->st->sid == 10161) {
		debug_flag = 1;
	}
	*/

	//printf ("Check all req for %d on %04d\n", du->st->sid, du->dp->depid);
	if (du->st->uclass != du->dp->uclass) {
		printf ("uclass not match for %d on %04d\n",
				du->st->sid, du->dp->depid);
		return 0;
	}

	if ((du->dp->lim != 0) && (du->st->check != 0)) {
		if ((du->dp->lim & du->st->check) != 0) {
			/*
			printf ("%04d, limit value = %d,"
					"stdval = %d (sid=%d)\n",
				du->dp->depid,
				du->dp->lim,
				du->st->check,
				du->st->sid);
			*/
			return 0;
		}
		/* Patch for ccy */
		// return 0;	// remove
	}

	match = 0;

	for (crset = du->dp->crset; crset != NULL; crset = crset->next) {
		du->crset = crset;
		dep_match = 1;

		for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) du->crsid[i] = 0;

		for (i = k = 0; i < MAX_USOT_EXAM_COURSE; i++) {
			if (crset->excrs[i] > 0) {
				crs_match = 0;
				for (j = 0; j < crset->excrs[i]; j++) {
					if (du->st->crs[i] == 0) {
						break;
					}

					if (du->st->crs[i] ==
							crset->crslist[k + j]) {
						crs_match = 1;
						du->crsid[i] = du->st->crs[i];
						break;
					}
				}
				if (! crs_match) {
					dep_match = 0;
					break;
				}
			}

			k += crset->excrs[i];
		}

		if (dep_match) {
			match = 1;
			break;
		}
	}

	if (! match) {
		fprintf (stderr, "%d not match on %04d\n",
					du->st->sid, du->dp->depid);
		return 0;
	}

	for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) {
		if (du->crsid[i] == 0) continue;
		// 缺考
		if (du->st->miss[i]) return 0;
	}

	score = 0;
	o_score = 0;

	for (i = 0; i < MAX_USOT_CRS_TYPE; i++) du->orgsc[i] = 0;

	if (debug_flag) {
		fprintf (stderr, "%06d %04d:",
				du->st->sid, du->dp->depid);
	}

	for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) {
		if (du->crsid[i] == 0) continue;
		if (du->dp->exam[i] <= 0) continue;

		if (du->st->sco[i] < du->dp->lowsco[i]) {
			fprintf (stderr, "%d score %d lower than %d\n",
						du->st->sid,
						du->st->sco[i],
						du->dp->lowsco[i]);
			return 0;
		}

		if (du->st->sco[i] == 0) {
			// 不得零分
			return 0;
		}

		score   += du->st->sco[i] * du->dp->exam[i];
		o_score += du->st->sco[i];

		m = dmptr->find_course_group (du->crsid[i]);

		if (m > 0 && m <= MAX_USOT_CRS_TYPE) {
			du->orgsc[m-1] += du->st->sco[i];
		}

		if (debug_flag) {
			fprintf (stderr, " + %d", du->st->sco[i]);
		}
	}


	if (debug_flag) {
		fprintf (stderr, " = %d [ %d ]\n", o_score, score);
	}

	du->orgscore  = o_score;

	o_score *= 1000;

	if (du->st->ptype != '1') {
		switch (du->st->ptype) {
		case '4': // 增加總分 10%
			score += misc->toInt (o_score * 10. / 100.);
			break;
		case 'A': // - 10%
			score += misc->toInt (o_score * 10. / 90.);
			break;
		case 'B': // - 25%
			score += misc->toInt (o_score * 25. / 75.);
			break;
		case 'C': // + 8%
			score += misc->toInt (o_score * 8. / 100.);
			break;
		case 'D': //  + 25%
			score += misc->toInt (o_score * 25. / 100.);
		}
	}
	//fprintf (stderr, "Student %d on %04d, score = %d\n",
	//		du->st->sid, du->dp->depid, score);

	du->score  = misc->toInt ((double) score / 1000.);

	if (du->dp->lowsum > 0) {
		if (du->score < du->dp->lowsum) {
			// 加權總分的檢定
			// fprintf (stderr, "%d < %d\n",
			//		du->score, du->dp->lowsum);
			du->score = 0;
			return 0;
		}
	}

	du->passok = 1;

	return 1;
}

static int is_pass (struct usot_dispatcher_unit_t *du) {
	return du->passok;
}

static int is_acceptable (struct usot_dispatcher_unit_t *du,
						const int tryres) {
	if (! du->passok) return 0;

	if (tryres) {
		if (du->score < du->dp->rdep->minreq) return 0;
	} else {
		if (du->score < du->dp->minreq) return 0;
	}
	return 1;
}

static void add_to_stdlist_tail (struct usot_dispatcher_unit_t *du,
							const int tryres) {
	struct usot_student_on_wish_t	*sw;
	int16_t 			idx;
	int32_t				*osptr;
	int				i;
	struct usot_department_t	*ptr;
	// int				i, k;

	ptr = tryres ? du->dp->rdep : du->dp;

	// 請注意 ++ 在前 ... 由於演算法先空位的關係, 方便做插入動作 ...
	sw = &ptr->stdlist[idx = ++ptr->lastidx];

	sw->st		= du->st;
	sw->score	= du->score;
	// sw->orgscore	= du->orgscore;
	sw->order	= 0;
	sw->wish_index	= du->st->wish_index;
	ptr->inlistcnt++;

	osptr = du->get_onsame (du);

	for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) {
		sw->onsame[i] = osptr[i];
	}
}

static int try_accept (struct usot_dispatcher_unit_t *du, const int tryres) {
	int				rc = 0;
	struct usot_department_t	*ptr;

	if (! du->is_acceptable (du, tryres)) return 0;

	add_to_stdlist_tail (du, tryres);

	ptr = tryres ? du->dp->rdep : du->dp;


	switch (dmptr->sort_students (ptr)) {
	case 0:	// 未入選
		break;
	case 1: // 暫時入選
	case 2: // 暫時入選, 但有些學生將被擠出
		//du->flags |= DUTF_IS_ACCEPT;

		//du->st->matched_wish = du->dp->id;
		//du->st->final_score  = score;
		rc = 1;
		break;
	}

	return rc;
}

static int32_t* get_onsame (struct usot_dispatcher_unit_t *du) {
	static int32_t	onsame[MAX_USOT_EXAM_COURSE];
	int		i, k;

	for (i = 0; i < MAX_USOT_EXAM_COURSE; i++) {
		switch (k = du->dp->onsame[i]) {
		case '0':	// 無
			onsame[i] = 0;
			break;
		case '1':	// 考試科目一
		case '2':	// 考試科目二
		case '3':	// 考試科目三
		case '4':	// 考試科目四
		case '5':	// 考試科目五
			onsame[i] = du->st->sco[k - '1'];
			break;
		case 'A':	// 專業科目原始總分 (專業=必考+選考)
			onsame[i] = du->orgsc[1] + du->orgsc[2];
			break;
		case 'B':	// 專業科目加權總分
			onsame[i] = du->score;
			break;
		case 'C':	// 專業科目最高分
		case 'D':	// 必考科目原始總分
		case 'E':	// 必考科目加權總分
		case 'F':	// 必考科目最高分
		case 'G':	// 選考科目原始總分
		case 'H':	// 選考科目加權總分
		case 'I':	// 選考科目最高分
			break;
		}
	}

	return onsame;
}

static struct lost_student_list_t* get_lostlist (
				struct usot_dispatcher_unit_t *du) {
	return lslptr;
}

struct usot_dispatcher_unit_t * allocate_usot_dispatcher_unit (
				struct usot_department_module_t *dm,
				struct usot_student_module_t *sm) {
	struct usot_dispatcher_unit_t	*ptr;


	if (dm != NULL) dmptr = dm;
	if (dmptr == NULL) dmptr = initial_usot_department_module ();

	if (sm != NULL) stmptr = sm;
	if (stmptr == NULL) stmptr = initial_usot_student_module ();

	lslptr = initial_lostlist_module ();

	if ((ptr = misc->malloc (sizeof (
				struct usot_dispatcher_unit_t))) != NULL) {
		ptr->dp			= NULL;
		ptr->st			= NULL;
		ptr->bind		= bind_student;
		ptr->load		= load;
		ptr->check_all_req	= check_all_req;
		ptr->is_pass		= is_pass;
		ptr->is_acceptable	= is_acceptable;
		ptr->try_accept		= try_accept;
		ptr->lostlist		= get_lostlist;
		ptr->get_onsame		= get_onsame;
		/*
		ptr->load_index		= load_index;
		ptr->load_test		= load_test;
		ptr->check_basic	= check_basic;
		ptr->check_exam		= check_exam;

		ptr->check_special	= check_special;
		ptr->check_sex		= check_sex;
		ptr->min_require	= min_require;
		ptr->get_score		= get_score;
		ptr->lostlist		= get_lostlist;
		ptr->bind_student	= bind_student_by_id;
		ptr->get_onsame		= get_onsame;
		ptr->try_accept		= try_accept;
		*/
	}

	return ptr;
}
