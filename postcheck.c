/*
 *	postcheck.c
 *
 *	Copyright (c) 2003, Jiann-Ching Liu
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "predefine.h"
#include "dtstruct.h"
#include "global_var.h"
#include "postcheck.h"
#include "student.h"
#include "department.h"
#include "sys_conf.h"
#include "misclib.h"
#include "studlist.h"


typedef int (*cmp_for_qsort)(const void *, const void *);

struct deplist_t {
	struct department_t	*dp;
	int			idx;
};

static struct department_module_t	*dmptr  = NULL;
static struct student_module_t		*smptr	= NULL;
static struct student_t			*std = NULL;
static struct department_t		*dep = NULL;
static int				stdnum = 0;
static int				depnum = 0;

static struct deplist_t			*deplist = NULL;

static struct postcheck_t	internal_pc;
static struct postcheck_t	*pc = NULL;
static FILE			*fp = NULL;
static int			need_close = 0;
static int			pgcnt = 0;
static int			lpp = 54;
struct student_t		*xstudent = NULL;
static char			*title1 =
					"九十二學年度大學分發入學 考生志願複查";
static char			*title2 =
					"九十二學年度大學分發入學 改補分發";
static char			*skill_name[] = { "音樂", "美術", "體育" };
static char			*basic_std[] = { "頂", "前", "均", "後", "底" };
static char			*exam_std[] = { "高", "均", "低" };
static const char		*common_check[] = {
					"零", "一", "二", "三", "四"
				};
static char			*skillgrp_name[] = { "音樂", "美術", "體育" };

static int openfile (const char *filename) {
	if (filename == NULL) {
		fp = fdopen (STDOUT_FILENO, "w");
		need_close = 0;
	} else {
		fp = fopen (filename, "w");
		need_close = 1;
	}

	if (fp == NULL) {
		need_close = 0;
		return 0;
	}

	fprintf (fp,
		"~paper=a4\n"
		"~pageno=0\n"
		"~fn /usr/lib/ttf/bkai00mp.ttf\n"
		"~rgb-save\n");

	return 1;
}

static void closefile (void) {
	if (need_close) {
		if (fp != NULL) fclose (fp);
		need_close = 0;
	}
}

static void finish_page (void) {
	if (fp == NULL) return;

	fprintf (fp, "~print 555,20,10,,,,%4d\n", ++pgcnt);
	fprintf (fp, "~formfeed\n");
}

static void show_pages (void) {
	finish_page ();
}

static void print_check (struct student_t *st, const int i,
					const int k, int *ismatched) {
	struct stdwish_result_t	*rs;
	struct department_t	*dep;
	char			buf[1024];
	int			match_found = 0;

	rs = &st->ck->rs[i];
	// dep = dmptr->find (st->wish[i]);
	dep = dmptr->find (rs->wish);

	fprintf (fp, "~print 25,%d,9,,,,%s%2d. " WISHID_FMT " %s　%s\n",
			k, " ",
			i + 1, rs->wish,
			dep == NULL ? "<Unknow>" : dep->univ->sch_name,
			dep == NULL ? "" : dep->cname
			);
	buf[0] = '\0';

	switch (rs->rcode) {
	case STDWR_FOUND_MATCH:
		if (! *ismatched) match_found = *ismatched = 1;

		sprintf (buf, "<< 錄取  %5.2f >= %5.2f >>",
				(double) rs->args[1] / 100.,
				(double) rs->args[2] / 100.);
		break;
	case STDWR_FOUND_MATCH2:
		if (! *ismatched) match_found = *ismatched = 1;
		sprintf (buf, "<< 分發錄取 >>");
		break;
	case STDWR_CHALLENGE_SCORE:
		if (! *ismatched) match_found = *ismatched = 1;
		sprintf (buf, "<< 達錄取標準 %5.2f > %5.2f >>",
				(double) rs->args[0] / 100.,
				(double) rs->args[1] / 100.);
		break;
	case STDWR_CHALLENGE:
		if (! *ismatched) match_found = *ismatched = 1;
		sprintf (buf, "<< 達錄取標準 (同分參酌) >>");
		break;
	case STDWR_WISH_NOT_EXIST:
		sprintf (buf, "志願不存在");
		break;
	case STDWR_SCORE_NOT_ENOUGH:
		sprintf (buf, "未達錄取標準 (%5.2f < %5.2f)",
				(double) rs->args[0] / 100.,
				(double) rs->args[1] / 100.);
		break;
	case STDWR_FAIL_ONSAME:
		sprintf (buf, "未達錄取標準 (同分參酌第 %d 序,%s)",
				rs->args[0], dpg_onsame_crs [rs->args[1]]);
		break;
	case STDWR_GENDER_NOT_QUALIFY:
		if (rs->args[0] == 1) {
			sprintf (buf, "限男生");
		} else {
			sprintf (buf, "限女生");
		}
		break;
	case STDWR_NORMAL_UNIV_ONLY:
		sprintf (buf, "限師範院校");
		break;
	case STDWR_FAIL_ON_MILITARY:
		sprintf (buf, "未通過國防體檢或智力測驗");
		break;
	case STDWR_NEED_SKILL_GROUP:
		sprintf (buf, "未報考術科(%s)",
				skillgrp_name[rs->args[0] - 1]);
		break;
	case STDWR_SKILL_MAJOR_SCORE:
		sprintf (buf, "術科主修分數不足");
		break;
	case STDWR_FAIL_ON_INSTRUMENT:
		sprintf (buf, "主/副修樂器不符");
		break;
	case STDWR_FAIL_ON_SKILL_MISS:
		sprintf (buf, "術科缺考");
		break;
	case STDWR_FAIL_ON_SKILL_STANDARD:
		sprintf (buf, "術科檢定未通過");
		break;
	case STDWR_NEED_C_SCORE:
		sprintf (buf, "未通過基本學科能力測驗之檢定");
		break;
	case STDWR_NEED_PASS_C_SCORE:
		sprintf (buf, "未通過基本學科能力測驗之檢定");
		break;
	case STDWR_NEED_PASS_C_STANDARD:
		sprintf (buf, "未通學測%s%s標 (%d < %d)",
				dpg_basic_crs[rs->args[0]],
				basic_std[rs->args[1]],
				rs->args[2], rs->args[3]);

		break;
	case STDWR_MISSING_CRS_ON_STDCHECK:
		sprintf (buf, "一般檢定(%s) 部份科目未報考",
				common_check[rs->args[0]]);
		break;
	case STDWR_NEED_PASS_S_SCORE:
		sprintf (buf, "未通過一般檢定(%s) (%5.2f < %5.2f)",
				common_check[rs->args[0]],
				(double) rs->args[1] / 100.,
				(double) rs->args[2] / 100.);
		break;
	case STDWR_NEED_PASS_S_STANDARD:
		sprintf (buf, "指定科目%s未達%s標 (%d < %d)",
				dpg_exam_crs[rs->args[0]],
				exam_std[rs->args[1]],
				rs->args[2], rs->args[3]);
		break;
	case STDWR_NEED_EXAM_COURSE:
		sprintf (buf, "指定科目考試未報考 (%s)",
					dpg_exam_crs[rs->args[0]]);
		break;
	case STDWR_NEED_SKILL_SUB:
		sprintf (buf, "術科(%s)小項未考",
				skillgrp_name[rs->args[0] - 1]);
		break;
	case STDWR_SKILL_SUB_ZERO:
		sprintf (buf, "術科(%s)小項零分",
				skillgrp_name[rs->args[0] - 1]);
		break;
	case STDWR_SKILL_SUB_SCORE:
		sprintf (buf, "術科(%s)小項(%d)分數 %5.2f < %d",
				skillgrp_name[rs->args[0] - 1],
				rs->args[1],
				(double) rs->args[2] / 100.,
				rs->args[3]);
		break;
	default:
		//fprintf (stderr, "[%d] Unknow code\n", rs->rcode);
		break;
	}

	fprintf (fp, "~print 390,%d,8.5,,,,%s\n", k, buf);

	if (dispatching_finalized) {
		if (match_found) {
			fprintf (fp, "~line 30,%d,500,0,0.2\n", k - 1);
		}
	} else {
		if (i + 1 == st->ck->matched_idx) {
			fprintf (fp, "~line 30,%d,500,0,0.2\n", k - 1);
		}
	}
}

static void print_all_check (struct student_t *st, const int num_of_wishes) {
	int		i, k, pgno = 1;
	int		have_matched = 0;

	for (i = 0, k = 530; i < num_of_wishes; i++, k -= 11) {
		if (k < 25) {
			finish_page ();
			k = 770;
			fprintf (fp,
				"~box 20,20,560,800,1.2\n"
				"~line 20,790,560,0,1\n"
				"~print 25,800,13,,,,"
				"准考證號: %d    序號: %s   頁次: %d\n",
				st->sid, st->ck->check_sn, ++pgno);
		}

		print_check (st, i, k, &have_matched);
	}
}

static void start_page (struct student_t *st) {
	char				buf[256];
	int				i, j, k, m, n, num_of_wishes;
	struct student_skill_info_t	*sk = st->skinfo;

	if (fp == NULL) return;

	if (xstudent != NULL) {
		show_pages ();
		xstudent = NULL;
	}

	if (st->ck == NULL) return;
	xstudent = st;

	fprintf (fp,
		"~box 20,20,560,800,1.2\n"
		"~print 100,785,20,,,,%s\n"
		"~line 20,780,560,0,1\n",
		dispatching_finalized ? title2 : title1);

	fprintf (fp, "~print 25,760,14,,,,准考證號:%d\n", st->sid);
	fprintf (fp, "~print 170,760,14,,,,姓名:%s\n",
				st->info == NULL ? "" : st->info->name);
	fprintf (fp, "~print 300,760,14,,,,序號:%s\n", st->ck->check_sn);
	fprintf (fp, "~print 420,760,14,,,,身份:%c\n", st->ptype);
	fprintf (fp, "~print 500,760,14,,,,性別:%s\n",
			(st->sflag & STUDENT_FLAG_GIRL) != 0 ? "女" : "男");

	fprintf (fp, "~line 20,755,560,0,1\n");
	fprintf (fp, "~print 25,735,14,,,,學測分數\n");

	for (i = j = 0; i < 5; i++) {
		j += sprintf (&buf[j], "%s ", dpg_basic_crs[i]);

		if (st->gradscore[i] >= 0) {
			j += sprintf (&buf[j], "%2d   ", st->gradscore[i]);
		} else {
			j += sprintf (&buf[j], "--   ");
		}
	}

	j += sprintf (&buf[j], "總級分 ");

	if (st->gradsum >= 0) {
		j += sprintf (&buf[j], "%2d", st->gradsum);
	} else {
		j += sprintf (&buf[j], "--");
	}

	fprintf (fp, "~print 120,735,13,14,,,%s\n", buf);	// 級分

	for (i = 0, j = 113; i < 6; i++, j += 72) {
		fprintf (fp, "~line %d,732,0,22,1\n", j);
	}

	fprintf (fp,
		"~line 20,732,560,0,1\n"
		"~print 25,700,14,18,,,指科分數\n"
		"~print 120,714,12,14,,,國　文　英　文　數學甲　數學乙　"
		"歷　史　地　理　物　理　化　學　生　物\n");

	fprintf (fp, "~line %d,710,%d,0,1\n", 113, 560 - 113 + 20);
	fprintf (fp, "~line 20,690,560,0,1\n");

	for (i = 0, j = 113; i < 9; i++, j += 52) {
		fprintf (fp, "~line %d,690,0,42,1\n", j);
	}

	for (i = 0, j = 116; i < 9; i++, j += 52) {
		// printf "~print %d,695,14,,,,%5d\n",
		// $j, $ptr->{'s_sco'}->[$i];
		fprintf (fp, "~print %d,695,13,,,,", j);

		if (st->testscore[i] >= 0) {
			fprintf (fp, "%5.2f\n", (double) st->testscore[i]
					/ 100.);
		} else {
			fprintf (fp, "--\n");
		}
	}

	if (use_disp92) {
		if (sk == NULL) {
			fprintf (fp, "~print 25,670,12,14,,,術科組別: --\n");
		} else {

			for (i = j = 0; i < NUMBER_OF_SKILL92; i++) {
				if (st->skill_sid[i] == 0) continue;


				if (j != 0) {
					j += sprintf (&buf[j], " / ");
				}

				j += sprintf (&buf[j], "%s", skill_name[i]);

				if (i < SK92_SUBSCR_SET) {
					for (k = 0; k < SK92_SUBSCR_NUM; k++) {
						if (k != 0) {
							j += sprintf (&buf[j],
									",");
						}

						if (sk->sk_score[i][k] < 0) {
							j += sprintf (&buf[j],
									"--");
						} else if (sk->sk_miss[i][k]) {
							j += sprintf (&buf[j],
									"**");
						} else {
							j += sprintf (&buf[j],
								"%5.2f",
								(double)
							    sk->sk_score[i][k] /
							    100.);
						}
					}

					if (i == 0) {
						j += sprintf (&buf[j],
							" 主:%04d,副:%04d",
							sk->music_major,
							sk->music_minor);
					}
				} else {
					if (sk->physical_score < 0) {
						j += sprintf (&buf[j], "---");
					} else if (sk->physical_miss) {
						j += sprintf (&buf[j], "***");
					} else {
						j += sprintf (&buf[j],
							"%5.2f (級分 %d)",
							(double)
							sk->physical_score /
							100.,
							sk->physical_grade);
					}
				}
			}
			buf[j] = '\0';
			fprintf (fp, "~print 25,670,12,14,,,術科組別:%s\n",
					buf);
		}
	} else {
		// fprintf (fp,
		//	"~print 25,670,12,14,,,術科組別:%s  術科分數:%s   "
		//	"主修:%s  副修:%s  主修分數:%s\n",
		// );
	}

	fprintf (fp,
		"~line 20,665,560,0,1\n"
		"~line 20,663,560,0,1\n");

	if (dispatching_finalized) {
		fprintf (fp, "~print 25,645,12,14,,,改補分發結果: ");
		if (st->ck->new_matched_idx == -1) {
			fprintf (fp, "落榜\n");
		} else {
			struct department_t	*dep;
			int16_t			wishid;

			wishid = st->wish[st->ck->new_matched_idx - 1];
			dep = dmptr->find (wishid);

			fprintf (fp, "第%d志願, " WISHID_FMT " %s %s\n",
				st->ck->new_matched_idx,
				wishid,
				dep == NULL ? "<Unknow>" : dep->univ->sch_name,
				dep == NULL ? "" : dep->cname);
		}
	} else {
		fprintf (fp, "~print 25,645,12,14,,,申請說明: %s\n",
				st->ck->check_description);
	}

	fprintf (fp, "~line 20,642,560,0,1\n");

	// ----------------------------------------------------------

	fprintf (fp, "~print 25,625,12,14,,,原分發結果: ");

	if (st->ck->matched_idx < 0) {
		fprintf (fp, "落榜\n");
	} else {
		struct department_t	*dep;
		int			i, j = 0;
		int16_t			wishid;

		i = st->ck->matched_idx - 1;
		wishid = st->wish[i];

		if (st->org_wish != NULL) {
			if (st->org_wish[i] != wishid) {
				wishid = st->org_wish[i];
				j = 1;
			}
		}

		dep = dmptr->find (wishid);

		fprintf (fp, "%s第%d志願, " WISHID_FMT " %s %s\n",
				j ? "原" : "",
				st->ck->matched_idx,
				wishid,
				dep == NULL ? "<Unknow>" : dep->univ->sch_name,
				dep == NULL ? "" : dep->cname);
	}

	for (num_of_wishes = MAX_PER_STUDENT_WISH;
				num_of_wishes > 0; num_of_wishes--) {
		if (st->wish[num_of_wishes - 1] != 0) break;
	}

	fprintf (fp,
		"~line 20,620,560,0,1\n"
		"~print 25,605,12,,,,考生志願 (共填 %d 個志願):\n",
		num_of_wishes);

	for (i = 0, k = 1, m = 590; i < 4; i++, m -= 11) {
		for (j = 0, n = 30; j < 20; j++, k++, n += 27) {
			if (k > num_of_wishes) break;

			fprintf (fp, "~print %d,%d,9,,,," WISHID_FMT "\n",
				n, m, st->wish[k -1]);

			if (st->org_wish != NULL) {
				if (st->wish[k -1] != st->org_wish[k-1]) {
					fprintf (fp,
						"~box %d,%d,23.8,8.8,0.1\n",
					        n - 1, m);
				}
			}
		}
	}

	fprintf (fp,
		"~line 20,552,560,0,1\n"
		"~line 20,550,560,0,1\n");

	print_all_check (st, num_of_wishes);
}

/////////////////////////////////////////////////////////////////////////

static void pre_range_check (const int i, const char *str) {
	if ((i < 1) || (i > MAX_PER_STUDENT_WISH)) {
		fprintf (stderr, "Out of range [%d] from %s\n", i, str);
	}
}

static void found_match (struct student_t *st,
				const int i, const int wish, const int sc,
				const int mysc, const int reqsc) {
	if (st->ck == NULL) return;

	pre_range_check (i, "found match");

	st->ck->rs[i - 1].rcode   = STDWR_FOUND_MATCH;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = sc;
	st->ck->rs[i - 1].args[1] = mysc;
	st->ck->rs[i - 1].args[2] = reqsc;

	if (st->ck->new_matched_idx == -1) st->ck->new_matched_idx = i;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" %s\n",
				st->sid, i, wish,
				sc ? "matched" : "rematch");
#endif
	}
}

static void found_match2 (struct student_t *st,
				const int i, const int wish,
				const int volun, const int ord) {
	if (st->ck == NULL) return;

	pre_range_check (i, "found match2");

	st->ck->rs[i - 1].rcode   = STDWR_FOUND_MATCH2;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = volun;
	st->ck->rs[i - 1].args[1] = ord;

	st->ck->matched_idx	  = ord;

	// if (st->ck->new_matched_idx == -1) st->ck->new_matched_idx = i;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: MATCHED=" WISHID_FMT ",%d\n",
				st->sid, volun, ord);
#endif
	}
}

static void challenge (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "challenge");

	st->ck->rs[i - 1].rcode   = STDWR_CHALLENGE;
	st->ck->rs[i - 1].wish    = wish;

	if (st->ck->new_matched_idx == -1) st->ck->new_matched_idx = i;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT " challenge\n",
			st->sid, i, wish);
#endif
	}
}

static void challenge_score (struct student_t *st,
				const int i, const int wish,
				const int32_t score, const int32_t minreq) {
	if (st->ck == NULL) return;

	pre_range_check (i, "challenge_score");

	st->ck->rs[i - 1].rcode   = STDWR_CHALLENGE_SCORE;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = score;
	st->ck->rs[i - 1].args[1] = minreq;

	if (st->ck->new_matched_idx == -1) st->ck->new_matched_idx = i;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" challenge [%d > %d]\n",
			st->sid, i, wish,
			score, minreq);
#endif
	}
}

/////////////////////////////////////////////////////////////////////////

static void wish_not_exists (struct student_t *st,
					const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "wish_not_exists");

	st->ck->rs[i - 1].rcode   = STDWR_WISH_NOT_EXIST;
	st->ck->rs[i - 1].wish    = wish;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d,"
			WISHID_FMT " not exists\n",
			st->sid, i, wish);
#endif
	}
}

static void score_not_enough (struct student_t *st,
				const int i, const int wish,
				const int32_t score, const int32_t minreq) {
	if (st->ck == NULL) return;

	pre_range_check (i, "score_not_enough");

	st->ck->rs[i - 1].rcode   = STDWR_SCORE_NOT_ENOUGH;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = score;
	st->ck->rs[i - 1].args[1] = minreq;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d,"
			WISHID_FMT " score not "
			"enough [%d < %d]\n",
			st->sid,
			st->wish_index,
			wish, score, minreq);
#endif
	}
}

static void fail_onsame (struct student_t *st,
				const int i, const int wish,
				const int level, const int oscrs) {
	if (st->ck == NULL) return;

	pre_range_check (i, "fail_onsame");

	st->ck->rs[i - 1].rcode   = STDWR_FAIL_ONSAME;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = level;
	st->ck->rs[i - 1].args[1] = oscrs;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT " onsame=%d,%d\n",
				st->sid, i, wish, level, oscrs);
#endif
	}
}


static void gender_not_qualify (struct student_t *st,
				const int i, const int wish, const int req) {
	if (st->ck == NULL) return;

	pre_range_check (i, "gender_not_qualify");

	st->ck->rs[i - 1].rcode   = STDWR_GENDER_NOT_QUALIFY;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = req;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT " sex need %s\n",
			st->sid, i, wish,
			(req == 1) ? "boy" : "girl");
#endif
	}
}

static void normal_univ_only (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "normal_univ_only");

	st->ck->rs[i - 1].rcode   = STDWR_NORMAL_UNIV_ONLY;
	st->ck->rs[i - 1].wish    = wish;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
						" normal university only\n",
			st->sid, i, wish);
#endif
	}
}

static void fail_on_military (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "fail_on_military");

	st->ck->rs[i - 1].rcode   = STDWR_FAIL_ON_MILITARY;
	st->ck->rs[i - 1].wish    = wish;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT " military fail\n",
			st->sid, i, wish);
#endif
	}
}

static void need_skill_group (struct student_t *st,
				const int i, const int wish,
				const int grp, const int mygrp) {
	if (st->ck == NULL) return;

	pre_range_check (i, "need_skill_group");

	st->ck->rs[i - 1].rcode   = STDWR_NEED_SKILL_GROUP;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = grp;
	st->ck->rs[i - 1].args[1] = mygrp;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		if (mygrp == 0) {
			fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
						" need skill group %d\n",
				st->sid, i, wish, grp);
		} else {
			fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
						" skill group %d (%d)\n",
				st->sid, i, wish, grp, mygrp);
		}
#endif
	}
}
static void skill_major_score (struct student_t *st,
				const int i, const int wish,
				const int mysc, const int reqsc) {
	if (st->ck == NULL) return;

	pre_range_check (i, "skill_major_score");

	st->ck->rs[i - 1].rcode   = STDWR_SKILL_MAJOR_SCORE;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = mysc;
	st->ck->rs[i - 1].args[1] = reqsc;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
					" skill major score %d < %d\n",
				st->sid, i, wish,
				mysc, reqsc);
#endif
	}
}

static void fail_on_instrument (struct student_t *st,
				const int i, const int wish,
				const int mj, const int mi) {
	if (st->ck == NULL) return;

	pre_range_check (i, "fail_on_instrument");

	st->ck->rs[i - 1].rcode   = STDWR_FAIL_ON_INSTRUMENT;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = mj;
	st->ck->rs[i - 1].args[1] = mi;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
					" Major/Minor=%04d/%04d\n",
				st->sid, i, wish,
				mj, mi);
#endif
	}
}

static void fail_on_skill_miss (struct student_t *st,
				const int i, const int wish,
				const int grp, const int crs) {
	if (st->ck == NULL) return;

	pre_range_check (i, "fail_on_instrument");

	st->ck->rs[i - 1].rcode   = STDWR_FAIL_ON_SKILL_MISS;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = grp;
	st->ck->rs[i - 1].args[1] = crs;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		if (crs == -1) {
			fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" skill missing %d\n",
				st->sid, i, wish, grp);
		}
#endif
	}
}

static void fail_on_skill_standard (struct student_t *st,
				const int i, const int wish,
				const int scr, const int stscr) {
	if (st->ck == NULL) return;

	pre_range_check (i, "fail_on_skill_standard");

	st->ck->rs[i - 1].rcode   = STDWR_FAIL_ON_SKILL_STANDARD;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = scr;
	st->ck->rs[i - 1].args[1] = stscr;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" skill score %d < %d\n",
				st->sid, i, wish, scr, stscr);
#endif
	}
}

static void need_c_score (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "need_c_score");

	st->ck->rs[i - 1].rcode   = STDWR_NEED_C_SCORE;
	st->ck->rs[i - 1].wish    = wish;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT " need c_score\n",
				st->sid, i, wish);
#endif
	}
}

static void need_pass_c_score (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "need_pass_c_score");

	st->ck->rs[i - 1].rcode   = STDWR_NEED_PASS_C_SCORE;
	st->ck->rs[i - 1].wish    = wish;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" need pass c_score\n",
				st->sid, i, wish);
#endif
	}
}

static void need_pass_c_standard (struct student_t *st,
				const int i, const int wish,
				const int crs, const int crsst,
				const int scr, const int stdscr) {
	if (st->ck == NULL) return;

	pre_range_check (i, "need_pass_c_standard");

	st->ck->rs[i - 1].rcode   = STDWR_NEED_PASS_C_STANDARD;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = crs;
	st->ck->rs[i - 1].args[1] = crsst;
	st->ck->rs[i - 1].args[2] = scr;
	st->ck->rs[i - 1].args[3] = stdscr;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" basic std %d:%d  [%d < %d]\n",
				st->sid, i, wish,
				crs + 1, crsst + 1,
				scr, stdscr);
#endif
	}
}

static void missing_crs_on_stdcheck (struct student_t *st,
				const int i, const int wish, const int chk) {
	if (st->ck == NULL) return;

	pre_range_check (i, "missing_crs_on_stdcheck");

	st->ck->rs[i - 1].rcode   = STDWR_MISSING_CRS_ON_STDCHECK;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = chk;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" missing score on standard check %d\n",
				st->sid, i, wish, chk);
#endif
	}
}

static void need_pass_s_score (struct student_t *st,
				const int i, const int wish, const int chk,
				const int mysc, const int stdsc) {
	if (st->ck == NULL) return;

	pre_range_check (i, "need_pass_s_score");

	st->ck->rs[i - 1].rcode   = STDWR_NEED_PASS_S_SCORE;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = chk;
	st->ck->rs[i - 1].args[1] = mysc;
	st->ck->rs[i - 1].args[2] = stdsc;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" not pass on standard check %d (%d < %d)\n",
				st->sid, i, wish, chk, mysc, stdsc);
#endif
	}
}

static void need_pass_s_standard (struct student_t *st,
				const int i, const int wish,
				const int crs, const int crsst,
				const int scr, const int stdscr) {
	if (st->ck == NULL) return;

	pre_range_check (i, "need_pass_s_standard");

	st->ck->rs[i - 1].rcode   = STDWR_NEED_PASS_S_STANDARD;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = crs;
	st->ck->rs[i - 1].args[1] = crsst;
	st->ck->rs[i - 1].args[2] = scr;
	st->ck->rs[i - 1].args[3] = stdscr;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" exam std %d:%d  [%d < %d]\n",
				st->sid, i, wish,
				crs + 1, crsst + 1,
				scr, stdscr);
#endif
	}
}

static void need_exam_course (struct student_t *st,
				const int i, const int wish,
				const int crs) {
	if (st->ck == NULL) return;

	pre_range_check (i, "need_exam_course");

	st->ck->rs[i - 1].rcode   = STDWR_NEED_EXAM_COURSE;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = crs;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" need exam course %d\n",
				st->sid, i, wish,
				crs + 1);
#endif
	}
}

static void need_skill_sub (struct student_t *st,
					const int i, const int wish,
					const int grp, const int si) {
	if (st->ck == NULL) return;

	pre_range_check (i, "need_skill_sub");

	st->ck->rs[i - 1].rcode   = STDWR_NEED_SKILL_SUB;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = grp;
	st->ck->rs[i - 1].args[1] = si;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" need skill sub\n",
				st->sid, i, wish);
#endif
	}
}

static void skill_sub_zero (struct student_t *st,
					const int i, const int wish,
					const int grp, const int si) {
	if (st->ck == NULL) return;

	pre_range_check (i, "skill_sub_zero");

	st->ck->rs[i - 1].rcode   = STDWR_SKILL_SUB_ZERO;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = grp;
	st->ck->rs[i - 1].args[1] = si;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" kill sub zero\n",
				st->sid, i, wish);
#endif
	}
}

static void skill_sub_score (struct student_t *st,
					const int i, const int wish,
					const int grp, const int si,
					const int mysc, const int stdsc) {
	if (st->ck == NULL) return;

	pre_range_check (i, "skill_sub_score");

	st->ck->rs[i - 1].rcode   = STDWR_SKILL_SUB_SCORE;
	st->ck->rs[i - 1].wish    = wish;
	st->ck->rs[i - 1].args[0] = grp;
	st->ck->rs[i - 1].args[1] = si;
	st->ck->rs[i - 1].args[2] = mysc;
	st->ck->rs[i - 1].args[3] = stdsc;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" kill sub score\n",
				st->sid, i, wish);
#endif
	}
}

static void disallow_on_basic (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "disallow_on_basic");

	st->ck->rs[i - 1].rcode   = STDWR_DISALLOW_ON_BASIC;
	st->ck->rs[i - 1].wish    = wish;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" disallow on basic\n",
				st->sid, i, wish);
#endif
	}
}

static void disallow_on_exam (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "disallow_on_exam");

	st->ck->rs[i - 1].rcode   = STDWR_DISALLOW_ON_EXAM;
	st->ck->rs[i - 1].wish    = wish;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" disallow on exam\n",
				st->sid, i, wish);
#endif
	}
}

static void disallow_on_skill (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	pre_range_check (i, "disallow_on_skill");

	st->ck->rs[i - 1].rcode   = STDWR_DISALLOW_ON_SKILL;
	st->ck->rs[i - 1].wish    = wish;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
		fprintf (cklfp, "%8d: WISH=%2d," WISHID_FMT
				" disallow on skill\n",
				st->sid, i, wish);
#endif
	}
}

#if 0
static void template (struct student_t *st,
				const int i, const int wish) {
	if (st->ck == NULL) return;

	if (fp != NULL) {
#if ENABLE_CHECKLIST == 1
	} else {
#endif
	}
}
#endif

static int order_by_maximal (struct deplist_t *p1, struct deplist_t *p2) {
	struct department_t	*d1, *d2;
	int			s1, s2;

	d1 = p1->dp;
	d2 = p2->dp;

	s1 = d1->take_sort;
	s2 = d2->take_sort;
	if (d1->skillgroup != 0) s1 = 7;
	if (d2->skillgroup != 0) s2 = 7;

	if (d1->deplim_grp == d2->deplim_grp) {
		if (d1->number_of_refcrs == 0) {
			if (d1->min_gradesum < d2->min_gradesum) {
				return 1;
			} else if (d1->min_gradesum > d2->min_gradesum) {
				return -1;
			}
		} else {
			if (d1->max_orgsum < d2->max_orgsum) {
				return 1;
			} else if (d1->max_orgsum > d2->max_orgsum) {
				return -1;
			}
		}
	} else {
		if ((s1 < 5) && (s2 >= 5)) {
			return -1;
		} else if ((s1 >= 5) && (s2 < 5)) {
			return 1;
		} else {
			return (d1->deplim_grp > d2->deplim_grp) ? 1 : -1;
		}
	}


	return 0;
}

static int order_by_average (struct deplist_t *p1, struct deplist_t *p2) {
	struct department_t	*d1, *d2;
	int			s1, s2;

	d1 = p1->dp;
	d2 = p2->dp;

	s1 = d1->take_sort;
	s2 = d2->take_sort;
	if (d1->skillgroup != 0) s1 = 7;
	if (d2->skillgroup != 0) s2 = 7;

	if (d1->deplim_grp == d2->deplim_grp) {
		if (d1->number_of_refcrs == 0) {
			if (d1->min_gradesum < d2->min_gradesum) {
				return 1;
			} else if (d1->min_gradesum > d2->min_gradesum) {
				return -1;
			}
		} else {
			if (d1->avg_orgsum_n < d2->avg_orgsum_n) {
				return 1;
			} else if (d1->avg_orgsum_n > d2->avg_orgsum_n) {
				return -1;
			}
		}
	} else {
		if ((s1 < 5) && (s2 >= 5)) {
			return -1;
		} else if ((s1 >= 5) && (s2 < 5)) {
			return 1;
		} else {
			return (d1->deplim_grp > d2->deplim_grp) ? 1 : -1;
		}
	}

	return 0;
}

static int order_by_minimal (struct deplist_t *p1, struct deplist_t *p2) {
	struct department_t	*d1, *d2;
	int			s1, s2;

	d1 = p1->dp;
	d2 = p2->dp;
	s1 = d1->take_sort;
	s2 = d2->take_sort;
	if (d1->skillgroup != 0) s1 = 7;
	if (d2->skillgroup != 0) s2 = 7;

	if (d1->deplim_grp == d2->deplim_grp) {
		if (d1->number_of_refcrs == 0) {
			if (d1->min_gradesum < d2->min_gradesum) {
				return 1;
			} else if (d1->min_gradesum > d2->min_gradesum) {
				return -1;
			}
		} else {
			if (d1->min_orgsum_n < d2->min_orgsum_n) {
				return 1;
			} else if (d1->min_orgsum_n > d2->min_orgsum_n) {
				return -1;
			}
		}
	} else {
		if ((s1 < 5) && (s2 >= 5)) {
			return -1;
		} else if ((s1 >= 5) && (s2 < 5)) {
			return 1;
		} else {
			return (d1->deplim_grp > d2->deplim_grp) ? 1 : -1;
		}
	}

	return 0;
}

static int order_by_not_qualify (struct deplist_t *p1, struct deplist_t *p2) {
	struct department_t	*d1, *d2;

	d1 = p1->dp;
	d2 = p2->dp;

	return d2->n_qualify_cnt - d1->n_qualify_cnt;
}

static int order_by_reject (struct deplist_t *p1, struct deplist_t *p2) {
	struct department_t	*d1, *d2;

	d1 = p1->dp;
	d2 = p2->dp;

	return d2->reject_cnt - d1->reject_cnt;
}

static void sort_deplist (const int stype) {

	cmp_for_qsort	funclist[] = {
		(cmp_for_qsort) order_by_minimal,	// 0
		(cmp_for_qsort) order_by_average,	// 1
		(cmp_for_qsort) order_by_maximal,	// 2
		(cmp_for_qsort) order_by_not_qualify,	// 3
		(cmp_for_qsort) order_by_reject,	// 4
	};

	if ((stype < 0) ||
			(stype >= (sizeof funclist / sizeof (cmp_for_qsort)))) {
		return;
	}

	qsort (deplist, depnum,
			sizeof (struct deplist_t),
			funclist[stype]);
//			(int (*)(const void *, const void *)) order_by_minimal);
}

static void print_mscboard (const int ptype) {
	FILE			*fp = NULL;
	char			*output_file = NULL;
	int			i, j, k, m, n, p;
	int16_t			wish;
	struct ncu_depname_t	*depna_ptr;
	struct department_t	*dept;
	int			last_st, last_gp;
	int			have_sort = 0;
	int			linecnt = 0;
	int			stcnt = 0;

	for (i = 0; i < depnum; i++) {
		deplist[i].dp  = &dep[i];
		deplist[i].idx = i;
	}

	for (wish = dmptr->reset_wish (); wish != -1;
					wish = dmptr->next_wish ()) {
		dmptr->calculate_statistic (wish);
	}

	switch (ptype) {
	case 0:	// no
		break;
	case 1: // no-sort
		break;
	case 2: // sort
		have_sort = 1;
		switch (x_option) {
		case 1: // MIN
			sort_deplist (0);
			break;
		case 2:	// AVG
			sort_deplist (1);
			break;
		case 3: // MAX
			sort_deplist (2);
			break;
		default:
			sort_deplist (0);
			break;
		}
		break;
	case 3: // no-match
		// have_sort = 1;
		sort_deplist (3);
		break;
	case 4: // less
		sort_deplist (4);
		// have_sort = 1;
		// sort_deplist ();
		break;
	case 5:
	case 6:
		break;
	}

	if ((output_file = sysconfig->getstr ("msc-board-file")) != NULL) {
		if ((fp = fopen (output_file, "w")) == NULL) {
			perror (output_file);
			output_file = NULL;
		}
	}

	if (fp == NULL) fp = stdout;

	fprintf (fp, "~landscape 1\n"
			"~paper b4\n"
			"~fs 11\n"
			"~fn /usr/lib/ttf/avmfv.ttf\n"
			"~chgcmd ~!#\n"
			"~!#rgb-save 1\n"
			"~!#inlinecmd 1\n");

	last_st = last_gp = -1;

	for (i = k = m = 0; i < depnum; i++) {
		dept = deplist[i].dp;

		if (last_gp != dept->deplim_grp) {
			if ((last_st <= 4) && (last_st == dept->take_sort)) {
			} else if (have_sort) {
				if (i > 0) {
					k = 0;

					if (last_st <= 4) {
						if (linecnt % lpp != 0) {
							fprintf (fp,
							      "~!#formfeed\n");
						}
						linecnt = 0;
					} else {
						linecnt += 4;
						fprintf (fp,
							"\n\n------------"
							"---------------------"
							"---------------------"
							"\n\n");
					}
				}

				if (linecnt > lpp - 10) {
					linecnt = 0;
					fprintf (fp, "~!#formfeed\n");
				}

				stcnt = 0;
				fprintf (fp, "採計[%d]:", ++m);
				//		dept->take_sort,
				//		dept->deplim_grp);

				for (j = 0; j < 9; j++) {
					if (dept->weight[j] > 0) {
						fprintf (fp, " %s",
							dpg_exam_crs[j]);
					}
				}

				++linecnt; ++linecnt;
				if (x_option > 0) {
					fprintf (fp,
						"\n---------------------------"
						"-----------------------------"
						"----------------------"
						"[MIN]   [AVG]   [MAX]  |  "
						"[DIFF] [N]   [ESD]"
						"\n");
				} else {
					fprintf (fp,
						"\n-----------------------\n");
				}
			}
			last_st = dept->take_sort;
			last_gp = dept->deplim_grp;
		}

		if (ptype == 0) {
			if (dept->num == dept->inlistcnt) continue;
		} else if (ptype == 5) {
			if (dept->max_onsame_use == 0) continue;
		}
		//fprintf (fp, WISHID_FMT "\n", dep[j].id);
		//dept->univ->schid,

		++linecnt;
		++stcnt;

		if (ptype <= 5) {
			fprintf (fp, "%4d. ", ++k);

			if (myschool_id == dept->univ->schid) {
				fprintf (fp, "~!#setrgb 0xff0000;");
			}

			fprintf (fp, "%-*s %-*s ",
				(int) sizeof depna_ptr->sch_name,
				dept->univ->sch_name,
				(int) sizeof depna_ptr->dep_name,
				dept->cname);

			if (myschool_id == dept->univ->schid) {
				fprintf (fp, "~!#rgb-restore 1;");
			}

			if ((x_option > 0) && (x_option <= 3)) {
				int	n;

				if (x_option == 1) {
					fprintf (fp, "~!#setrgb 0x0000ff;");
				}
				fprintf (fp, "%6.2f  ",
					(double) dept->min_orgsum_n / 100.);
				if (x_option == 1) {
					fprintf (fp, "~!#rgb-restore 1;");
				}

				if (x_option == 2) {
					fprintf (fp, "~!#setrgb 0x0000ff;");
				}
				fprintf (fp, "%6.2f  ",
					(double) dept->avg_orgsum_n / 100.);
				if (x_option == 2) {
					fprintf (fp, "~!#rgb-restore 1;");
				}

				if (x_option == 3) {
					fprintf (fp, "~!#setrgb 0x0000ff;");
				}
				fprintf (fp, "%6.2f ",
					(double) dept->max_orgsum / 100.);
				if (x_option == 3) {
					fprintf (fp, "~!#rgb-restore 1;");
				}

				fprintf (fp, "| %6.2f ",
					(double) (dept->max_orgsum -
						 dept->min_orgsum_n) / 100.);

				n = dept->inlistcnt - dept->number_of_special;

				fprintf (fp, " %3d ", n);
				if (n > 1) {
					fprintf (fp, "%7.2f",
						sqrt (dept->variation /
							(double)(n - 1)));
				} else {
					fprintf (fp, "--------");
				}
			} else {
				fprintf (fp,
					"%6.2f %2d %3d %6.2f",
					(double) dept->min_orgsum_n / 100.,
					dept->min_gradesum,
					dept->number_of_special,
					(double) dept->min_orgsum / 100.);
			}
		} else {
			fprintf (fp,
				"%5d. %-*s %-*s %6.2f",
				++k,
				(int) sizeof depna_ptr->sch_name,
				dept->univ->sch_name,
				(int) sizeof depna_ptr->dep_name,
				dept->cname,
				(double) dept->minreq / 100.);

			if (dept->inlistcnt == 0) {
				fprintf (fp, "[%6.2f] ", 0.0);

				for (n = 0; n <
					       MAXIMUM_ON_SAME_REFERENCE; n++) {
					fprintf (fp, " -----");
				}
			} else {
				fprintf (fp, "[%6.2f] ",
						(double)dept->stdlist[0].score
						/100. );

				p = dept->lastidx - 1;

				for (n = 0; n <
					       MAXIMUM_ON_SAME_REFERENCE; n++) {
					if (dept->onsame[n] == 2) {
						fprintf (fp, " %5d",
						    MAX_PER_STUDENT_WISH -
						    dept->stdlist[0].onsame[n]);
					} else {
						fprintf (fp, " %5d",
						    dept->stdlist[p].onsame[n]);
					}
				}
				// fprintf (fp, " %5d", dept->min_onsame[n]);
			}
		}

		if (ptype == 5) {
			fprintf (fp, " [%d]", dept->max_onsame_use);

			p = dept->lastidx - 1;

			for (n = 0; n < MAXIMUM_ON_SAME_REFERENCE; n++) {
				if (dept->onsame[n] == 2) {
					fprintf (fp, " %5d",
						MAX_PER_STUDENT_WISH -
						dept->stdlist[p].onsame[n]);
				} else {
					fprintf (fp, " %5d",
						dept->stdlist[p].onsame[n]);
				}
				// fprintf (fp, " %5d", dept->min_onsame[n]);
			}
		} else if ((ptype < 5) && (x_option == 0)) {
			fprintf (fp, " [%3d/%3d]",
					dept->inlistcnt,
					dept->num);
			fprintf (fp, " %5d %5d",
					dept->reject_cnt,
					dept->n_qualify_cnt);
		}

		/*
		fprintf (fp, "[%d][%2d]",
				dept->take_sort,
				dept->deplim_grp);
				*/

		fprintf (fp, "\n");
	}

	if ((fp != NULL) && (output_file != NULL)) fclose (fp);
}

