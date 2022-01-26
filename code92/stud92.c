/*
 *	stud92.c
 *
 *	Copyright (c) 2003, Jiann-Ching Liu
 */

#ifndef __STUD92_C__
#define __STUD92_C__

#if ENABLE_DISPATCHER92 == 1

#if ENABLE_EXTEND_DISPATCHER92 == 1
#  define NUM_OF_SKILL92	(NUMBER_OF_SKILL92 + 1)
#else
#  define NUM_OF_SKILL92	NUMBER_OF_SKILL92
#endif

static struct student_indexes_t         *stdsksortedindex[NUM_OF_SKILL92];
static struct student_skill_info_t	*skill_info = NULL;
static int				skill_info_cnt = 0;
static int				skill_info_idx = 0;
static int				fill_in_cnt = 0;
#if TRY_CEEC_MERGE_SN == 1
static struct student_indexes_t		*stdsnsortedindex = NULL;
#endif

static int read_ceec92_skill_score_record (char *buffer, const int len) {
#if ENABLE_EXTEND_DISPATCHER92 == 1
	struct ceec_extend_skill_score92	*ptr =
				  (struct ceec_extend_skill_score92 *) buffer;
#else
	struct ceec_skill_score92	*ptr =
					  (struct ceec_skill_score92 *) buffer;
#endif
	struct student_skill_info_t	*p;
	int				i, j;

	p = &skill_info[skill_info_idx];
	p->own_by = -1;

	for (i = 0; i < NUMBER_OF_SKILL92; i++) {
		p->skill_sid[i] = misc->dec (ptr->skill_sid[i],
						STUDENT_ID_LENGTH);

		stdsksortedindex[i][skill_info_idx].idx = skill_info_idx;
		stdsksortedindex[i][skill_info_idx].sid = p->skill_sid[i];
	}

#if TRY_CEEC_MERGE_SN == 1
	stdsnsortedindex[skill_info_idx].idx = skill_info_idx;
	stdsnsortedindex[skill_info_idx].sid = misc->dec (ptr->serial,
							sizeof ptr->serial);
	p->serial = stdsnsortedindex[skill_info_idx].sid;
#endif

	p->disallow = (ptr->disallow[0] == 'Y') ? 1 : 0;

	p->music_major = misc->dec (ptr->m_major, 4);
	p->music_minor = misc->dec (ptr->m_minor, 4);

	for (i = 0; i < SK92_SUBSCR_SET; i++) {
		for (j = 0; j < SK92_SUBSCR_NUM; j++) {
			if (ptr->sk_score[i][j][0] == '*') {
				p->sk_miss[i][j] = 1;
			} else {
				p->sk_miss[i][j] = 0;
			}

			/*
			p->sk_score[i][j] = misc->dec (
					ptr->sk_score[i][j], 5);
			*/
			p->sk_score[i][j] = misc->score_conv (
					ptr->sk_score[i][j]);
		}
	}

	for (i = 0; i < MAX_PER_STUDENT_WISH; i++) p->sksc_onwish[i] = 0;

	// p->physical_score = misc->dec (ptr->p_score, 5);
	// p->physical_grade = misc->dec (ptr->p_grade, 2);
	p->physical_score = misc->score_conv (ptr->p_score);

	if (ptr->p_grade[0] == '-') {
		p->physical_grade = -1;
	} else if (ptr->p_grade[0] == '*') {
		p->physical_grade = 0;
		p->physical_miss = 1;
	} else {
		p->physical_grade = misc->dec (ptr->p_grade, 2);
	}

	// p->physical_miss = (ptr->p_score[0] == '*') ? 1 : 0;


#if ENABLE_EXTEND_DISPATCHER92 == 1
	p->skill_group = 0;
	p->exam_sid    = 0;

	if (len >= sizeof (struct ceec_extend_skill_score92)) {
		p->skill_group = misc->dec (ptr->skill_group, 1);
		p->skill_score = misc->dec (ptr->skill_score, 5);
		p->exam_sid = misc->dec (ptr->sid_exam, STUDENT_ID_LENGTH);
	}

	stdsksortedindex[NUMBER_OF_SKILL92][skill_info_idx].idx =
								skill_info_idx;
	stdsksortedindex[NUMBER_OF_SKILL92][skill_info_idx].sid = p->exam_sid;
#endif

	skill_info_idx++;
	return 1;
}

