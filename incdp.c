/*
 *	incdp.c
 *
 *	Copyright (c) 2003, Jiann-Ching Liu
 *
 *	Incremental dispatching algorithm
 */

#include "predefine.h"

#if ENABLE_INCREMENTAL_ALGORITHM == 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "global_var.h"
#include "misclib.h"
#include "student.h"
#include "department.h"
#include "dispunit.h"
#include "sys_conf.h"
#include "stdvalue.h"

struct student_volun_t {
	//int32_t			onsame[MAXIMUM_ON_SAME_REFERENCE];
	//int32_t			skill_score;
	int8_t		qflag;
};

struct department_state_t {
	int		num_qualified;
	int		affect;	//influence;	// affect;
	int32_t		mflag;
	int32_t		dstate;
};

struct scr_sid_t {
	int32_t		sid;
	int		score;
};

struct deplim_grp_t {
	int			score[9];
	int			skill;
	int			index;
	struct scr_sid_t	*list;
	int			depcnt;
	int			crscnt;
	int			scnt[100];
	int			numcnt;
	int			dscnt[3];
};

#define STVQ_REJECT		0
#define STVQ_QUALIFIED		1
#define STVQ_SAFE		2
#define STVQ_BOUND		3
#define STVQ_FINAL		4

#define DPST_FREE		0
#define DPST_FINAL		1

static FILE			*logfp = NULL;

static struct student_t		*std = NULL;
static struct department_t	*dep = NULL;
static int			stdnum = 0;
static int			depnum = 0;

static struct student_module_t		*st = NULL;
static struct department_module_t	*dp = NULL;
static struct dispatcher_unit_t		*du = NULL;

static struct student_volun_t		**stv = NULL;
struct student_on_wish_t		*stdlist = NULL;
static int				stlidx = 0;
static int				have_safe, have_bound, have_final;
static int				have_changed;
static int				dep_final;
static struct department_state_t	*dstlist = NULL;

static int cmp_scrsid (struct scr_sid_t *p, struct scr_sid_t *q) {
	return q->score - p->score;
}

