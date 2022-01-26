/*
 *	dispunit.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "predefine.h"
#include "dispunit.h"
#include "department.h"
#include "student.h"
#include "studlist.h"
#include "global_var.h"
#include "misclib.h"
#include "textio.h"
#include "stdvalue.h"
#include "sys_conf.h"
#include "skillset.h"
#include "postcheck.h"


////////////////////////////////////////////////////////////////////

static struct lost_student_list_t	*lslptr = NULL;
static struct department_module_t	*dmptr  = NULL;
static struct student_module_t		*stmptr = NULL;

////////////////////////////////////////////////////////////////////

static int load_test (struct dispatcher_unit_t *du, const int wmonly);

////////////////////////////////////////////////////////////////////

#if ENABLE_DISPATCHER92 == 1
#include "code92/du92.c"
#endif

static void state_cleanup (struct dispatcher_unit_t *du) {
	du->score = du->orgscore = du->addtotoal = 0;
	du->flags  = 0;
}

#if ENABLE_DEPARTMENT_FIRST == 1
static int load_wish_order (struct dispatcher_unit_t *du, const int odr) {
	state_cleanup (du);

	if ((du->dp = dmptr->find_index (du->st->wishidx[odr])) == NULL)
	       return 0;

	return 1;
}
#endif

static int load_index (struct dispatcher_unit_t *du, const int widx) {
	state_cleanup (du);
	if ((du->dp = dmptr->find_index (widx)) == NULL) return 0;

	return 1;
}

static int load (struct dispatcher_unit_t *du, const int16_t wish) {
//	int	idx;
//	if ((idx = search_department (wish)) == -1) return 0;
//	du->dp = &depbuf[idx];

	state_cleanup (du);
	if ((du->dp = dmptr->find (wish)) == NULL) return 0;

	return 1;
}

static void bind_student (struct dispatcher_unit_t *du,
					struct student_t *stud) {

	du->st = stud;
	state_cleanup (du);
}

static int32_t* get_onsame (struct dispatcher_unit_t *du) {
	static int32_t	onsame[MAXIMUM_ON_SAME_REFERENCE];
	int		i, k;

	for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
		switch (k = du->dp->onsame[i]) {
		case 0:	// No
			onsame[i] = 0;
			break;
		case 1: // 總級分
			onsame[i] = du->st->gradsum;
			break;
		case 2: // 志願序 -- 故意做成相反才會有相同的比較方式
			if (no_wish_order) {
				onsame[i] = MAX_PER_STUDENT_WISH - 1;
			} else {
				onsame[i] =
					MAX_PER_STUDENT_WISH -
					du->st->wish_index;
			}
			break;
		default:
			if ((k >= 3) && (k <= 7)) {
				k -= 3;
				onsame[i] = du->st->gradscore[k];
			} else if ((k >= 8) && (k <= 16)) {
				k -= 8;
				onsame[i] = du->st->testscore[k];
			} else {
#if ENABLE_DISPATCHER92 == 1
				if (use_disp92) {
					onsame[i] = 
						misc->toInt ((double)
						du->st->skillscore / 100.0);
				} else {
					onsame[i] = du->st->skillscore;
				}
#else
				onsame[i] = du->st->skillscore;
#endif
			}
			break;
		}
	}

	return onsame;
}

static int bind_student_by_id (struct dispatcher_unit_t *du,
							const int32_t sid) {
	struct student_t	*stptr;

	if ((stptr = stmptr->find (sid)) != NULL) {
		bind_student (du, stptr);
		return 1;
	}
	return 0;
}

static int check_sex (struct dispatcher_unit_t *du) {
	if ((du->flags & DUTF_SEX_IS_CHECK) == 0) {
		du->flags |= DUTF_SEX_IS_CHECK;

#if DEBUG_LEVEL > 1
		if (du->st->sid == trace_sid) {
			misc->print (PRINT_LEVEL_DEBUG,
				"SEX    check for %d on " WISHID_FMT " ... ",
				du->st->sid, du->dp->id);
		}
#endif

		if (du->dp->sex == 0) {
			du->flags |= DUTF_SEXCHECK_IS_PASS;
#if DEBUG_LEVEL > 1
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
						"no restriction\r\n");
			}
#endif
			return 1;
		}
		if (du->dp->sex == 1) {	// 限男生
#if DEBUG_LEVEL > 1
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
					"need boy ... ");
			}
#endif
			if ((du->st->sflag & STUDENT_FLAG_GIRL) == 0) {
				du->flags |= DUTF_SEXCHECK_IS_PASS;
#if DEBUG_LEVEL > 1
				if (du->st->sid == trace_sid) {
					misc->print (PRINT_LEVEL_DEBUG,
						"yes\r\n");
				}
#endif
				return 1;
			}
#if DEBUG_LEVEL > 1
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG, "no\r\n");
			}
#endif
		} else {			// 限女生
#if DEBUG_LEVEL > 1
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
					"need girl ... ");
			}
#endif
			if (du->st->sflag & STUDENT_FLAG_GIRL) {
				du->flags |= DUTF_SEXCHECK_IS_PASS;
#if DEBUG_LEVEL > 1
				if (du->st->sid == trace_sid) {
					misc->print (PRINT_LEVEL_DEBUG,
						"yes\r\n");
				}
#endif
				return 1;
			}
			// du->dp->reject_cnt++;
#if DEBUG_LEVEL > 1
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG, "no\r\n");
			}
#endif
		}

#if ENABLE_POSTCHECK == 1
		pstck->gender_not_qualify (
				du->st,
				du->st->wish_index,
				du->dp->id,
				du->dp->sex);
#endif

#if DEBUG_LEVEL > 8
		dprint ("wish: " WISHID_FMT " - %d %d\n",
				du->dp->id, du->dp->sex,
				du->st->sid);
		if (du->dp->id == 0706) dprintw ("");
#endif
	}
	return (du->flags & DUTF_SEXCHECK_IS_PASS);
}

#if ENABLE_PRE_TESTING
static int on_pre_checkall = 0;

static int pre_checkall (struct dispatcher_unit_t *du, const int mask) {
	int	i, retval = 0;

	// 這個地方是個預檢的動作, 要檢查是否符合學測一般檢定
	// 或指科四個一般檢定的任一個, 如果任一滿足, return 0
	on_pre_checkall = 1;

	if (mask == 0) printf ("%06d,%d - ", du->st->serial, du->st->sid);

	do {
		if ((mask & 1) == 0) {
			du->dp->check1  = 1;	// 先假設需要學科檢定
			if (du->check_basic (du)) {
				if (mask == 0) {
					printf ("ok (basic)\n");
				}
				break;
			}
		}

		retval = 1;

		if ((mask & 2) == 0) {
			for (i = 1; i <= 4; i++) {
				// du->flags |= DUTF_CHECK2_IS_CHECK;
				du->flags = 0;
				du->dp->check2 = i;
				if (du->check_exam (du)) {
					if (mask == 0) {
						printf ("ok (test %d)\n", i);
					}
					retval = 0;
					break;
				}
			}
		}

		if ((mask == 0) && (retval == 1)) printf ("fail\n");
	} while (0);

	on_pre_checkall = 0;

	return retval;
}
#endif

// 特殊限制檢定
static int check_special (struct dispatcher_unit_t *du) {
	if ((du->st->sflag & STUDENT_FLAG_NU_ONLY) != 0) {
		// 只能考師範
		if (du->dp->special != 3) {
#if ENABLE_POSTCHECK == 1
			pstck->normal_univ_only (
				du->st,
				du->st->wish_index,
				du->dp->id);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_NORMAL_UNIV_ONLY;
#endif
			return 0;
		}
		// return (du->dp->special == 3) ? 1 : 0;
		return 1;
	} else if (du->dp->special == 2) {
		if ((du->st->sflag & STUDENT_FLAG_DF_ALLOW) == 0) {
#if ENABLE_POSTCHECK == 1
			pstck->fail_on_military (
				du->st,
				du->st->wish_index,
				du->dp->id);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_MILITARY_NOT_QUALIFY;
#endif
			return 0;
		}

		return 1;
	}

	return 1;
}

// 術科檢定
static int check_skill (struct dispatcher_unit_t *du) {

	struct skillset_t	*skptr;
	int64_t			skmj, skmi;
	int32_t			skill_check_score;
	short			skok;
#if SKILL_COUNT_VERSION == 0
	// 術科組別的 8 (美術二) 換成 5 (美術)
	static int	checkfix[] = { 0, 1, 2, 3, 4, 5, 6, 7, 5 };
#endif

	if ((du->flags & DUTF_SKILL_IS_CHECK) == 0) {
		du->flags |= DUTF_SKILL_IS_CHECK;

#if DEBUG_LEVEL > 0
		if (du->st->sid == trace_sid) {
			dprint ("SKILL    check for %d on " WISHID_FMT "\n",
					du->st->sid, du->dp->id);
		}
#endif

		if (du->dp->skillgroup == 0) {
			// 沒有術科
			du->flags |= DUTF_SKILLCHECK_IS_PASS;
			return 1;
		}

		// 沒考術科
		if (du->st->skill_group == 0) {
			// du->dp->reject_cnt++;
#if ENABLE_POSTCHECK == 1
			pstck->need_skill_group (
				du->st,
				du->st->wish_index,
				du->dp->id,
				du->dp->skillgroup, 0);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_NO_SKILL_ITEM;
#endif
			return 0;
		}

#if SKILL_COUNT_VERSION == 0
		if ((du->dp->skillgroup != du->st->skill_group) &&
			checkfix[du->dp->skillgroup] != du->st->skill_group) {
#else
		if (du->dp->skillgroup != du->st->skill_group) {
#endif
#if DEBUG_LEVEL > 0
			if (du->st->sid == trace_sid) {
				dprint (WISHID_FMT " - group %d v.s. %d (%d)\n",
					du->dp->id,
					du->dp->skillgroup,
					du->st->skill_group,
					du->st->sid);
			}
#endif

#if ENABLE_POSTCHECK == 1
			pstck->need_skill_group (
				du->st,
				du->st->wish_index,
				du->dp->id,
				du->dp->skillgroup,
				du->st->skill_group);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_NO_SKILL_ITEM;
#endif
			return 0;
		}

#if DEBUG_LEVEL > 0
		if (du->st->sid == trace_sid) {
			dprint (WISHID_FMT " - group %d v.s. %d (%d) .. ok\n",
					du->dp->id,
					du->dp->skillgroup,
					du->st->skill_group,
					du->st->sid);
		}
#endif
		// 術科主修成績限制

		if ((du->dp->skill_major > 0) &&
		    (du->st->skillmajor_score < du->dp->skill_major)) {
			// 未達主修成績之標準
			// du->dp->reject_cnt++;
#if DEBUG_LEVEL > 0
			if (du->st->sid == trace_sid) {
				dprint ("On " WISHID_FMT
				     " Not meet standard on major: %d < %d\r\n",
					du->dp->id,
					du->st->skillmajor_score,
					du->dp->skill_major);
			}
#endif

#if ENABLE_POSTCHECK == 1
			pstck->skill_major_score (
				du->st,
				du->st->wish_index,
				du->dp->id,
				du->st->skillmajor_score,
				du->dp->skill_major);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_NO_SKILL_ITEM;
#endif
			return 0;
#if DEBUG_LEVEL > 0
		} else if (du->st->sid == trace_sid) {
			dprint ("On " WISHID_FMT
				" Meet standard on major: %d >= %d\r\n",
					du->dp->id,
					du->st->skillmajor_score,
					du->dp->skill_major);
#endif
		}

		// 主副修科目限制
		if ((skptr = du->dp->skillset) != NULL) {
			skmj = dmptr->skill_bitmap (du->dp->skillgroup,
							du->st->skillmajor);
			skmi = dmptr->skill_bitmap (du->dp->skillgroup,
							du->st->skillminor);

			for (skok = 0; skptr != NULL; skptr = skptr->next) {
				// 請注意! 這裡是假設 skptr->major == 0
				// 或 skptr->minor == 0 的狀況時,
				// 表任何科目皆行 (因為若當皆不行是不合理的)
#if DEBUG_LEVEL > 0
				if ((du->st->sid == trace_sid) ||
						(du->dp->id == trace_wishid)) {
					dprint (WISHID_FMT
						" %d (%llx/%llx)(%llx/%llx)\n",
							du->dp->id,
							du->st->sid,
							skptr->major,
							skptr->minor,
							skmj, skmi);
				}
#endif

				if (((skptr->major == 0) ||
				    ((skmj & skptr->major) != 0)) &&
				    ((skptr->minor == 0) ||
				    ((skmi & skptr->minor) != 0))) {
					skok = 1;
					break;
				}
			}
#if DEBUG_LEVEL > 0
			if ((du->st->sid == trace_sid) ||
					(du->dp->id == trace_wishid)) {
				dprint ("Major/Minor: %04d/%04d (%s)\n",
						du->st->skillmajor,
						du->st->skillminor,
						skok ? "ok" : "fail");
			}
#endif

			if (disallow_same_instrument) {
				if (skmj == skmi) {
					skptr = du->dp->skillset;
					if ((skptr->major != 0) &&
							(skptr->minor != 0)) {
						// 主副修相同
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
						du->st->not_qualify_flag |=
							STNQF_INSTRUMENT;
#endif
						return 0;
					}
				}
			}


			if (! skok) {
#if ENABLE_POSTCHECK == 1
				pstck->fail_on_instrument (
					du->st,
					du->st->wish_index,
					du->dp->id,
					du->st->skillmajor,
					du->st->skillminor);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag |= STNQF_INSTRUMENT;
#endif
				// du->dp->reject_cnt++;
				return 0; // 主修或副修科目不滿足
			}
		}

#if DEBUG_LEVEL > 0
		if ((du->st->sid == trace_sid) ||
					(du->dp->id == trace_wishid)) {
			dprint ("SKILL  check for %d on " WISHID_FMT
					" (%d-%d)[%d:%d]\n",
					du->st->sid, du->dp->id,
					du->st->skill_group,
					du->dp->skillgroup,
					du->dp->skill_main,
					du->dp->skill_skill);
		}
#endif


#if SKILL_CHECK_BY_CALCULATE == 0
		skill_check_score = du->st->skillcheck;
#else
		// 有點怪怪的 ... 主要問題是在這次的分發中,
		// 是否術科一定是丙案, 如果不是, 那術科的檢定分數 ??

		if (weighted_skill_standard) {
			// 術科檢定按學科數來加權 ...
			skill_check_score = misc->toInt (
				(double) du->dp->ccnt *
				(double) du->st->skillscore);
		} else {
			// 術科檢定直接以術科成績來檢定 ...
			skill_check_score = du->st->skillscore;
		}

#  if DEBUG_LEVEL > 2
		if (skill_check_score != skill_check_score) {
			dprint ("%d (%.2f x %.2f) v.s. %d\n",
					skill_check_score,
					(double) du->st->skillscore,
					(double) du->dp->ccnt,
					skill_check_score);
		}
#  endif
#endif

		if ((du->st->sflag & STUDENT_FLAG_SKILL_MISS) != 0) {
			// 術科有缺考
#if DEBUG_LEVEL > 0
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
					"skill missing!!\n");
			}
#endif
#if ENABLE_POSTCHECK == 1
			pstck->fail_on_skill_miss (
					du->st,
					du->st->wish_index,
					du->dp->id,
					du->dp->skillgroup,
					-1);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_NO_SKILL_ITEM;
#endif
			return 0;
		}

		if (du->dp->checkskill == 0) {
			// 沒有術科檢定
			du->flags |= DUTF_SKILLCHECK_IS_PASS;
			return 1;
		}

		if (skill_check_score <
				standval->checkskill[du->dp->skillgroup]) {
			// 未達術科檢定分數
#if DEBUG_LEVEL > 0
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
					"skill score = %d (%d)\n",
					skill_check_score,
					standval->checkskill[du->dp->skillgroup]
					);
			}
#endif
#if ENABLE_POSTCHECK == 1
			pstck->fail_on_skill_standard (
					du->st,
					du->st->wish_index,
					du->dp->id,
					skill_check_score,
					standval->checkskill[
							du->dp->skillgroup]);
#endif
			// du->dp->reject_cnt++;
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_SKILL_ITEM_NQ;
#endif
			return 0;
#if DEBUG_LEVEL > 0
		} else {
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
					"skill score = %d (%d)\n",
					skill_check_score,
					standval->checkskill[du->dp->skillgroup]
					);
			}
#endif
		}

		du->flags |= DUTF_SKILLCHECK_IS_PASS;
		return 1;
	}

	return (du->flags & DUTF_SKILLCHECK_IS_PASS);
}

static int check_basic (struct dispatcher_unit_t *du) {
	int	i, j, passok;
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
	int	delay_return = 0;
#endif


	// 請注意!! 本段程式碼完全沒有被測試過 ....

	if ((du->flags & DUTF_CHECK1_IS_CHECK) == 0) {
		du->flags |= DUTF_CHECK1_IS_CHECK;

#if DEBUG_LEVEL > 1
		if (du->st->sid == trace_sid) {
			dprint ("CHECK1 check for %d on " WISHID_FMT
					", gradesum = %d (standard=%d)\n",
					du->st->sid, du->dp->id,
					du->st->gradsum,
					standval->gradesum);
		}
#endif

		// 檢查學科能力測驗的一般檢定 ...

		passok = 1; // 先假定通過

		if (du->dp->check1) {
			if ((du->st->score_disallow & STSCD_DISALLOW_BASIC)
							!= 0) {
#if ENABLE_POSTCHECK == 1
				pstck->disallow_on_basic (
					du->st,
					du->st->wish_index,
					du->dp->id);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag |=
							STNQF_SCORE_DISALLOW;
#endif
				return 0;
			}

			// 先踢除掉沒參加學科能力測驗的學生 ...
			if (du->st->gradsum < 0) {
				// du->dp->reject_cnt++;
#if ENABLE_POSTCHECK == 1
				pstck->need_c_score (
					du->st,
					du->st->wish_index,
					du->dp->id);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag |=
							STNQF_BASIC_STANDARD;
				if (check_all_rules) {
					passok = 0;
					// return 0;
				} else {
					return 0;
				}
#else
				return 0;
#endif
			}

			if (du->st->gradsum < standval->gradesum) {
				// 總分未達最低總級分
				// 不過沒關係, 只要各科均達最低級分即可
				
				for (i = 0; i < 5; i++) {
					// 任一科未達最低級分 ...
					if (du->st->gradscore[i] <
							standval->grademin[i]) {
						passok = 0;
						break;
					}
				}

				if (! passok) {
					// 未達最低級分也無所謂,
					// 通過補考即可
					if ((du->st->sflag &
						STUDENT_FLAG_PASS1) != 0) {
						// 連補考都不及格, 那就沒救了
						passok = 1;
#if DEBUG_LEVEL > 1
						if (du->st->sid == trace_sid) {
							dprint ("CHECK1 "
							   "force pass %d\n",
							   trace_sid);
						}
#endif
					}
				}
#if DEBUG_LEVEL > 1
			} else if (du->st->sid == trace_sid) {
				dprint ("CHECK1 check for %d on " WISHID_FMT
					", gradesum = %d >= %d\n",
					du->st->sid, du->dp->id,
					du->st->gradsum,
					standval->gradesum
					);
#endif
			}
		}

#if ENABLE_PRE_TESTING
		if (on_pre_checkall) return passok;
#endif

#if ENABLE_POSTCHECK == 1
		if (! passok) {
			pstck->need_pass_c_score (
					du->st,
					du->st->wish_index,
					du->dp->id);
		}
#endif

#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
		if (check_all_rules && (! passok)) {
			passok = 1;
			delay_return = 1;
			du->st->not_qualify_flag |= STNQF_BASIC_STANDARD;
		}
#endif
		if (passok) {
			// 檢查校系對學測的檢定項目

			for (i = 0; i < 5; i++) {
				if ((j = du->dp->test1[i]) > 0) {// 需要檢定
					j--;
					if (du->st->gradscore[i] <
							standval->grade[i][j]) {
						// 未達 頂、前、均、後、底 標
						passok = 0;
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
						du->st->not_qualify_flag |=
							STNQF_BASIC_CRS(i);
#endif
#if ENABLE_POSTCHECK == 1
						pstck->need_pass_c_standard (
							du->st,
							du->st->wish_index,
							du->dp->id,
							i, j,
							du->st->gradscore[i],
							standval->grade[i][j]
							);
#endif
#if DEBUG_LEVEL > 1
						if (du->st->sid == trace_sid) {
							misc->print (
							      PRINT_LEVEL_DEBUG,
							      "%d on "
							      WISHID_FMT
							      ". Basic %d[%d], "
							      "(%d < %d)[er]\n",
							      trace_sid,
							      du->dp->id,
							      i + 1, j + 1,
							      du->st->
							      	gradscore[i],
							      standval->
							      	grade[i][j]
							);
						}
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
						if (check_all_rules) {
							delay_return = 1;
						} else {
							break;
						}
#else
						break;
#endif
#if DEBUG_LEVEL > 1
					} else {
						if (du->st->sid == trace_sid) {
							misc->print (
							      PRINT_LEVEL_DEBUG,
							      "%d on "
							      WISHID_FMT
							      ". Basic %d[%d], "
							      "(%d>=%d)[ok]\n",
							      trace_sid,
							      du->dp->id,
							      i + 1, j + 1,
							      du->st->
							      	gradscore[i],
							      standval->
							      	grade[i][j]
							);
						}
#endif
					}
				}
			}
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
		} else {
			du->st->not_qualify_flag |= STNQF_BASIC_STANDARD;
#endif
		}

#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
		if (check_all_rules && delay_return) passok = 0;
#endif

#if DEBUG_LEVEL > 1
		if (du->st->sid == trace_sid) {
			dprint ("CHECK1 on " WISHID_FMT " for %d is %spass\n",
					du->dp->id,
					du->st->sid,
					(passok ? "" : "NOT "));

		}
#endif
		if (! passok) {
			/*
			dprint ("basic check require for %d on "
					WISHID_FMT "\r\n",
					du->st->sid,
					du->dp->id);
			dprintw ("%d v.s. %d\r\n",
					du->st->gradsum, standval->gradesum);
			*/
			// du->dp->reject_cnt++;
			return 0;
		} else {
			du->flags |= DUTF_CHECK1_IS_PASS;
			return 1;
		}
	}
	return (du->flags & DUTF_CHECK1_IS_PASS);
}