static int fill_in (struct student_indexes_t  *ptr, const int xsid,
						const int i, const int j) {
	if (stdbuf[i].skinfo->own_by == xsid) {
		return 0;
	} else if (stdbuf[i].skinfo->own_by != -1) {
		fprintf (stderr, "\nOOPS! "
				"Duplicate SID %d for "
				"%d v.s %d\n",
				xsid,
				stdbuf[i].skinfo->own_by,
				stdbuf[i].sid
		);
		stdbuf[i].skinfo = NULL;
		return 0;
	}

	fill_in_cnt++;

	stdbuf[i].skinfo->own_by = stdbuf[i].sid;

	if (stdbuf[i].skinfo->disallow) {
		stdbuf[i].score_disallow |= STSCD_DISALLOW_SKILL;
	}

	if (j == 0) { // 音樂
		// 抄進舊表中 ...
		stdbuf[i].skillmajor = stdbuf[i].skinfo->music_major;
		stdbuf[i].skillminor = stdbuf[i].skinfo->music_minor;
#if ENABLE_EXTEND_DISPATCHER92 == 1
	} else if (j == NUMBER_OF_SKILL92) {
		stdbuf[i].skillmajor = stdbuf[i].skinfo->music_major;
		stdbuf[i].skillminor = stdbuf[i].skinfo->music_minor;
#endif
	}

	if (stdbuf[i].skill_group == 0) {
		stdbuf[i].skill_group = j + 1;
	} else {
		// multiple skill group
	}

	return 1;
}