static int count_all_score_level (const int sql_out) {
	int				ii, i, j, k, m, n, acc, skg, found;
	int32_t				bitmap, skmap, gsort[256];
	int32_t				sum, sksc;
	int				gsort_sort[256];
	int				sidx = 0;
	int				ssidx = 0;
	int				sz_sort = sizeof (gsort) /
							sizeof (int32_t);
	struct student_t		*student;
	struct deplim_grp_t		*dlmg;
	struct student_skill_info_t	*sk;
	int				tkst[] = { 0, 2, 2, 2, 2, 0, 1 };
						// 0  1  2  3  4  5  6
	char				xbuf[64];
	char				rbuf[16];
	int				xidx;
	char				*scn_list[] = {
						"國文", "英文",
						"數學甲",
						"數學乙",
						"歷史", "地理",
						"物理", "化學", "生物"
					};
	char				*skn_list[] = {
						"音樂", "美術", "體育"
					};

	misc->print (PRINT_LEVEL_SYSTEM, "Analysis deplim ... ");

	for (i = 0; i < depnum; i++) {
		bitmap = 0;
		skmap  = 0;

		for (j = 0; j < 9; j++) {
			bitmap <<= 1;

			if (dep[i].weight[j] != 0) bitmap |= 1;
		}

		if (dep[i].skillgroup > 0) {
			skmap = 1;
			skmap <<= (12 + dep[i].skillgroup);
			bitmap |= skmap;
		}

		for (j = found = 0; j < sidx; j++) {
			if (gsort[j] == bitmap) {
				found = 1;
				break;
			}
		}

		dep[i].deplim_grp = j;

		if (! found) {
			if (sidx < sz_sort - 1) {
				gsort_sort[sidx] = sidx;
				gsort[sidx++] = bitmap;
				if (dep[i].skillgroup > 0) ssidx++;
			} else {
				fprintf (stderr, "Too many group\n");
			}
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%d group (%d + %d)\n",
				sidx, sidx - ssidx, ssidx);

	if ((dlmg = misc->malloc (sidx * sizeof (
					struct deplim_grp_t))) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM, "Out of memory\n");
		return 0;
	}

	for (i = 0; i < sidx; i++) {
		if ((dlmg[i].list = misc->malloc (
				stdnum * sizeof (struct scr_sid_t))) == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM, "Out of memory\n");
			return 0;
		}
		dlmg[i].index  = 0;
		dlmg[i].depcnt = 0;
		dlmg[i].crscnt = 0;
		dlmg[i].numcnt = 0;
		dlmg[i].dscnt[0] = dlmg[i].dscnt[1] = dlmg[i].dscnt[2] = 0;

		for (j = 0; j < 100; j++) dlmg[i].scnt[j] = 0;

		bitmap = gsort[i];
		skmap = bitmap >> 13;
		bitmap &= 0xfff;

		for (j = 0; skmap > 0; j++) skmap >>= 1;

		if ((dlmg[i].skill = j) > 0) dlmg[i].crscnt++;

		// printf ("%03x, SK=%x\n", bitmap, j);

		for (j = 0, skmap = 0x100; j < 9; j++) {
			dlmg[i].score[j] = ((bitmap & skmap) != 0) ? 1 : 0;
			if (dlmg[i].score[j]) dlmg[i].crscnt++;
			skmap >>= 1;
		}
	}

	for (i = 0; i < depnum; i++) {
		j = dep[i].deplim_grp;
		k = tkst[dep[i].take_sort];

		dlmg[j].depcnt++;
		dlmg[j].numcnt += dep[i].num;
		dlmg[j].dscnt[k]++;
	}

	for (k = 0; k < sidx; k++) {
		for (i = 0; i < stdnum; i++) {
			student = &std[i];

			if ((student->sflag & STUDENT_FLAG_HAVE_SCORE) == 0) {
				if ((dlmg[k].skill > 0) &&
						(dlmg[k].crscnt == 1)) {
					if ((student->sflag &
						STUDENT_FLAG_HAVE_CEEC_BASIC)
							== 0) {
						continue;
					}
				} else {
					continue;
				}
			}

			if ((student->score_disallow & STSCD_DISALLOW_EXAM) 
							!= 0) {
				// 違規不算
				// continue;
				if ((dlmg[k].skill > 0) &&
						(dlmg[k].crscnt == 1)) {
					if ((student->sflag &
						STUDENT_FLAG_HAVE_CEEC_BASIC)
							== 0) {
						continue;
					}
				} else {
					continue;
				}
			}

			for (j = 0, sum = 0; j < 9; j++) {
				if (dlmg[k].score[j]) {
					if (student->testscore[j] < 0) {
						sum = -1;
						break;
					// } else if (student->testmiss[j]) {
						// 缺考不算 - 不合理 !!
					//	sum = -1;
					//	break;
					}

					sum += student->testscore[j];
				}
			}

			if (sum < 0) continue;

			if ((skg = dlmg[k].skill) > 0) {
				if (use_disp92) {
					skg--;
					if ((sk = student->skinfo) == NULL) {
						continue;
					}
					// if (student->skill_sid[skg] == 0) {
					if (sk->skill_sid[skg] == 0) {
						continue;
					}

					if (skg < SK92_SUBSCR_SET) {
						sksc = 0;
						for (m = n = 0;
							m < SK92_SUBSCR_NUM;
									m++) { 
							if (sk->sk_score[skg]
								     [m] > 0) {
								sksc +=
								   sk->sk_score
								     [skg][m];
								n++;
							} else if (sk->sk_score[
								skg][m] == 0) {
								if (sk->sk_miss[
								   skg][m] != 0)
									n++;
							}
						}
						sum += misc->toInt (
							((double) sksc) /
							(double) n);
					} else {
						if (sk->physical_score < 0) {
							continue;
						}
						sum += sk->physical_score;
					}
				} else if (student->skill_group ==
								dlmg[k].skill) {
					if (student->skillscore >= 0) {
						sum += student->skillscore;
					} else {
						continue;
					}
				} else {
					continue;
				}
			}

			dlmg[k].list[dlmg[k].index].sid = student->sid;
			dlmg[k].list[dlmg[k].index].score = sum;
			dlmg[k].index++;
			// printf ("%2d. %6d\n", k, sum);

			if (dlmg[k].crscnt > 0) {
				if (sum == 0) {
					j = 0;
				} else {
					j = (int) ((double) (sum - 1) / 100.
						/ (double) dlmg[k].crscnt);
				}
				// if (j == 100) j = 99;

				if (j > 99 || j < 0) {
					fprintf (stderr,
						"Oops [%d]! out of range\n", j);
				} else {
					dlmg[k].scnt[j]++;
				}
			}
		}
	}

	for (i = 0; i < sidx - 1; i++) {
		for (k = i, j = i + 1; j < sidx ; j++) {
			if (gsort[gsort_sort[k]] > gsort[gsort_sort[j]]) {
				k = j;
			}
		}

		if (k != i) {
			j = gsort_sort[i];
			gsort_sort[i] = gsort_sort[k];
			gsort_sort[k] = j;
		}
	}

	if (sql_out) {
		if (sql_out == 1) {
			printf ("DROP TABLE IF EXISTS deplim_group,"
					"score_table,dep_limgrp;\n\n");
			printf ("CREATE TABLE deplim_group (\n"
				"grpid INT NOT NULL,\n"
				"crs1 INT, crs2 INT, crs3 INT,\n"
				"crs4 INT, crs5 INT, crs6 INT,\n"
				"crs7 INT, crs8 INT, crs9 INT,\n"
				"skill INT, tkst1 INT, tkst2 INT, tkst3 INT,\n"
				"ndep INT, nnum INT, crsnum INT, "
				"PRIMARY KEY (grpid)\n"
				");\n\n");

			printf ("CREATE TABLE score_table (\n"
				"grpid INT NOT NULL,\n"
				"sc_range INT NOT NULL,\n"
				"snum INT, spst CHAR(8), sumnum INT, "
				"sumpst CHAR(8),\n"
				"PRIMARY KEY (grpid, sc_range));\n\n");

			printf ("CREATE TABLE dep_limgrp (\n"
				"sn INT NOT NULL,\n"
				"grpid INT NOT NULL, PRIMARY KEY (sn));\n\n");
		}

		for (ii = 0; ii < sidx; ii++) {
			i = gsort_sort[ii];
			j = dlmg[i].index;

			xbuf[(xidx = 0)] = '\0';

			qsort (dlmg[i].list, j, sizeof (struct scr_sid_t),
				(int (*)(const void *, const void *)) 
				cmp_scrsid);

			if (sql_out == 1) {
				printf ("INSERT INTO deplim_group VALUES (%d",
					ii + 1);
			}

			for (k = 0; k < 9; k++) {
				if (sql_out == 1) {
					printf (",%d",
						dlmg[i].score[k] ? 1 : 0);
				} else {
					if (dlmg[i].score[k]) {
						if (xidx != 0) {
							xidx += sprintf (
							    &xbuf[xidx], "、");
						}

						xidx += sprintf (&xbuf[xidx],
							"%s", scn_list[k]);
					}
				}
			}

			if ((sql_out != 1) && (dlmg[i].skill != 0)) {
				if (xidx != 0) {
					xidx += sprintf ( &xbuf[xidx], "、");
				}

				xidx += sprintf (&xbuf[xidx], "%s",
						skn_list[dlmg[i].skill -1 ]);
			}

			if (sql_out == 1) {
				printf (",%d,%d,%d,%d,%d,%d,%d);\n",
					dlmg[i].skill,
					dlmg[i].dscnt[0],
					dlmg[i].dscnt[1],
					dlmg[i].dscnt[2],
					dlmg[i].depcnt,
					dlmg[i].numcnt,
					dlmg[i].crscnt);
			}

			for (j = k = 0; j < depnum; j++) {
				if (dep[j].deplim_grp == i) {
					if (sql_out == 1) {
						printf ("INSERT INTO dep_limgrp"
							" VALUES (%d,%d);\n",
							dep[j].id,
							ii + 1);
					}
				}
			}

			for (acc = 0, j = 99, k = 0; j >= 0; j--) {
				acc += dlmg[i].scnt[j];

				if (sql_out == 1) {
					printf ("INSERT INTO score_table "
						"VALUES (%d,%d,%d,"
						"'%5.3f',%d,'%5.3f');\n",
						ii + 1, j,
						dlmg[i].scnt[j],
						dlmg[i].scnt[j] * 100. /
								dlmg[i].index,
						acc,
						acc * 100. / dlmg[i].index);
				} else {
					printf ("%02d%-60s", ii + 1, xbuf);
#define PATCH_FOR_CCY	1

#if PATCH_FOR_CCY == 1
					if (j == 0) {
					    sprintf (rbuf, "0 - %d",
						(j + 1) * dlmg[i].crscnt);
					} else {
					    sprintf (rbuf, "%d.0%d - %d",
						j * dlmg[i].crscnt,
						j ? 1 : 1,
						(j + 1) * dlmg[i].crscnt);
					}
#else
					sprintf (rbuf, "%d.0%d - %d.00",
						j * dlmg[i].crscnt,
						j ? 1 : 0,
						(j + 1) * dlmg[i].crscnt);
#endif
					printf ("%-15s", rbuf);
					printf ("%6d%6.2f%6d%6.2f\r\n",
						dlmg[i].scnt[j],
						dlmg[i].scnt[j] * 100. /
								dlmg[i].index,
						acc,
						acc * 100. / dlmg[i].index);
				}
				k++;
			}
		}
		return 1;
	}

	for (ii = 0; ii < sidx; ii++) {
		i = gsort_sort[ii];

		j = dlmg[i].index;

		qsort (dlmg[i].list, j, sizeof (struct scr_sid_t),
				(int (*)(const void *, const void *))
				cmp_scrsid);

		printf ("GRP <%2d>: ", ii + 1);
		for (k = 0; k < 9; k++) {
			if (dlmg[i].score[k]) {
				printf (" %s", dpg_exam_crs[k]);
			}
		}
		printf (", SKILL: %d (CRS: %d, Student: %d, "
				"DEP:%d(%d,%d,%d), NUM=%d)\n",
					dlmg[i].skill,
					dlmg[i].crscnt,
					dlmg[i].index,
					dlmg[i].depcnt,
					dlmg[i].dscnt[0],
					dlmg[i].dscnt[1],
					dlmg[i].dscnt[2],
					dlmg[i].numcnt);

		for (j = k = 0; j < depnum; j++) {
			if (dep[j].deplim_grp == i) {
				if (++k % 12 == 0) { k = 1; printf ("\n"); }
				printf ("[" WISHID_FMT "]", dep[j].id);
			}
		}
		printf ("\n");

#if 1
		if ((dlmg[i].crscnt >= 5) && (dlmg[i].skill == 0)) {
			for (j = 0; j < 5; j++) {
				printf ("Pos %3d. %d <%d>\n", j + 1,
						dlmg[i].list[j].score,
						dlmg[i].list[j].sid);
			}
		}
#endif

		for (acc = 0, j = 99, k = 0; j >= 0; j--) {
			if (k == 0 && dlmg[i].scnt[j] == 0) continue;

			acc += dlmg[i].scnt[j];

			printf ("%2d.%4d.01 ~%4d %7d, %7.3f%%, %6d, %7.3f%%\n",
					ii + 1,
					j * dlmg[i].crscnt,
					(j + 1) * dlmg[i].crscnt,
					dlmg[i].scnt[j],
					dlmg[i].scnt[j] * 100. / dlmg[i].index,
					acc,
					acc * 100. / dlmg[i].index
					);
			k++;
		}
	}

	return 1;
}