static int check_exam (struct dispatcher_unit_t *du) {
	int		i, j, k;
	int32_t		testval;
	static const int course[4][7] = {
				{ 1, 2, 4, 5, 6, 0, 0 },
				{ 1, 2, 3, 7, 8, 0, 0 },
				{ 1, 2, 3, 7, 8, 9, 0 },
				{ 1, 2, 3, 8, 9, 0, 0 }
			};
#if DEBUG_LEVEL > 0
	static const char	*std_name[] = { "高", "均", "低" };
	int			delay_break = 0;
#endif

#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
	if (check_all_rules) delay_break = 1;
#endif

	if ((du->flags & DUTF_CHECK2_IS_CHECK) == 0) {
		du->flags |= DUTF_CHECK2_IS_CHECK;

#if DEBUG_LEVEL > 0
		if (du->st->sid == trace_sid) {
			misc->print (PRINT_LEVEL_DEBUG,
				"CHECK2 check for %d on " WISHID_FMT "\n",
					du->st->sid, du->dp->id);
		}
#endif

		if (du->dp->check2) {
#if DEBUG_LEVEL > 8
			int	need_print = 0;
#endif
			k = du->dp->check2 - 1;
			testval = 0;

			if ((du->st->score_disallow & STSCD_DISALLOW_EXAM)
							!= 0) {
#if ENABLE_POSTCHECK == 1
				pstck->disallow_on_exam (
					du->st,
					du->st->wish_index,
					du->dp->id);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag |=
							STNQF_SCORE_DISALLOW;
#endif
				return 0;
			}

			for (j = 0; (i = course[k][j]) != 0; j++) {
				if (du->st->testscore[i-1] < 0) {
#if DEBUG_LEVEL > 0
					if (du->st->sid == trace_sid) {
						misc->print (PRINT_LEVEL_DEBUG,
							"Student %d on "
							WISHID_FMT
							" require %s\r\n",
							du->st->sid,
							du->dp->id,
							dpg_exam_crs[i - 1]);
					}
#endif
#if DEBUG_LEVEL > 0
					if (du->dp->id == trace_wishid) {
						printf (WISHID_FMT
							": %d missing %s\n",
							trace_wishid,
							du->st->sid, 
							dpg_exam_crs[i - 1]);
					}
#endif
#if DEBUG_LEVEL > 8
					if (need_print == 0) {
						dprint ("%d (" WISHID_FMT
							") missing:",
							du->st->sid,
							du->dp->id);
						need_print = 1;
					}
					dprint (" %d", i);
#else
					// du->dp->reject_cnt++;
#  if ENABLE_POSTCHECK == 1
					pstck->missing_crs_on_stdcheck (
						du->st,
						du->st->wish_index,
						du->dp->id,
						du->dp->check2);
#  endif
#  if ENABLE_FULL_FAIL_RESULT_CHECK == 1
					du->st->not_qualify_flag |=
							STNQF_EXAM_STANDARD;

					// du->st->not_qualify_flag |=
					// 			STNQF_EXAM(i);

					if (! check_all_rules) return 0;
#  else
					return 0;
#  endif
#endif
				}
				testval += du->st->testscore[i-1];
			}
#if DEBUG_LEVEL > 8
			if (need_print) {
				dprint ("\n");
				// du->dp->reject_cnt++;
				return 0;
			}
#endif

// #if USE_ADDED_VALUE_AS_CHECK == 1
			// 要以加分後的原始總分來檢定而非原始總分
			du->get_score (du);
			testval = du->addtotoal;
// #endif
			// 未通過一般檢定
			if (testval < standval->check[k]) {
#if DEBUG_LEVEL > 0
				if (du->st->sid == trace_sid) {
					misc->print (PRINT_LEVEL_DEBUG,
						"%d (" WISHID_FMT ") %d < %d"
						" (on standard check %d)\n",
						du->st->sid,
						du->dp->id,
						testval,
						standval->check[k],
						k + 1);
				}
#endif
				// du->dp->reject_cnt++;
#if DEBUG_LEVEL > 0
				if (du->dp->id == trace_wishid) {
					printf (WISHID_FMT
							": %d Not pass %d\n",
						trace_wishid,
						du->st->sid, k);
				}
#endif
#if ENABLE_POSTCHECK == 1
				pstck->need_pass_s_score (
					du->st,
					du->st->wish_index,
					du->dp->id,
					du->dp->check2,
					testval, standval->check[k]);
#endif
#  if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag |= STNQF_EXAM_STANDARD;
				if (! check_all_rules) return 0;
#  else
				return 0;
#  endif
			}
#if DEBUG_LEVEL > 9
			if (du->st->sid == trace_sid) {
				dprint ("%d (" WISHID_FMT ") %d < %d\n",
						du->st->sid,
						du->dp->id,
						testval,
						standval->check[k]);
			}
#endif
		}

		// 需通過某種一般檢定(1, 2, 3, 4)
#if ENABLE_PRE_TESTING
		if (on_pre_checkall) return 1;
#endif

#if DEBUG_LEVEL > 0
		if (du->dp->id == trace_wishid) delay_break = 1;
#endif
		// 檢定高低標 ...
		for (i = 0; i < 9; i++) {
			if ((j = du->dp->test2[i]) == 0) continue;

			j--;

			// 成績是: du->st->testscore[i];
			// 標準是: standval->test[i][j];
			// 檢查學生是否有達到標準
#if DEBUG_LEVEL > 0
			if (du->st->sid == trace_sid) {
				dprint ("Check on course [%s][%s]:",
						dpg_exam_crs[i],
						std_name[j]);
			}
#endif

			if (du->st->testscore[i] < standval->test[i][j]) {
#if DEBUG_LEVEL > 0
				if (du->st->sid == trace_sid) {
					dprint ("no pass (%d < %d)\r\n",
						du->st->testscore[i],
						standval->test[i][j]);
				}

				if (du->dp->id == trace_wishid) {
					printf (WISHID_FMT
							": %d %s [%d < %d]\n",
						trace_wishid,
						du->st->sid,
						dpg_exam_crs[i],
						du->st->testscore[i],
						standval->test[i][j]);
				}
#endif
#if ENABLE_POSTCHECK == 1
				pstck->need_pass_s_standard (
					du->st,
					du->st->wish_index,
					du->dp->id,
					i, j,
					du->st->testscore[i],
					standval->test[i][j]);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag |= STNQF_EXAM(i);
#endif
				// du->dp->reject_cnt++;
				if (! delay_break) return 0;

				delay_break++;
#if DEBUG_LEVEL > 0
			} else if (du->st->sid == trace_sid) {
				dprint ("pass (%d = %d)\r\n",
						du->st->testscore[i],
						standval->test[i][j]);
#endif
			}
		}

#if DEBUG_LEVEL > 0
		if (delay_break > 1) return 0;
#endif
			
		du->flags |= DUTF_CHECK2_IS_PASS;
		return 1;
	}
	return (du->flags & DUTF_CHECK2_IS_PASS);
}