static int load_CEEC_skill_score (char *file) {
	int				sid, i, j, len, nof, errcnt;
	char				*buffer;
	struct student_indexes_t	*ptr, key;
	int				mnum[NUM_OF_SKILL92];
#if TRY_CEEC_MERGE_SN == 1
	int				reget = 0;
#endif

	for (i = 0; i < NUM_OF_SKILL92; i++) mnum[i] = 0;

	misc->print (PRINT_LEVEL_SYSTEM,
			"Load CEEC92 skill score file [ %s ] ... ", file);

	i = 0;

	if (tio->open (file)) {
		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct ceec_skill_score92)) continue;
			i++;
		}
		tio->close ();
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return 0;
	}

	misc->print (PRINT_LEVEL_SYSTEM, "%d records\r\n", i);

	// Allocate memory for skill score ...

	if (i > 0) {
		if (skill_info != NULL) free (skill_info);

		if ((skill_info = misc->calloc (
				skill_info_cnt = i,
				sizeof (struct student_skill_info_t))) == NULL){
			fprintf (stderr, "Out of memory\r\n");
			return 0;
		}

		skill_info_idx = nof = 0;

		for (i = 0; i < NUM_OF_SKILL92; i++) {
			if ((stdsksortedindex[i] = misc->malloc (
					skill_info_cnt *
					sizeof (struct student_indexes_t)))
					== NULL) {
				fprintf (stderr, "Out of memory\r\n");
				return 0;
			}
		}

#if TRY_CEEC_MERGE_SN == 1
		if ((stdsnsortedindex = misc->malloc (
				skill_info_cnt *
				sizeof (struct student_indexes_t))) == NULL) {
			fprintf (stderr, "Out of memory\r\n");
			return 0;
		}
#endif

		misc->timer_started ();

		misc->print (PRINT_LEVEL_SYSTEM,
				"Reading and sorting for skill score ... ");

		tio->open (file);

		while ((len = tio->read (&buffer)) >= 0) {
			if (len < sizeof (struct ceec_skill_score92)) continue;

			if (read_ceec92_skill_score_record (
						buffer, len) != -1) {
				nof++;
			}
		}

		tio->close ();

		if (skill_info_idx != skill_info_cnt) {
			misc->print (PRINT_LEVEL_SYSTEM, "Oops ! %d v.s. %d\n",
					skill_info_idx,
					skill_info_cnt);
		}

		//misc->print (PRINT_LEVEL_SYSTEM,
		//		"%d read, %d use\r\n", i, nof);

		for (i = 0; i < NUM_OF_SKILL92; i++) {
			qsort (stdsksortedindex[i], skill_info_cnt,
				sizeof (struct student_indexes_t),
				(int (*)(const void *, const void *))
				order_by_sid);
		}
#if TRY_CEEC_MERGE_SN == 1
		qsort (stdsnsortedindex, skill_info_cnt,
				sizeof (struct student_indexes_t),
				(int (*)(const void *, const void *))
				order_by_sid);
#endif

		if (check_for_duplicate) {
			for (i = 0; i < NUM_OF_SKILL92; i++) {
				for (j = 1; j < skill_info_cnt; j++) {
					if (stdsksortedindex[i][j].sid == 0) {
						continue;
					}
					if (stdsksortedindex[i][j].sid ==
						stdsksortedindex[i][j -1].sid) {
						fprintf (stderr,
							"Duplicate sid %d "
							"on skill sid [%d]\n",
							stdsksortedindex[i][j].
							sid,
							i);
					}
				}
			}
		}

		for (i = errcnt = 0; i < stdnum; i++) {
			for (j = 0; j < NUM_OF_SKILL92; j++) {
#if ENABLE_EXTEND_DISPATCHER92 == 1
				if (j < NUMBER_OF_SKILL92) {
					sid = stdbuf[i].skill_sid[j];
				} else {
					sid = stdbuf[i].sid;
				}
#else
				sid = stdbuf[i].skill_sid[j];
#endif
				if (sid != 0) {
					key.sid = sid;

					ptr = bsearch (&key,
						stdsksortedindex[j],
						skill_info_cnt,
						sizeof (struct
							student_indexes_t),
						(int (*)(const void *,
							 const void *))
						order_by_sid);

#if ENABLE_EXTEND_DISPATCHER92 == 1
					if (ptr == NULL) {
						if (j < NUMBER_OF_SKILL92) {
							fprintf (stderr,
								"No such sid "
								"%d\n",
								sid);
							errcnt++;
						}

						continue;
					}
#else
					if (ptr == NULL) {
						fprintf (stderr, "No such sid "
								"%d\n",
								sid);
						errcnt++;
						continue;
					}
#endif

					stdbuf[i].skinfo =
							&skill_info[ptr->idx];

					fill_in (ptr, stdbuf[i].sid, i, j);

					++mnum[j];
#if 0
					if (stdbuf[i].skinfo->own_by != -1) {
						fprintf (stderr, "\nOOPS! "
							"Duplicate SID %d for "
							"%d v.s %d\n",
							sid,
							stdbuf[i].skinfo->
									own_by,
							stdbuf[i].sid
							);
						stdbuf[i].skinfo = NULL;
						break;
					} else {
						stdbuf[i].skinfo->own_by =
							stdbuf[i].sid;
					}

					if (j == 0) { // 音樂
						// 抄進舊表中 ...
						stdbuf[i].skillmajor
							= stdbuf[i].skinfo->
								music_major;
						stdbuf[i].skillminor
							= stdbuf[i].skinfo->
								music_minor;
#if ENABLE_EXTEND_DISPATCHER92 == 1
					} else if (j == NUMBER_OF_SKILL92) {
						stdbuf[i].skillmajor
							= stdbuf[i].skinfo->
								music_major;
						stdbuf[i].skillminor
							= stdbuf[i].skinfo->
								music_minor;
#endif
					}

					if (stdbuf[i].skill_group == 0) {
						stdbuf[i].skill_group = j + 1;
					} else {
						// multiple skill group
					}
#endif
					break;
				}
			}
		}

#if TRY_CEEC_MERGE_SN == 1
		for (i = 0; i < stdnum; i++) {
			if (stdbuf[i].skinfo == NULL) {	// try sn merge
				key.sid = stdbuf[i].serial;

				// fprintf (stderr, "Try sn merge for %d\n",
				// 		key.sid);

				ptr = bsearch (&key,
						stdsnsortedindex,
						skill_info_cnt,
						sizeof (struct
							student_indexes_t),
						(int (*)(const void *,
							 const void *))
						order_by_sid);

				if (ptr != NULL) {
					stdbuf[i].skinfo =
							&skill_info[ptr->idx];

					if (fill_in (ptr,
							stdbuf[i].sid, i, 0)) {
						reget++;
						//printf ("SN=%05d, ", key.sid);
						for (j = 0;
							j < NUMBER_OF_SKILL92;
								j++) {
							stdbuf[i].skill_sid[j] =
							   stdbuf[i].skinfo->
								skill_sid[j];
							//printf ("[%d: %d]",
							//j,
							//stdbuf[i].skinfo->
							//	skill_sid[j]);
						}
						//printf ("\n");
					}
				}
			}
		}

		if (reget > 0) {
			misc->print (PRINT_LEVEL_SYSTEM, "[%d/%d] ",
					reget, fill_in_cnt);
		} else {
			misc->print (PRINT_LEVEL_SYSTEM, " [");

			for (i = 0; i < NUM_OF_SKILL92; i++) {
				misc->print (PRINT_LEVEL_SYSTEM, "%d,",
						mnum[i]);
			}

			misc->print (PRINT_LEVEL_SYSTEM, "(%d) use] ",
					fill_in_cnt);
		}
#else
		misc->print (PRINT_LEVEL_SYSTEM, "[%d use] ", fill_in_cnt);
#endif

		misc->print (PRINT_LEVEL_SYSTEM,
				"%s.  (%.4f seconds)\r\n",
				errcnt ? "error" : "ok",
				misc->timer_ended()); 

		for (i = 0; i < NUM_OF_SKILL92; i++) free (stdsksortedindex[i]);
#if TRY_CEEC_MERGE_SN == 1
		free (stdsnsortedindex);
#endif

#if 0
		for (i = j = 0; i < skill_info_cnt; i++) {
			if (skill_info[i].own_by == -1) {
				j++;
				fprintf (stderr, "[%d]",
							skill_info[i].serial);
			}
		}

		if (j > 0) {
			misc->print (PRINT_LEVEL_SYSTEM,
					"%d t_sco do not have owner !!\n", j);
		}
#endif

		if (errcnt) return 0;
	} else {
		return 0;
	}

	return 1;
}