static void write_news_media (void) {
	// FILE			*fa, *fb, *fc;
	FILE			*fp;
	int			i, k;
	struct department_t	*dept;
	struct student_t	*st;

	/*
	fa = fopen ("fa92.dat", "w");
	fb = fopen ("fb92.dat", "w");
	fc = fopen ("fc92.dat", "w");
	*/
	fp = fopen ("www_res92.dat", "w");

	// --------------------------------------------------------------

	for (k = 0; k < depnum; k++) {
		dept = &dep[k];

		// fprintf (fa, " " WISHID_FMT "\r\n", dept->id);
		for (i = 0; i < dept->lastidx; i++) {
			st = dept->stdlist[i].st;
			if (st->usedid[0] > 0) {
				fprintf (fp, "%8d", st->usedid[0]);
			} else {
				fprintf (fp, "        ");
			}

			if (st->usedid[1] > 0) {
				fprintf (fp, "%8d", st->usedid[1]);
			} else {
				fprintf (fp, "        ");
			}

			fprintf (fp, WISHID_FMT "%-12s\r\n", dept->id,
					st->info->name);
		}
	}

	// --------------------------------------------------------------

	/*
	fclose (fa);
	fclose (fb);
	fclose (fc);
	*/
	fclose (fp);
}
#define LNPstr(x)	(strnstr (x, sizeof x))