#if ENABLE_DEPARTMENT_FIRST == 1

static void inc_valid_student (struct dispatcher_unit_t *du) {
	du->dp->student_cnt++;
}

#endif


static int32_t min_require (struct dispatcher_unit_t *du) {
	// 為了加速, 先不檢查了
	// if (du == NULL) return INT_MAX;
	// if (du->dp == NULL) return INT_MAX;

	return du->dp->minreq;
}

static int32_t get_score (struct dispatcher_unit_t *du) {
	// 為了加速, 先不檢查了
	// if ((du == NULL) || (st == NULL)) return 0;
	// if (du->dp == NULL) return 0;

	if ((du->flags & DUTF_HAVE_SCORE) == 0) {
		int	i;
		int32_t	orgsum;		// 原始總分
		double	sum;		// 加權總分
		double  addscore;	// 加分
		double	asum;		// 加權總分 + 加分
		double	skillsum;		// 術科總分
		double  ratio;
		double	multiply[] = { 0.0, 1.0, 1.25, 1.5, 1.75, 2.0 };
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
		short	delay_score = 0;
#endif

#if DEBUG_LEVEL > 0
		int	do_debug = 0;

		if (du->st->sid == trace_sid) do_debug = 1;

		// if (du->st->sid == 22034513) do_debug = 1; // 特種生的範例
		//   22052029 與 22030029 同分的例子
		//if (du->st->sid == 22052029) do_debug = 1;
		//if (du->st->sid == 22030029) do_debug = 1;
		//     另一組同分
		// if (du->st->sid == 22001805) do_debug = 1;
		// if (du->st->sid == 22009423) do_debug = 1;
		// 23007242 這個傢伙幫助找到問題
		//if (du->st->sid == 23007242) do_debug = 1;
		//if (du->st->sid == 22138224) do_debug = 1;
		// if (du->st->sid == 22039841) do_debug = 1;
#endif

		sum = asum = skillsum = addscore = 0.0;
		orgsum = 0;
		du->flags |= DUTF_HAVE_SCORE;

#if DEBUG_LEVEL > 0
		if (do_debug) misc->print (PRINT_LEVEL_DEBUG,
				"%d " WISHID_FMT " %3d %d, [", 
				du->st->sid, du->dp->id, du->dp->num,
				du->dp->minreq);
#endif		
	
		for (i = 0; i < 9; i++) {
#if ENABLE_PRE_TESTING
			if (on_pre_checkall) {
				int course[5][9] = {
					{ 0, 0, 0, 0, 0, 0, 0, 0, 0 },
					{ 1, 2, 0, 4, 5, 6, 0, 0, 0 },
					{ 1, 2, 3, 0, 0, 0, 7, 8, 0 },
					{ 1, 2, 3, 0, 0, 0, 7, 8, 9 },
					{ 1, 2, 3, 0, 0, 0, 0, 8, 9 }
				};

				if (course[du->dp->check2][i]) {
					sum += (int32_t) du->st->testscore[i];
					orgsum += (int32_t)du->st->testscore[i];
				}
				continue;
			}
#endif
			if (du->dp->weight[i] != 0) {
				if (du->st->testscore[i] < 0) {
					// 採計的科目不能沒考
#if ENABLE_POSTCHECK == 1
					pstck->need_exam_course (
						du->st,
						du->st->wish_index,
						du->dp->id,
						i);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
					du->st->not_qualify_flag |=
								STNQF_EXAM(i);
					if (check_all_rules) {
						delay_score = 1;
						// return (du->score = -1);
					} else {
						return (du->score = -1);
					}
#else
					return (du->score = -1);
#endif
				}
#if DEBUG_LEVEL > 0
				if (do_debug)
					misc->print (PRINT_LEVEL_DEBUG,
						" %dx%d",
						du->dp->weight[i],
						du->st->testscore[i]);
#endif		

				sum += (double) du->st->testscore[i] *
					multiply[du->dp->weight[i]];
				orgsum += (int32_t) du->st->testscore[i];
			}
		}

#if DEBUG_LEVEL > 0
		if (do_debug) misc->print (PRINT_LEVEL_DEBUG,
						" ] = %.2f\n", sum);
#endif		
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
		if (check_all_rules && delay_score) return (du->score = -1);
#endif

		du->st->org_score = du->orgscore = orgsum;

#if DEBUG_LEVEL > 3
		if (trace_wishid == du->dp->id) {
			dprint ("orgscore = %d\n",
					du->st->org_score);
		}
#endif

#if ROUND_ON_MAIN_WEIGHT == 1
		sum = (double) misc->toInt (sum);
#endif

		// 考慮特種考生的加分方式
		//
		if ((du->st->sflag & STUDENT_FLAG_ADD) != 0) {
			// 考慮增加總分的 ...
			addscore = (double) orgsum *
				(double) du->st->ratio / 100.0;
#if DEBUG_LEVEL > 0
			if (do_debug)
				misc->print (PRINT_LEVEL_DEBUG,
						"add score (%d%%): ",
						du->st->ratio);
#endif
		} else if ((du->st->sflag & STUDENT_FLAG_LOW) != 0) {
			// 考慮降低錄取標準的
			addscore = (double) orgsum * ((double) du->st->ratio /
				   (double) (100 - du->st->ratio));
#if DEBUG_LEVEL > 0
			if (do_debug)
				misc->print (PRINT_LEVEL_DEBUG,
					"lower total (%d%%): ", du->st->ratio);
#endif
		}

		// 給檢定用的, 因為檢定要求是加分後的原始分數 ...
#if USE_ADDED_VALUE_AS_CHECK == 1
		du->addtotoal = misc->toInt (orgsum + addscore);
#else
		du->addtotoal = misc->toInt (orgsum);
#endif

#if ROUND_ON_SPECIAL == 1
		addscore = misc->toInt (addscore);
#endif

#if ROUND_ON_MAIN_WEIGHT_PLUS_SPECIAL == 1
		asum = misc->toInt (sum + addscore);
#else
		asum = sum + addscore;
#endif

#if DEBUG_LEVEL > 0
		if (do_debug) {
			misc->print (PRINT_LEVEL_DEBUG,
				"SUM=%.2f, ORGSUM=%d, ADD=%.2f, REAL=%.2f\n",
				asum, orgsum, addscore, asum + addscore);
		}
#endif

		// 考慮術科的計算方式
		if (du->dp->skillgroup != 0) {
#if ENABLE_DISPATCHER92 == 1
			if (use_disp92) {
#  if ENABLE_PRE_TESTING
				du->skill_score = du->st->skillscore = 0;
#  else
#    if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				if (du->st->skinfo == NULL) {
					if (du->st->not_qualify_flag == 0) {
						fprintf (stderr, "Oops for %d "
							WISHID_FMT "\n",
							du->st->sid,
							du->dp->id);
					}

					du->skill_score =
							du->st->skillscore = 0;
				} else {
					du->skill_score = du->st->skillscore =
						du->st->skinfo->sksc_onwish[
						du->st->wish_index - 1];
				}
#    else
				du->skill_score = du->st->skillscore =
					du->st->skinfo->sksc_onwish[
						du->st->wish_index - 1];
#    endif
#  endif
#if DEBUG_LEVEL > 0
				if (do_debug) {
					misc->print (PRINT_LEVEL_DEBUG,
						"%d skill score for "
						WISHID_FMT
						" %d restore from %d [%p]\n",
						du->st->sid,
						du->dp->id,
						du->skill_score,
						du->st->wish_index - 1,
						du->st->skinfo
					);
				}
#endif
				if (du->st->skillscore < 0) {
					// 術科不得缺考 -- 似乎不必 ...
					// 		 有術科檢定把關
					du->score = -1;
					return -1;
				}

				if (du->dp->skillway == 1) {
					skillsum = (double) du->st->skillscore *
						multiply[du->dp->skillweight]
						/ 100.0;

					du->score = misc->toInt (asum +
								skillsum);
				} else if (du->dp->skillway == 2) {
					//術科佔總成績一定比
					skillsum = (double) du->st->skillscore;

					ratio = (double) du->dp->skill_main +
						(double) du->dp->skill_skill;

					du->score = misc->toInt (
						(
#if ROUND_ON_MAIN_SUBTOTAL == 1
					    misc->toInt
#endif
						 (asum * (double)
						  du->dp->skill_main
						  / ratio) +
#if ROUND_ON_SKILL_SUBTOTAL == 1
					    misc->toInt
#endif
						(skillsum * 
						 (double) du->dp->ccnt *
						 (double) du->dp->skill_skill) /
					        ratio / 100.0)
						);
					if (do_debug) {
						misc->print (PRINT_LEVEL_DEBUG,
							"int((%.2fx%d)+"
							"(%.2fx%.2fx%d)) "
							"= %d\n",
							asum,
							du->dp->skill_main,
							skillsum,
							(double) du->dp->ccnt,
							du->dp->skill_skill,
							du->score);
					}
				} else {
					// oops !!
				}
			} else {
				du->skill_score = du->st->skillscore;
#endif
				if (du->st->skillscore < 0) {
					// 術科不得缺考 -- 似乎不必 ...
					// 		 有術科檢定把關
					du->score = -1;
					return -1;
				}

				if (du->dp->skillway == 1) {	//單科加權計算
					skillsum = (double) du->st->skillscore *
						multiply[du->dp->skillweight];

					du->score = misc->toInt (asum +
								skillsum);
				} else if (du->dp->skillway == 2) {
					//術科佔總成績一定比
					skillsum = (double) du->st->skillscore;

					ratio = (double) du->dp->skill_main +
						(double) du->dp->skill_skill;

					du->score = misc->toInt (
						(
#if ROUND_ON_MAIN_SUBTOTAL == 1
					    misc->toInt
#endif
						 (asum * (double)
						  du->dp->skill_main
						  / ratio) +
#if ROUND_ON_SKILL_SUBTOTAL == 1
					    misc->toInt
#endif
						(skillsum * 
						 (double) du->dp->ccnt *
						 (double) du->dp->skill_skill) /
					        ratio)
						);

#if DEBUG_LEVEL > 0
					if (do_debug) {
						misc->print (PRINT_LEVEL_DEBUG,
							"int((%.2fx%d)+"
							"(%.2fx%.2fx%d)) "
							"= %d\n",
							asum,
							du->dp->skill_main,
							skillsum,
							(double) du->dp->ccnt,
							du->dp->skill_skill,
							du->score);
					}
#endif
				} else {
					// oops ------
				}
#if ENABLE_DISPATCHER92 == 1
			}
#endif
		} else {
			// 沒有術科
			du->score = misc->toInt (sum + addscore);
		}
#if DEBUG_LEVEL > 0
		if (do_debug) {
			misc->print (PRINT_LEVEL_DEBUG,
			  "SUM=%.2f, ORGSUM=%d, ADD=%.2f, SKILL=%.2f, FINAL=%d\n",
				asum, orgsum, addscore, skillsum, du->score);
		}
#endif
	}

	return du->score;
}