static void initialize_all_students (void) {
	int			i;
	struct student_t	*student;
	int32_t			wish;

	for (i = 0; i < stdnum; i++) {
		student = &std[i];
		student->stdstate = STDSTATE_FREE;

		if (student->wishidx == NULL) {
			student->stdstate = STDSTATE_REJECT;
			continue;
		}

		st->reset_wish (student);
		du->bind (du, student);
		// dp->clear_use_log ();

		while ((wish = st->nextwish (student)) != -1) {
			if (wish <= 0) continue;
			if (! du->load (du, wish)) continue;

			//if (dp->test_and_set (du->dp->index)) {
				du->set_wishidx (du);
			//}
			du->set_wishidx (du);
		}
	}
}

static int load_student_wish (const int k) {
	int			rc = 0;
	int			i, j, m, n;
	struct student_t	*student;


	for (i = 0; i < stdnum; i++) {
		student = &std[i];

		if ((student->stdstate & STDSTATE_REJECT) != 0) continue;
		if ((student->stdstate & STDSTATE_HAVE_SAFE) == 0) continue;
		if ((student->stdstate & STDSTATE_BOUND) != 0) continue;

		if ((j = student->wishidx[k]) < 0) continue;

		if (stv[i][j].qflag == STVQ_REJECT) continue;

		if (stv[i][j].qflag == STVQ_SAFE) {
			stv[i][j].qflag = STVQ_BOUND;

			if ((student->stdstate & STDSTATE_BOUND) == 0) {
				student->stdstate |= STDSTATE_BOUND;
				have_bound++;
				rc |= 1;
			}

			for (m = 0; m < depnum; m++) dstlist[m].mflag = 1;
			for (m = 0; m <= k; m++) {
				if ((n = student->wishidx[m]) < 0) continue;
				dstlist[n].mflag = 0;
			}

			for (m = 0; m < depnum; m++) {
				// 因為已經篤定錄取，所以把佔名額的部份去掉
				if (! dstlist[m].mflag) continue;
				if (stv[i][m].qflag == STVQ_REJECT) continue;

				stv[i][m].qflag = STVQ_REJECT;
				dstlist[m].affect = 1;
				have_changed = 1;
			}

			if (k == std[i].max_possible) {
				stv[i][j].qflag = STVQ_FINAL;
				student->stdstate |= STDSTATE_FINAL;
				have_final++;
				rc |= 2;

				if (logfp != NULL) {
					fprintf (logfp,
						"[3] STD Final: %d, %2d, "
						WISHID_FMT "\n",
						std[i].sid, k,
						dep[j].id
					);
				}
			} else {
				if (logfp != NULL) {
					fprintf (logfp,
						"[3] STD Bound: %d, %2d, "
						WISHID_FMT "\n",
						std[i].sid, k,
						dep[j].id
					);
				}
			}
		}
	}

	return rc;
}