static int read_ceec_student_info_file_record92 (char *buffer) {
	struct ceec_stdinfo92	*ptr = (struct ceec_stdinfo92 *) buffer;
	int			sid, idx;
	char			*ceec_idpos[4];
	int			i;
	int			skstf = 0;

	if (sizeof ptr->skillmajor == 4) {
		skstf = 0;
	} else if (sizeof ptr->skillmajor == 5) {
		skstf = 1;
	}

	ceec_idpos[0] = ptr->sid_exam;
	ceec_idpos[1] = ptr->sid_basic;
	ceec_idpos[2] = ptr->sid_mend;
	ceec_idpos[3] = NULL;

	for (i = 0, idx = -1; ceec_idpos[i] != NULL; i++) {
		sid = misc->dec (ceec_idpos[i], STUDENT_ID_LENGTH);
		if ((idx = search_sid_index (sid)) != -1) {
			break;
		}
	}

	if (idx >= 0) {
		// stdbuf[idx].sflag = 0;
		if ((stdbuf[idx].sflag & STUDENT_FLAG_HAVE_INFO) != 0) {
			fprintf (stderr, "Oops!! duplicat student wish\n");
		}

		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_INFO;


#if ENABLE_CEEC_MERGE_SN == 1
		stdbuf[idx].sn     = misc->dec (
					ptr->serial, sizeof ptr->serial);
#else
#  if TRY_CEEC_MERGE_SN == 1
		stdbuf[idx].serial = misc->dec (
					ptr->serial, sizeof ptr->serial);
#  endif
		stdbuf[idx].usedid[0] =
				misc->dec (ptr->sid_exam,  STUDENT_ID_LENGTH);
		stdbuf[idx].usedid[1] =
				misc->dec (ptr->sid_basic, STUDENT_ID_LENGTH);
		stdbuf[idx].usedid[2] =
				misc->dec (ptr->sid_mend,  STUDENT_ID_LENGTH);
#endif

#if DEBUG_LEVEL > 1
		if (stdbuf[idx].extsid != 0) {
			if (stdbuf[idx].usedid[1] != stdbuf[idx].extsid) {
				fprintf (stderr, "OOPS! %d v.s. %d\n",
						stdbuf[idx].usedid[1],
						stdbuf[idx].extsid);
			}
		}
#endif

		if (ptr->sex != '1') {
			// 不是男生的都算是女生
			stdbuf[idx].sflag |= STUDENT_FLAG_GIRL;
		}

		// 注意!! 這個欄位其實不是大考原應有的 ...
		get_student_special_flag (idx, ptr->ptype);

		stdbuf[idx].ptype = ptr->ptype;
		stdbuf[idx].highsch = misc->dec (
				ptr->edu_level, sizeof ptr->edu_level);

		for (i = 0; i < NUMBER_OF_SKILL92; i++) {
			stdbuf[idx].skill_sid[i] = misc->dec (
					ptr->sid_skill[i], STUDENT_ID_LENGTH);
		}
		/*
		if ((i = misc->dec (ptr->skillgroup, 1)) != 0) {
			stdbuf[idx].skillgroup = i;
			stdbuf[idx].skillmajor = misc->dec (
						&ptr->skillmajor[skstf], 4);
			stdbuf[idx].skillminor = misc->dec (
						&ptr->skillminor[skstf], 4);
		} else {
			stdbuf[idx].skillgroup = 0;
			stdbuf[idx].skillmajor = 0;
			stdbuf[idx].skillminor = 0;
		}
		*/
		
		stdbuf[idx].skill_group = 0;
		stdbuf[idx].skillmajor  = 0;
		stdbuf[idx].skillminor  = 0;

		if (ptr->disallow == 'N') {
			stdbuf[idx].sflag |= STUDENT_FLAG_DISALLOW;
		}

		switch (misc->dec (&ptr->special, 1)) {
		case 2: // 國防
			// dprint ("Student: %d DF\n", stdbuf[idx].sid);
			stdbuf[idx].sflag |= STUDENT_FLAG_DF_ALLOW;
			break;
		case 3: // 師範
			// dprint ("Student: %d nou only\n", stdbuf[idx].sid);
			stdbuf[idx].sflag |= STUDENT_FLAG_NU_ONLY;
			break;
		case 1:
		default:
			break;
		}

		if (include_student_extend_info) {
			struct student_extend_info_t		*info1;
			struct student_extend_info_level2_t	*info2;

			if (have_level_2_info) {
				info2 = &stdinfo2[idx];
				info1 = (void *) info2;
			} else {
				info1 = &stdinfo[idx];
				info2 = (void *) info1;
			} 

			stdbuf[idx].info = info1;

			strncpy (info1->name, ptr->name, MAX_STUDENT_LEN * 2);

			info1->birthday = misc->birthday (ptr->birth);

			if (have_level_2_info) {

#define MCPSTWLN(x,y)	memcpy (x, y, sizeof x)

				MCPSTWLN (info2->regist_sn, ptr->regist_sn);
				MCPSTWLN (info2->person_id, ptr->person_id);
				MCPSTWLN (info2->address  , ptr->address);
				MCPSTWLN (info2->cmaddr   , ptr->cmaddr);
				MCPSTWLN (info2->edu_level, ptr->edu_level);
				MCPSTWLN (info2->tel_no   , ptr->tel_no);
				MCPSTWLN (info2->zip_code , ptr->zip_code);
				MCPSTWLN (info2->edu_year , ptr->edu_year);
				MCPSTWLN (info2->guard_man, ptr->guard_man);
			}


#if DEBUG_LEVEL > 0
			if (stdbuf[idx].sid == trace_sid) {
				misc->print (PRINT_LEVEL_DEBUG,
					"Student: %d, Name = %s, Gender=%s\r\n",
					stdbuf[idx].sid,
					stdbuf[idx].info->name,
					(stdbuf[idx].sflag & STUDENT_FLAG_GIRL)
					!= 0 ? "女" : "男");
			}
#endif
		}
	}

	return idx;
}
#endif