static void add_to_stdlist_tail (struct dispatcher_unit_t *du, const int fix) {
	struct student_on_wish_t	*sw;
	int16_t 			idx;
	int				i, k;

	if ((du->flags & DUTF_ADD_TO_STDLIST) != 0) return;

	
	if (fix) {
		// 請注意 ++ 在前 ... 由於演算法先空位的關係, 方便做插入動作 ...
		sw = &du->dp->stdlist[idx = ++du->dp->lastidx];
	} else {
		// 給學系榜用的 ...
		sw = &du->dp->stdlist[idx = du->dp->lastidx++];
	}

	sw->st		= du->st;
	sw->score	= du->score;
	sw->order	= 0;
#if ENABLE_DISPATCHER92 == 1
	// if (use_disp92) {
	// }
	sw->skill_score = du->skill_score;
#endif
#if DEBUG_LEVEL > 1
	if (du->st->sid == trace_sid) {
		misc->print (PRINT_LEVEL_DEBUG,
			"%d on " WISHID_FMT ", Score=%d, Skill=%d\n",
			trace_sid, du->dp->id,
			sw->score, sw->skill_score);
	}
#endif
#if ENABLE_DEPARTMENT_FIRST == 1
	sw->aflag	= 0;	// 學系榜用時當 flag 用, 清空是必要的
	sw->wish_order = du->st->wish_index;
#endif
	du->dp->inlistcnt++;

	du->flags |= DUTF_ADD_TO_STDLIST;

#if DEBUG_LEVEL > 9
	dprint ("on same [ ");
#endif
	// du->get_onsame (du);

	for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
#if DEBUG_LEVEL > 9
		dprint (" %d", du->dp->onsame[i]);
#endif
		switch (k = du->dp->onsame[i]) {
		case 0:	// No
			sw->onsame[i] = 0;
			break;
		case 1: // 總級分
			sw->onsame[i] = du->st->gradsum;
			break;
		case 2: // 志願序 -- 故意做成相反才會有相同的比較方式
			if (no_wish_order) {
				sw->onsame[i] =
					MAX_PER_STUDENT_WISH - 1;
			} else {
				sw->onsame[i] =
					MAX_PER_STUDENT_WISH -
							du->st->wish_index;
			}
			break;
		default:
			if ((k >= 3) && (k <= 7)) {
				k -= 3;
				sw->onsame[i] = du->st->gradscore[k];
			} else if ((k >= 8) && (k <= 16)) {
				k -= 8;
				sw->onsame[i] = du->st->testscore[k];
			} else {
				// sw->onsame[i] = du->st->skillscore;
#if ENABLE_DISPATCHER92 == 1
				if (use_disp92) {
					sw->onsame[i] = 
						misc->toInt ((double)
						du->st->skillscore / 100.0);
				} else {
					sw->onsame[i] = du->st->skillscore;
				}
#else
				sw->onsame[i] = du->st->skillscore;
#endif
			}
			break;
		}
	}