static void write_result (void) {
	int	i, j;
	char	*output_file;
	FILE	*fp = NULL;

	if ((output_file = sysconfig->getstr ("output-file")) != NULL) {
		if ((fp = fopen (output_file, "w")) == NULL) {
			misc->print (PRINT_LEVEL_SYSTEM,
				"%s: ", output_file);
			misc->perror (PRINT_LEVEL_SYSTEM);
			return;
		}
	}

	for (j = 0; j < depnum; j++) {
		for (i = 0; i < stdnum; i++) {
			if (stv[i][j].qflag == STVQ_REJECT) continue;

			fprintf (fp, WISHID_FMT ", %d\n",
				dep[j].id,
				std[i].sid);
		}
	}

	fclose (fp);
}

static int examine_wish (const int k, const int phase) {
	struct student_on_wish_t	*sw;
	struct student_t		*student;
	int				i, j, m, n, x;
	int				ndep;
	int32_t				score;
	int				save_dep_final;

	have_changed = 0;
	save_dep_final = dep_final;

	misc->print (PRINT_LEVEL_SYSTEM,
			"ON wish %2d (phase %2d)... ", k + 1, phase);

	if (logfp != NULL) {
		fprintf (logfp, "[+] ON wish %2d (phase %2d)\n", k + 1, phase);
	}

	misc->timer_started ();

	for (j = ndep = 0; j < depnum; j++) {
		if (dstlist[j].dstate != DPST_FREE) continue;
		if (! dstlist[j].affect) continue;

		dstlist[j].affect = 0;

		ndep++;

		fprintf (stderr, WISHID_FMT "\b\b\b\b", dep[j].id);
		// 計算
		for (i = stlidx = 0; i < stdnum; i++) {
			if (stv[i][j].qflag == STVQ_REJECT) continue;

			student = &std[i];

			du->st->wish_index = 0;
			du->bind (du, student);
			du->load_index (du, j);
			score = du->get_score (du);

			sw = &stdlist[stlidx++];

			sw->id    = i;
			sw->score = score;

			for (m = 0; m < MAXIMUM_ON_SAME_REFERENCE; m++) {
				switch (n = du->dp->onsame[m]) {
				case 0: // No
					sw->onsame[m] = 0;
					break;
				case 1: // 總級分
					sw->onsame[m] = du->st->gradsum;
					break;
				case 2: // 志願序
					sw->onsame[m] =
						MAX_PER_STUDENT_WISH - 1;
					// if (no_wish_order) {
					// }
					break;
				default:
					if ((n >= 3) && (n <= 7)) {
						n -= 3;
						sw->onsame[m] =
							du->st->gradscore[n];
					} else if ((n >= 8) && (n <= 16)) {
						n -= 8;
						sw->onsame[m] =
							du->st->testscore[n];
					} else if (n > 16) {
						sw->onsame[m] =
							use_disp92 ?
							misc->toInt ((double)
	                                                du->st->skillscore /
							100.0) :
							du->st->skillscore;
					}
					break;
				}
			}
		}

		qsort (stdlist, stlidx,
				sizeof (struct student_on_wish_t),
				(int (*)(const void *, const void *))
				dp->compare_order);

		n = dep[j].num;
		n = stlidx < n ? stlidx : n;

		for (m = x = 0; m < n; m++) {
			i = stdlist[m].id;

			/*
			if (stv[i][j].qflag == STVQ_REJECT) {
				fprintf (stderr, "Oops on %d %d\n", i, j);
			}
			*/

			switch (stv[i][j].qflag) {
			case STVQ_QUALIFIED:
				stv[i][j].qflag = STVQ_SAFE;

				if ((std[i].stdstate &
						STDSTATE_HAVE_SAFE) == 0) {
					std[i].stdstate |= STDSTATE_HAVE_SAFE;
					have_safe++;

					if (logfp != NULL) {
						fprintf (logfp,
							"[1] STD Safe: %d\n",
							std[i].sid
						);
					}
				}
					/*
					if (logfp != NULL) {
						fprintf (logfp,
							"[1] STD Safe: %d, "
							WISHID_FMT "\n",
							std[i].sid,
							dep[j].id
						);
					}
					*/
				break;
			case STVQ_SAFE:
				break;
			case STVQ_BOUND:
				break;
			case STVQ_FINAL:
				x++;
				break;
			}
		}

		if ((n != 0) && (x == n)) {
			dstlist[j].dstate = DPST_FINAL;
			dep_final++;
			have_changed = 1;

			if (logfp != NULL) {
				fprintf (logfp, "[4] DEP Final: "
					WISHID_FMT ", %d\n",
					dep[j].id, x
				);
			}
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%4d,[%d]", ndep, have_safe);
	// if (logfp != NULL) fprintf (logfp, "[%d]", have_safe);


	if (k < MAX_PER_STUDENT_WISH) {
		// if (save_dep_final != dep_final) {
		// }

		for (i = 0; i <= k; i++) {
			for (i = 0; i < stdnum; i++) {
				if (std[i].wishidx == NULL) continue;

				for (m = 0; m < k; m++) {
					if ((j = std[i].wishidx[m]) < 0) {
						continue;
					}
					if (dstlist[j].dstate == DPST_FREE) {
						break;
					}

					std[i].max_possible = m + 1;
				}
			}

			load_student_wish (k);
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM,
				"(%d,%d,%d,%d) ok [ %.02f sec ]\r\n",
				have_safe, have_bound, have_final,
				dep_final, misc->timer_ended ());

	if (logfp != NULL) {
		fprintf (logfp, "[-] ON wish %2d (%2d)"
			"(S=%d,B=%d,F=%d,DF=%d)\r\n",
			k + 1, phase,
			have_safe, have_bound, have_final, dep_final);
		fflush (logfp);
	}

	return have_changed;
}

static int remove_safe_but_not_bound (void) {
	int			i, j;
	int			rc = 0;
	struct student_t	*student;


	for (i = 0; i < stdnum; i++) {
		student = &std[i];

		if ((student->stdstate & STDSTATE_REJECT) != 0) continue;
		if ((student->stdstate & STDSTATE_HAVE_SAFE) == 0) continue;
		if ((student->stdstate & STDSTATE_BOUND) != 0) continue;

		for (j = 0; j < depnum; j++) {
			if (stv[i][j].qflag == STVQ_SAFE) {
				stv[i][j].qflag = STVQ_REJECT;
				dstlist[j].affect = 1;
				rc++;
			}
		}
	}

	return rc;
}

int incdp_main (struct student_module_t *p_st,
					struct department_module_t *p_dp,
					struct dispatcher_unit_t *p_du,
					const int clevel, const int qflag) {
	struct student_t	*student;
	int			i, j, k, cnt, maxcnt;
	int32_t			score;

	st = p_st;
	dp = p_dp;
	du = p_du;

	// fprintf (stderr, "Incremental Dispatching Algorithm\n");

	if (! st->load_all_students (0)) return 0;
	if (! st->load_info   (0)) return 0;

	if (clevel == 0) if (! st->load_wishes (0, 0)) return 0;

	if (! st->load_score  (0)) return 0;

	if (! dp->load_limits ()) return 0;
	dp->load_skillset ();

	if ((standval = initial_standard_values ()) == NULL) {
		standval = misc->calloc (1, sizeof (struct standard_values_t));
	}

	std	= st->get_stdbuf ();
	stdnum	= st->number_of_students ();
	dep	= dp->get_depbuf ();
	depnum	= dp->number_of_departments ();

	// if (! st->prepare_sorted_stack (0)) return 1;

	if (clevel) return count_all_score_level (qflag);

	// --------------------------------------------------------

	if ((dstlist = misc->malloc (sizeof (struct department_state_t) *
				depnum)) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM, "Out of memory (0)");
		return 0;
	} else {
		for (i = 0; i < depnum; i++) {
			dstlist[i].num_qualified = 0;
			dstlist[i].dstate = DPST_FREE;
		}
	}

	// --------------------------------------------------------

	if ((stv = misc->malloc (sizeof (struct student_volun_t *) *
						stdnum)) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM, "Out of memory (1)");
		return 0;
	} else {
		// print_memory ();
		for (i = 0; i < stdnum; i++) {
			if ((stv[i] = misc->malloc (sizeof 
					(struct student_volun_t) * depnum))
					== NULL) {
				misc->print (PRINT_LEVEL_SYSTEM,
							"Out of memory (2)");
				return 0;
			}

			for (j = 0; j < depnum; j++) {
				stv[i][j].qflag = STVQ_REJECT;
			}
		}

		print_memory ();
	}
	
	misc->print (PRINT_LEVEL_SYSTEM,
			"\r\nExamine all students on all departments ... ");
	misc->timer_started ();

	for (i = cnt = 0; i < stdnum; i++) {
		student = &std[i];
		student->max_possible = 0;

		if ((student->sflag & STUDENT_FLAG_DISALLOW) != 0) continue;

		du->bind (du, student);

		for (j = 0; j < depnum; j++) {
			du->st->wish_index = 0;

			if (! du->load_index	(du, j)) continue;
			// -------------------------------------------
			if (! du->check_sex	(du)) continue;
			if (! du->check_skill	(du)) continue;
			if (! du->check_special	(du)) continue;
			if (! du->check_basic	(du)) continue;
			if (! du->check_exam	(du)) continue;
			if ((score = du->get_score (du)) < 0) continue;
			// -------------------------------------------
			cnt++;
			stv[i][j].qflag = STVQ_QUALIFIED;
			dstlist[j].num_qualified++;
		}
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"%d [ %.02f second(s) ]\r\n",
			cnt, misc->timer_ended ());

	for (j = maxcnt = 0; j < depnum; j++) {
		cnt = dstlist[j].num_qualified;
		maxcnt = maxcnt < cnt ? cnt : maxcnt;

		// 每個系的檢定標準的通過人數
		// printf (WISHID_FMT " - %7d\n", dep[j].id, cnt);
	}

	if ((stdlist = misc->malloc (maxcnt *
				sizeof (struct student_on_wish_t))) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM, "Out of memory (1)");
		return 0;
	}

	if (! dp->allocate_space ()) return 1;

	print_memory ();

	//

	logfp = fopen ("/tmp/dispatcher.log", "w");

	initialize_all_students ();

	have_safe = have_bound = have_final = dep_final = 0;

	for (i = 0; i < depnum; i++) dstlist[i].affect = 1;

// #undef MAX_PER_STUDENT_WISH
// #define MAX_PER_STUDENT_WISH 3
	misc->timer_started ();

	for (k = 0; k <= MAX_PER_STUDENT_WISH; k++) {
		for (i = 0; examine_wish (k, i);i++)
			;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Finish trace on all wishes [ %.02f second(s) ]\r\n",
			misc->timer_ended ());

	// 志願用完了, 去掉那些在 Safe State 而不 bound 的學生

	misc->timer_started ();

	while (remove_safe_but_not_bound ()) {
		for (i = 0; examine_wish (MAX_PER_STUDENT_WISH, i);i++)
			;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Finish [ %.02f second(s) ]\r\n",
			misc->timer_ended ());

	if (logfp != NULL) fclose (logfp);

	// ------------------------------------------------------

	// 準備寫分發結果 ...

	write_result ();

	return 1;
}
#endif