static char *strnstr (char *fmt, const int length) {
	static char	data[16][128];
	static int	sel = 0;
	int		len = length;

	sel = (sel + 1) % 16;

	if (len > 127) len = 127;

	memcpy (data[sel], fmt, len);
	data[sel][len] = '\0';

	return data[sel];
}

static void write_adm_format (struct student_t *st,
				struct department_t *dp, FILE *fp) {
	int				i, j;
	struct student_skill_info_t	*sk;

	if (highsch_only && (st->highsch != highsch_only)) return;

	sk = st->skinfo;

	fprintf (fp, "%03d%s%8d%-12s%d%s%06d%s%s%s%s%s%s%s%c ",
			dp->univ->schid,
			LNPstr (st->infol2->regist_sn),
			st->sid,
			st->infol2->name,
			((st->sflag & STUDENT_FLAG_GIRL) == 0) ? 1 : 2,
			LNPstr (st->infol2->person_id),
			st->infol2->birthday,
			LNPstr (st->infol2->address),
			LNPstr (st->infol2->guard_man),
			LNPstr (st->infol2->zip_code),
			LNPstr (st->infol2->cmaddr),
			LNPstr (st->infol2->tel_no),
			LNPstr (st->infol2->edu_level),
			LNPstr (st->infol2->edu_year),
			st->ptype);

	for (i = 0; i < 5; i++) {
		if (st->gradmiss[i]) {
			fprintf (fp, "**");
		} else if (st->gradscore[i] < 0) {
			fprintf (fp, "--");
		} else {
			fprintf (fp, "%2d", st->gradscore[i]);
		}
	}

	for (i = 0; i < 9; i++) {
		if (st->testmiss[i]) {
			fprintf (fp, "*****");
		} else if (st->testscore[i] < 0) {
			fprintf (fp, "-----");
		} else {
			fprintf (fp, "%5d", st->testscore[i]);
		}
	}

	if (sk == NULL) {
		for (i = 0; i < SK92_SUBSCR_SET; i++) {
			for (j = 0; j < SK92_SUBSCR_NUM; j++) {
				fprintf (fp, "-----");
			}
		}
		fprintf (fp, "-----");
		fprintf (fp, "    ");
		fprintf (fp, "    ");
	} else {
		for (i = 0; i < SK92_SUBSCR_SET; i++) {
			for (j = 0; j < SK92_SUBSCR_NUM; j++) {
				if (sk->sk_miss[i][j]) {
					fprintf (fp, "*****");
				} else if (sk->sk_score[i][j] < 0) {
					fprintf (fp, "-----");
				} else {
					fprintf (fp, "%5d", sk->sk_score[i][j]);
				}
			}
		}

		if (sk->physical_miss) {
			fprintf (fp, "*****");
		} else if (sk->physical_score < 0) {
			fprintf (fp, "-----");
		} else {
			fprintf (fp, "%5d", sk->physical_score);
		}

		if (sk->music_major == 0) {
			fprintf (fp, "    ");
		} else {
			fprintf (fp, "%04d", sk->music_major);
		}

		if (sk->music_minor == 0) {
			fprintf (fp, "    ");
		} else {
			fprintf (fp, "%04d", sk->music_minor);
		}
	}

	fprintf (fp, "%5d%2d" WISHID_FMT "%-26s%-44s\r\n",
			st->final_score, st->wish_index, dp->id,
			dp->univ->sch_name,
			dp->cname);
}