#if DEBUG_LEVEL > 9
	dprint ("]\n");
#endif
}

#if ENABLE_DEPARTMENT_FIRST == 1

// 不排序, 不判斷是否有空間, 直接塞
static void do_accept (struct dispatcher_unit_t *du) {
	du->get_score (du);
	add_to_stdlist_tail (du, 0);
}

static void set_wishidx (struct dispatcher_unit_t *du) {
#if DEBUG_LEVEL > 9
	dprintw ("Student: %d, Wish: %d, Wish: " WISHID_FMT " (%d)\n",
			du->st->sid, du->st->wish_index - 1,
			du->dp->id, du->dp->index);
#endif
	stmptr->set_wishidx (du->st, du->st->wish_index - 1, du->dp->index);
}

#endif

static int try_accept (struct dispatcher_unit_t *du) {
#if DEBUG_LEVEL > 3
	dprint ("To check studet: %d on wish %o\n",
			du->st->sid, du->dp->id);
#endif
	if ((du->flags & DUTF_HAVE_CHECK_ACCEPT) == 0) {
		int32_t	score;
		int32_t	minreq;

		score  = du->get_score (du);
		minreq = du->min_require (du);
#if DEBUG_LEVEL > 1
		if (du->st->sid == trace_sid) {
			misc->print (PRINT_LEVEL_DEBUG,
					"Score=%d, Min-Requirement=%d ... ",
					score, minreq);
		}
#endif

		du->flags |= DUTF_HAVE_CHECK_ACCEPT;

		if (score < minreq) {
#if DEBUG_LEVEL > 1
			if (du->st->sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
						"Not accept\r\n");
			}
#endif
#  if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_SCORE_NOT_ENOUGH;
#  endif
			return 0;
		}

//		if (du->dp->inlistcnt >= du->dp->num) {
//			if (score < minreq) return 0;
//		}

		add_to_stdlist_tail (du, 1);

		switch (dmptr->sort_students (du->dp)) {
		case 0:	// 未入選
			// --du->dp->inlistcnt;
			break;
		case 1: // 暫時入選
		case 2: // 暫時入選, 但有些學生將被擠出
			du->flags |= DUTF_IS_ACCEPT;

			du->st->matched_wish = du->dp->id;
			du->st->final_score  = score;
			break;
		}

#if DEBUG_LEVEL > 1
		if (du->st->sid == trace_sid) {
			if ((du->flags & DUTF_IS_ACCEPT) == 0) {
				misc->print (PRINT_LEVEL_DEBUG,
						"not accept\r\n");
			} else {
				misc->print (PRINT_LEVEL_DEBUG,
						"Accept\r\n");
			}
		}
#endif
	}

	return (du->flags & DUTF_IS_ACCEPT);
}

