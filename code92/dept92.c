/*
 *	dept92.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __DEPT92_C__
#define __DEPT92_C__

#ifndef DULL_ELEPHANT_OPTION 
#define DULL_ELEPHANT_OPTION	1
#endif

static int load_skillset92 (void) {
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
			"Load skill-set 92 file [ %s ] ... ", filename);

	if (tio->open (filename)) {
		have_error = count = 0;

#if DULL_ELEPHANT_OPTION == 1
		if (de_is_skip_first_line) tio->read ((char **) &ptr);
#endif
		while ((len = tio->read ((char **) &ptr)) >= 0) {
			if (len < sizeof (struct ncu_skillset)) continue;
			count++;
			err = 0;

			if (de_wish_use_oct) {
				depid = misc->oct (ptr->wishid,
							DEPARTMENT_ID_LENGTH);
			} else {
				depid = misc->dec (ptr->wishid,
							DEPARTMENT_ID_LENGTH);
			}

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
#if DEBUG_LEVEL > 0
				if (depbuf[i].id == trace_wishid) {
					dprint ("Major %04d - %llu\n",
							crs, bitmap);
				}
#endif


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

#if DEBUG_LEVEL > 0
				if (depbuf[i].id == trace_wishid) {
					dprint ("Minor %04d - %llu\n",
							crs, bitmap);
				}
#endif

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

#if DEBUG_LEVEL > 0
			if (depbuf[i].id == trace_wishid) {
				dprint ("Dep: " WISHID_FMT
					" Skillset: %llu - %llu\n",
						trace_wishid,
						major, minor);
			}
#endif
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


static int load_limits92 (void) {
	char			*filename;
	int			i, j, k, idx, len;
#if SKILL_WEIGHT_ON_MAIN_WEIGHT == 1
	double			ccnt;
	double			multiply[] = { 0.0, 1.0, 1.25, 1.5, 1.75, 2.0 };
#else
	int			ccnt;
#endif
	int			crsnum_cnt;
	int			count = 0;
	struct ncu_deplim92	*ptr;

	if ((filename = sysconfig->getstr ("department-limit-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
			"%% system variable (department-limit-file)"
			" not define!!\r\n");
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load department limit92 file [ %s ] ... ", filename);

	if (tio->open (filename)) {
#if DULL_ELEPHANT_OPTION == 1
		if (de_is_skip_first_line) tio->read ((char **) &ptr);
#endif
		count = 0;
		while ((len = tio->read ((char **) &ptr)) >= 0) {
			if (len < sizeof (struct ncu_deplim92)) continue;
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
			"Load department limit92 into memory  ... ");

	if (tio->open (filename)) {
#if DULL_ELEPHANT_OPTION == 1
		if (de_is_skip_first_line) tio->read ((char **) &ptr);
#endif
		for (idx = 0; (len = tio->read ((char **) &ptr)) >= 0; ){
			if (len < sizeof (struct ncu_deplim92)) continue;

			if (de_wish_use_oct) {
				depbuf[idx].id = misc->oct (ptr->wishid,
						DEPARTMENT_ID_LENGTH);
			} else {
				depbuf[idx].id = misc->dec (ptr->wishid,
						DEPARTMENT_ID_LENGTH);
			}

			depbuf[idx].num = misc->dec (ptr->num,
						DEPARTMENT_NUMBER_LENGTH);

#if ENABLE_DEPARTMENT_FIRST == 1
			depbuf[idx].real_num = depbuf[idx].num;
#endif

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

			depbuf[idx].max_orgsum   = 0;
			depbuf[idx].min_orgsum   = 0;
			depbuf[idx].min_orgsum_n = 0;
#if DEBUG_LEVEL > 9
			dprint ("on same: [");
#endif
			for (i = 0; i < MAXIMUM_ON_SAME_REFERENCE; i++) {
#if DEBUG_LEVEL > 9
				dprint ("%c", buffer[j]);
#endif
				depbuf[idx].onsame[i] = (ptr->onsame[i] - 'A');
			}

			// 術科 音樂 比例、分數, 美術的 比例、分數

			for (i = 0; i < 2; i++) {
				for (j = 0; j < 2; j++) {
					for (k = 0; k < 5; k++) {
						depbuf[idx].skill_lim[i][j][k]
						   = misc->dec (
						   ptr->sk_ratio_score[i][j][k],
						   3);
					}
				}
			}

			depbuf[idx].skill_zero = misc->dec (
					ptr->skill_zero, 1);
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

			if (depbuf[idx].skillgroup > 0) {
				// [*]
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

				fprintf (stderr, "[logwish=%d]\n", logwish);

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

#endif