static void write_adm_media (void) {
	FILE			*fp = NULL;
	int			i, k;
	struct student_t	*st;
	struct department_t	*ptr;
	struct student_list_t	*sl;
	char			filename[64];
	char			dirname[64];
	int			lastuid = -1;

	// sprintf (dirname, "92adm.%d", getpid ());
	sprintf (dirname, "92adm");

	mkdir (dirname, 0755);

	misc->print (PRINT_LEVEL_SYSTEM,
			"Write for University on [ %s ] ... ", dirname);


	for (k = 0; k < depnum; k++) {
		ptr = &dep[k];

		if (ptr->univ->schid != lastuid) {
			sprintf (filename, "%s/92adm%03d.txt",
					dirname, ptr->univ->schid);

			if (fp != NULL) {
				fclose (fp);
				fp = NULL;
			}

			if ((fp = fopen (filename, "w")) == NULL) {
				misc->perror (PRINT_LEVEL_SYSTEM);
				continue;
			}

			lastuid = ptr->univ->schid;
		}

		for (i = 0; i < ptr->lastidx; i++) {
			st = ptr->stdlist[i].st;
			write_adm_format (st, ptr, fp);
		}

		for (sl = ptr->lastlist; sl != NULL; sl = sl->next, i++) {
			st = sl->st;
			write_adm_format (st, ptr, fp);
		}
	}

	if (fp != NULL) {
		fclose (fp);
		fp = NULL;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "ok.\r\n");
}