static struct lost_student_list_t* get_lostlist (struct dispatcher_unit_t *du) {
	return lslptr;
}


struct dispatcher_unit_t *
		allocate_dispatcher_unit (struct department_module_t *dm,
				struct student_module_t *sm) {
	struct dispatcher_unit_t	*ptr;

	if (dm != NULL) dmptr = dm;
	if (dmptr == NULL) dmptr = initial_department_module (0);

	if (sm != NULL) stmptr = sm;
	if (stmptr == NULL) stmptr = initial_student_module ();

	lslptr = initial_lostlist_module ();

	if ((ptr = misc->malloc (sizeof (struct dispatcher_unit_t))) != NULL) {
		ptr->dp			= NULL;
		ptr->st			= NULL;
		ptr->load		= load;
		ptr->load_index		= load_index;
		ptr->load_test		= load_test;
		ptr->check_basic	= check_basic;
		ptr->check_exam		= check_exam;
#if ENABLE_DISPATCHER92 == 1
		if (use_disp92) {
			ptr->check_skill = check_skill92;
		} else {
			ptr->check_skill = check_skill;
		}
#else
		ptr->check_skill	= check_skill;
#endif

		ptr->check_special	= check_special;
		ptr->check_sex		= check_sex;
		ptr->min_require	= min_require;
		ptr->get_score		= get_score;
		ptr->lostlist		= get_lostlist;
		ptr->bind		= bind_student;
		ptr->bind_student	= bind_student_by_id;
		ptr->get_onsame		= get_onsame;
		ptr->try_accept		= try_accept;
#if ENABLE_PRE_TESTING
		ptr->pre_checkall	= pre_checkall;
#endif
#if ENABLE_DEPARTMENT_FIRST == 1
		ptr->load_wish_order	= load_wish_order;
		ptr->do_accept		= do_accept;
		ptr->set_wishidx	= set_wishidx;
		ptr->inc_valid_student	= inc_valid_student;
#endif
	}

#if DEBUG_LEVEL > 0
	// trace_sid = sysconf->getint ("trace-student");
#endif

	return ptr;
}