static int read_ceec_exam_score_file_record92 (char *buffer) {
	struct ceec_exam_score92	*ptr =
				(struct ceec_exam_score92 *) buffer;
	int			i, idx, sid;

#if ENABLE_CEEC_MERGE_SN == 1
	sid = misc->dec (ptr->serial, STUDENT_CEEC_SN_LENGTH);
#else
	sid = misc->dec (ptr->sid_exam, STUDENT_ID_LENGTH);
#endif

	if ((idx = search_stdsn_index (sid)) != -1) {
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_SCORE;
		stdbuf[idx].sflag |= STUDENT_FLAG_HAVE_CEEC_EXAM;

		for (i = 0; i < 9; i++) {
			if (ptr->score[i][0] == '*') {
				stdbuf[idx].testmiss[course_order_patch[i] - 1]
						= 1;
			}

			stdbuf[idx].testscore[course_order_patch[i] - 1] =
					misc->score_conv (ptr->score[i]);
		}

		if (ptr->disallow[0] == 'Y') {
			stdbuf[idx].score_disallow |= STSCD_DISALLOW_EXAM;
		}

		/*
		if ((i = stdbuf[idx].skillgroup) != 0) {

			if (ptr->skill[i - 1][0] == '*') {
				stdbuf[idx].sflag |= STUDENT_FLAG_SKILL_MISS;
			} else if ((stdbuf[idx].sflag &
					STUDENT_FLAG_SKILL_MISS) != 0){
				stdbuf[idx].sflag &= ~STUDENT_FLAG_SKILL_MISS;
			}

			stdbuf[idx].skillscore = misc->score_conv (
							ptr->skill[i - 1]);

			stdbuf[idx].skillmajor_score =
						misc->score_conv (ptr->major);
		} else {
			stdbuf[idx].skillscore       = 0;
			stdbuf[idx].skillmajor_score = 0;
		}
		*/
	}

	return idx;
}

#endif