static void write_media (const int mtype) {
	switch (mtype) {
	case 1:
		write_news_media ();
		break;
	case 2:
		write_adm_media ();
		break;
	}
}

static void write_resultcheck (struct student_t *st,
					const int i, const int wish) {
	int32_t		mask = 1;
	static FILE	*fp = NULL;
	static int	have_open = 0;
	char		*checkfile = "resultcheck.txt";

	if (fp == NULL) {
		if (have_open == 0) {
			have_open = 1;
			if ((fp = fopen (checkfile, "w")) == NULL) {
				perror (checkfile);
				return;
			}
		} else {
			return;
		}
	} else if (st == NULL) {
		if (have_open) {
			fclose (fp);
			fp = NULL;
			return;
		}
	}

	if (st->ck == NULL) return;

	fprintf (fp, "%08d" WISHID_FMT, st->sid, wish);

	for (mask = STNQF_WISH_NOT_FOUND; mask <= STNQF_INSTRUMENT;
								mask <<= 1) {
		fprintf (fp, "%d",
				((st->not_qualify_flag & mask) == 0) ? 0 : 1);
	}
	fprintf (fp, "\r\n");
}

struct postcheck_t * init_postcheck_module (struct student_module_t *st,
				struct department_module_t *dp) {
	if (pc == NULL) {
		pc = &internal_pc;

		smptr	= st;
		dmptr	= dp;

		std     = st->get_stdbuf ();
		stdnum  = st->number_of_students ();
		dep     = dp->get_depbuf ();
		depnum  = dp->number_of_departments ();

		if ((deplist = misc->malloc (depnum *
					sizeof (struct deplist_t))) == NULL) {
			misc->perror (PRINT_LEVEL_SYSTEM);
		}

		pc->open	= openfile;
		pc->close	= closefile;
		pc->start	= start_page;
		pc->finish	= finish_page;

		pc->wish_not_exists		= wish_not_exists;
		pc->score_not_enough		= score_not_enough;
		pc->found_match			= found_match;
		pc->found_match2		= found_match2;
		pc->fail_onsame			= fail_onsame;
		pc->challenge			= challenge;
		pc->challenge_score		= challenge_score;
		pc->gender_not_qualify		= gender_not_qualify;
		pc->normal_univ_only		= normal_univ_only;
		pc->fail_on_military		= fail_on_military;
		pc->need_skill_group		= need_skill_group;
		pc->skill_major_score		= skill_major_score;
		pc->fail_on_instrument		= fail_on_instrument;
		pc->fail_on_skill_miss		= fail_on_skill_miss;
		pc->fail_on_skill_standard	= fail_on_skill_standard;
		pc->need_c_score		= need_c_score;
		pc->need_pass_c_score		= need_pass_c_score;
		pc->need_pass_c_standard	= need_pass_c_standard;
		pc->missing_crs_on_stdcheck	= missing_crs_on_stdcheck;
		pc->need_pass_s_score		= need_pass_s_score;
		pc->need_pass_s_standard	= need_pass_s_standard;
		pc->need_exam_course		= need_exam_course;
		pc->need_skill_sub		= need_skill_sub;
		pc->skill_sub_zero		= skill_sub_zero;
		pc->skill_sub_score		= skill_sub_score;
		pc->disallow_on_basic		= disallow_on_basic;
		pc->disallow_on_exam		= disallow_on_exam;
		pc->disallow_on_skill		= disallow_on_skill;
		pc->print_mscboard		= print_mscboard;
		pc->write_media			= write_media;
		pc->write_resultcheck		= write_resultcheck;

		// if (dmptr == NULL) dmptr = initial_department_module (0);
	}

	return pc;
}