////////////////////////////////////////////////////////////////

static int load_test (struct dispatcher_unit_t *du, const int wishmin_only) {
	char			*filename;
	int			i, j, len;
	int			replace;
	int16_t			wish;
	int32_t			sid, score, ord;
	int32_t			onsame[MAXIMUM_ON_SAME_REFERENCE];
	char			*buffer;
	struct textfile_io_t	*tio = NULL;
	int			have_onsame;

	if ((filename = sysconfig->getstr ("check-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
		    "%% system variable (check-file) not define!!\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "Load checking file [ %s ] ... ",
			                        filename);
	tio = initial_textfile_io_module ();

        if (tio->open (filename)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < (27+(7*MAXIMUM_ON_SAME_REFERENCE))) {
				if (len < 27) continue;

				have_onsame = 0;
			} else {
				have_onsame = 1;
			}

			wish  = misc->oct (&buffer[0], DEPARTMENT_ID_LENGTH);
			sid   = misc->dec (&buffer[6], STUDENT_ID_LENGTH);
			ord   = misc->dec (&buffer[17], 2);
			score = misc->dec (&buffer[22], 5);

			for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
				if (have_onsame) {
					onsame[i] = misc->dec (
							&buffer[29 + i * 7], 5);
				} else {
					onsame[i] = 0;
				}
			}

			if (! du->bind_student (du, sid)) continue;
			// fprintf (stderr, "%04o %d %d\n", wish, sid, score);

			if (wishmin_only) {
				if (du->load (du, wish)) {
#if ENABLE_POSTCHECK == 1
					pstck->found_match2 (
						du->st,
						ord,
						// du->st->wish_index,
						du->dp->id,
						wish, ord);
#endif
					// du->st->matched_wish = wish;
				}
				continue;
			}

			if (du->load (du, wish)) {
				replace = 0;

				// du->dp->real_num--;

				for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++){
					if (du->dp->onsame[i] == 2) {
						// 志願序 .. patch 一下
						if (no_wish_order) {
							onsame[i] =
							MAX_PER_STUDENT_WISH -
							1;
						} else {
							onsame[i] =
							MAX_PER_STUDENT_WISH -
							onsame[i];
						}
					}
				}

				if ((du->dp->rflag &
						BARF_CHECK_INITIALIZED) == 0) {
					du->dp->rflag |= BARF_CHECK_INITIALIZED;

					replace = 1;
				} else if (du->dp->minreq > score) {
					replace = 1;
				} else if (du->dp->minreq == score) {
					j = misc->compare_list (
						du->dp->min_onsame, onsame,
						MAXIMUM_ON_SAME_REFERENCE);
					if (j > 0) replace = 1;
				}

				if (replace) {
					du->dp->minreq = score;
					misc->copy_list (
						    du->dp->min_onsame, onsame,
						    MAXIMUM_ON_SAME_REFERENCE);
				}
				// 進榜人數, 主要是想知道是否足額錄取
				// 因為驗榜時, 不足額錄取要將 minreq
				// 降為 0
				du->dp->inlistcnt++;

				for (i = 0; i < MAX_PER_STUDENT_WISH; i++) {
					if (wish == du->st->wish[i]) {
						du->st->matched_wish = wish;
						du->st->final_score = score;
						break;
					}
				}
#if ENABLE_POSTCHECK == 1
				//if (i < MAX_PER_STUDENT_WISH) {
					pstck->found_match2 (
						du->st,
						// du->st->wish_index,
						ord,
						// i + 1,
						du->dp->id,
						du->st->matched_wish, ord);
				//}
#endif
			}
		}
		misc->print (PRINT_LEVEL_SYSTEM, "ok\n");
		tio->close ();
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	/*
	for (wish = dmptr->reset_wish (); wish != -1;
					wish = dmptr->next_wish ()) {
		if (du->load (du, wish)) {
			if (du->dp->real_num > 0) {	// 不足額
				du->dp->minreq = 0;
			}
		}
	}
	*/

	return 1;
}
