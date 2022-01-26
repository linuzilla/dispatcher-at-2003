/*
 *	du92.c
 *
 *	Copyright (c) 2003, Jiann-Ching Liu
 */

#ifndef __DISPUNIT92_C__
#define __DISPUNIT92_C__

#if ENABLE_DISPATCHER92 == 1

static int check_skill92 (struct dispatcher_unit_t *du) {
	int				i, grp;
	struct student_skill_info_t	*skinf;
	int8_t				*sk_score;
	int8_t				*sk_ratio;
	int64_t				skmj, skmi;
	struct skillset_t		*skptr;
	short				skok;
	int32_t				skill_real_score;
	int32_t				skill_check_score;
	int32_t				sksc;

#if DEBUG_LEVEL > 0
	static struct student_skill_info_t	*skfirst = NULL;
	short				debug_flag = 0;


	if (du->st->sid == trace_sid) debug_flag = 1;
#endif

	if ((du->flags & DUTF_SKILL_IS_CHECK) == 0) {
		du->flags |= DUTF_SKILL_IS_CHECK;

		if (du->dp->skillgroup == 0) {
			// 不考術科 ...

			du->flags |= DUTF_SKILLCHECK_IS_PASS;
			return 1;
		}

		grp = du->dp->skillgroup - 1;

		// grp 必需小於 NUMBER_OF_SKILL92
#if DEBUG_LEVEL > 0
		if (debug_flag) {
			dprint ("SKILL92   check for %d on "
					WISHID_FMT " (SKSID:%d)%d\n",
					du->st->sid, du->dp->id,
					du->st->skill_sid[grp],
					grp);
		}
#endif              

		if ((du->st->score_disallow & STSCD_DISALLOW_SKILL) != 0) {
#if ENABLE_POSTCHECK == 1
			pstck->disallow_on_skill (
				du->st,
				du->st->wish_index,
				du->dp->id);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
			du->st->not_qualify_flag |= STNQF_SCORE_DISALLOW;
#endif
			return 0;
		}

#if ENABLE_EXTEND_DISPATCHER92 == 1
		if (grp < NUMBER_OF_SKILL92) {
			if (du->st->skill_sid[grp] == 0) { // 考生沒報考 ...
#if DEBUG_LEVEL > 1
				if (debug_flag) {
					dprint ("SKILL92 fail (no sid)\n");
				}
#endif
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
		} else {
#if DEBUG_LEVEL > 9
			if (du->st->sid == trace_sid) {
				dprint ("Pass one!!\n");
			}
#endif              
			if ((skinf = du->st->skinfo) == NULL) {
#if DEBUG_LEVEL > 1
				if (debug_flag) {
					dprint ("SKILL92 fail (NULL ptr)\n");
				}
#endif
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
#if DEBUG_LEVEL > 9
			if (du->st->sid == trace_sid) {
				dprint ("Pass two!! %d, %d\n",
						skinf->skill_group,
						du->dp->skillgroup);
			}
#endif              
			if (skinf->skill_group != du->dp->skillgroup) {
#if DEBUG_LEVEL > 1
				if (debug_flag) {
					dprint ("SKILL92 fail (group "
							"%d != %d)\n",
							skinf->skill_group,
							du->dp->skillgroup);
				}
#endif
#if ENABLE_POSTCHECK == 1
				pstck->need_skill_group (
					du->st,
					du->st->wish_index,
					du->dp->id,
					du->dp->skillgroup,
					skinf->skill_group);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag |= STNQF_NO_SKILL_ITEM;
#endif
				return 0;
			}
#if DEBUG_LEVEL > 9
			if (du->st->sid == trace_sid) {
				dprint ("Pass three!!\n");
			}
#endif              
		}
#else
		if (du->st->skill_sid[grp] == 0) { // 考生沒報考 ...
#if DEBUG_LEVEL > 1
			if (debug_flag) {
				dprint ("SKILL92 fail (no sid)");
			}
#endif
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
#endif

		if ((skinf = du->st->skinfo) == NULL) {
			// Oops !!
#if DEBUG_LEVEL > 1
			if (debug_flag) {
				dprint ("SKILL92 fail (NULL ptr)\n");
			}
#endif              
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

#if DEBUG_LEVEL > 9
		if (debug_flag) {
			dprint ("Pass one!!\n");
		}
#endif              

		if (grp < SK92_SUBSCR_SET) { // 音樂或美術
			sk_score = du->dp->skill_lim[grp][SK92_SCORE_INDEX];
			sk_ratio = du->dp->skill_lim[grp][SK92_RATIO_INDEX];

			for (i = 0; i < SK92_SUBSCR_NUM; i++) { // 比五科的成績
				if (sk_ratio[i] != 0) {
					if (skinf->sk_score[grp][i] < 0) {
						// 未報考
#if DEBUG_LEVEL > 1
						if (debug_flag) {
							dprint ("Skill fail on "
								"No check!!\n");
						}
#endif              
#if ENABLE_POSTCHECK == 1
						pstck->need_skill_sub (
							du->st,
							du->st->wish_index,
							du->dp->id,
							du->dp->skillgroup, i);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
						du->st->not_qualify_flag |=
							STNQF_NO_SKILL_ITEM;
#endif
						return 0;
					}
#if CARE_ABOUT_SKILL_MISS_OR_ZERO == 1
					if (skinf->sk_miss[grp][i]) {
#  if DEBUG_LEVEL > 1
						if (debug_flag) {
							dprint ("SKILL92 fail "
							    "(miss %d,%d)\n",
							    grp, i);
						}
#  endif              
#  if ENABLE_POSTCHECK == 1
						pstck->skill_sub_miss (
							du->st,
							du->st->wish_index,
							du->dp->id,
							du->dp->skillgroup, i);
#  endif
#  if ENABLE_FULL_FAIL_RESULT_CHECK == 1
						du->st->not_qualify_flag |=
							STNQF_NO_SKILL_ITEM;
#  endif
						return 0;
					}
#endif
				}

				if (sk_score[i] != 0) { // 有分數限制 ...
#if DEBUG_LEVEL > 0
					if (debug_flag) {
						dprint ("%d v.s. %d!!\n",
							skinf->
							sk_score[grp][i]
							, sk_score[i]);
					}
#endif              
					if (skinf->sk_score[grp][i] <
							sk_score[i] * 100) {
#if DEBUG_LEVEL > 1
						if (debug_flag) {
							dprint ("SKILL92 fail ("
							     "score %d < %d)\n",
								skinf->
								sk_score[grp][i]
								, sk_score[i]);
						}
#endif              
#  if ENABLE_POSTCHECK == 1
						pstck->skill_sub_score (
							du->st,
							du->st->wish_index,
							du->dp->id,
							du->dp->skillgroup,
							i,
							skinf->sk_score[grp][i],
							sk_score[i]);
#  endif
#  if ENABLE_FULL_FAIL_RESULT_CHECK == 1
						du->st->not_qualify_flag |=
							STNQF_SKILL_ITEM_NQ;
#  endif
						return 0;
					}
				} else if (du->dp->skill_zero) {
					// 任一小項零分
					// 所謂的任一小項是指 ....
#if DO_NOT_CARE_ABOUT_SCORE_ON_NO_USED == 1
					if (sk_ratio[i] == 0) continue;
#endif

					if (skinf->sk_score[grp][i] <= 0) {
#if DEBUG_LEVEL > 1
						if (debug_flag) {
							dprint ("SKILL92 fail ("
								"score %d = 0)"
								"\n",
								skinf->
								sk_score[grp][i]
								);
						}
#endif              
#  if ENABLE_POSTCHECK == 1
						pstck->skill_sub_zero (
							du->st,
							du->st->wish_index,
							du->dp->id,
							du->dp->skillgroup, i);
#  endif
#  if ENABLE_FULL_FAIL_RESULT_CHECK == 1
						du->st->not_qualify_flag |=
							STNQF_SKILL_ITEM_ZERO;
#  endif
						return 0;
					}
				}
			}

			if ((skptr = du->dp->skillset) != NULL) {
				// 主副修限制
#if DEBUG_LEVEL > 0
				if (debug_flag) {
					dprint ("Check SKILLSET (%04d,%04d)\n",
							du->st->skillmajor,
							du->st->skillminor);
				}
#endif

				skmj = dmptr->skill_bitmap (
						du->dp->skillgroup,
						du->st->skillmajor);
				skmi = dmptr->skill_bitmap (
						du->dp->skillgroup,
						du->st->skillminor);

				for (skok = 0; skptr != NULL;
							skptr = skptr->next) {
					// 請注意! 這裡是假設 skptr->major == 0
					// 或 skptr->minor == 0 的狀況時,
					// 表任何科目皆行
					// (因為若當皆不行是不合理的)
#if DEBUG_LEVEL > 0
					if (debug_flag) {
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

				if (! skok) {
#if DEBUG_LEVEL > 9
					if (du->st->sid == trace_sid) {
						dprint ("skmj\n");
					}
#endif              
#if ENABLE_POSTCHECK == 1
					pstck->fail_on_instrument (
						du->st,
						du->st->wish_index,
						du->dp->id,
						du->st->skillmajor,
						du->st->skillminor);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
					du->st->not_qualify_flag |=
							STNQF_INSTRUMENT;
#endif
					return 0;
				}

				if (disallow_same_instrument) {
					if (skmj == skmi) {
						skptr = du->dp->skillset;

						if ((skptr->major != 0) &&
							  (skptr->minor != 0)) {
							// 主副修相同
#if ENABLE_POSTCHECK == 1
							pstck->
							   fail_on_instrument (
								du->st,
								du->st->
								    wish_index,
								du->dp->id,
								du->st->
								    skillmajor,
								du->st->
								    skillminor);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
							du->st->not_qualify_flag
								|=
							       STNQF_INSTRUMENT;
#endif
							return 0;
						}
					}
				}
			}

			skill_real_score = skill_check_score = 0;

			for (i = 0; i < SK92_SUBSCR_NUM; i++) { // 比五科的成績
#if DEBUG_LEVEL > 0
				if (debug_flag) {
					if (sk_ratio[i] > 0) {
						dprint ("[%d x %d]",
						    sk_ratio[i],
						    skinf->sk_score[grp][i]);
					}
				}
#endif
				if (sk_ratio[i] > 0) {
					skill_check_score +=
						sk_ratio[i] *
						skinf->sk_score[grp][i];
				}
			}
#if DEBUG_LEVEL > 0
			if (debug_flag) {
				dprint ("= %d\n", skill_check_score);
			}
#endif

			// 算出術科成績了 .... 這個數值是 10000 倍
			sksc = skill_check_score;

			if (du->dp->checkskill != 0) {
				// 術科檢定 ...
				if (skill_check_score < standval->checkskill[
						du->dp->skillgroup] * 100) {
#if DEBUG_LEVEL > 0
					if (du->st->sid == trace_sid) {
						dprint ("sk score (%d) "
								"%d < %d\n",
							du->dp->skillgroup,
							skill_check_score,
							standval->checkskill[
							du->dp->skillgroup]);
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
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
					du->st->not_qualify_flag
							|= STNQF_SKILL_ITEM_NQ;
#endif
					return 0;
				}
			}
#if ENABLE_EXTEND_DISPATCHER92 == 1
		} else if (grp == SK92_SUBSCR_SET) { // 體育

			if ((skill_real_score = skinf->physical_score) < 0) {
#if DEBUG_LEVEL > 0
				if (debug_flag) {
					dprint ("SKILL92 fail (phy)\n");
				}
#endif
#if ENABLE_POSTCHECK == 1
				pstck->need_skill_group (
					du->st,
					du->st->wish_index,
					du->dp->id,
					du->dp->skillgroup, 0);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag
						|= STNQF_NO_SKILL_ITEM;
#endif
				return 0;
			}

			if (grade_on_skill_physical) {
				skill_check_score = skinf->physical_grade;

				if (standval->checkskill[
						du->dp->skillgroup] >= 100) {
					skill_check_score *= 100;
				}
			} else {
				skill_check_score = skill_real_score;
			}

#if CARE_ABOUT_SKILL_MISS_OR_ZERO == 1
			if (skinf->physical_miss) {
#  if DEBUG_LEVEL > 0
				if (debug_flag) {
					dprint ("SKILL92 fail (phy miss)\n");
				}
#  endif
#  if ENABLE_POSTCHECK == 1
				pstck->fail_on_skill_miss (
					du->st,
					du->st->wish_index,
					du->dp->id,
					du->dp->skillgroup, -1);
#  endif
#  if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag
						|= STNQF_NO_SKILL_ITEM;
#  endif
				return 0;
			}
#endif

			if (du->dp->checkskill != 0) {
				if (skill_check_score < standval->
					checkskill[du->dp->skillgroup]) {
#if DEBUG_LEVEL > 0
					if (debug_flag) {
						dprint ("SKILL92 fail (phy %d "
								"< %d)\n",
							skill_check_score,
							standval->checkskill[
							du->dp->skillgroup]);
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
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
					du->st->not_qualify_flag |=
							STNQF_SKILL_ITEM_NQ;
#endif
					return 0;
				}
			}

			sksc = skill_real_score * 100;
		} else {	// 其它術科 ...
			skill_check_score = skinf->skill_score;

			if (du->dp->checkskill != 0) {
				if (skill_check_score < standval->
					checkskill[du->dp->skillgroup]) {
#if ENABLE_POSTCHECK == 1
					pstck->fail_on_skill_standard (
						du->st,
						du->st->wish_index,
						du->dp->id,
						skill_check_score,
						standval->checkskill[
							 du->dp->skillgroup]);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
					du->st->not_qualify_flag |=
							STNQF_SKILL_ITEM_NQ;
#endif
					return 0;
				}
			}

			if ((skptr = du->dp->skillset) != NULL) {
				// 主副修限制

				skmj = dmptr->skill_bitmap (
						du->dp->skillgroup,
						du->st->skillmajor);
				skmi = dmptr->skill_bitmap (
						du->dp->skillgroup,
						du->st->skillminor);

				for (skok = 0; skptr != NULL;
							skptr = skptr->next) {
					// 請注意! 這裡是假設 skptr->major == 0
					// 或 skptr->minor == 0 的狀況時,
					// 表任何科目皆行
					// (因為若當皆不行是不合理的)

					if (((skptr->major == 0) ||
						((skmj & skptr->major) != 0)) &&
						((skptr->minor == 0) ||
						((skmi & skptr->minor) != 0))) {
						skok = 1;
						break;
					}
				}

				if (! skok) {
#if DEBUG_LEVEL > 0
					if (du->st->sid == trace_sid) {
						dprint ("skmj\n");
					}
#endif              
#if ENABLE_POSTCHECK == 1
					pstck->fail_on_instrument (
						du->st,
						du->st->wish_index,
						du->dp->id,
						du->st->skillmajor,
						du->st->skillminor);
#endif
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
					du->st->not_qualify_flag |=
							STNQF_INSTRUMENT;
#endif
					return 0;
				}
			}

			sksc = skill_check_score * 100;
		}
#else
		} else if (du->dp->checkskill != 0) { // 術科 (體育) 檢定

			// 程式有點瀾，但只可能是體育 ....

			if ((skill_real_score = skinf->physical_score) < 0) {
#if DEBUG_LEVEL > 0
				if (debug_flag) {
					dprint ("SKILL92 fail (phy)\n");
				}
#endif
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

			if (grade_on_skill_physical) {
				skill_check_score = skinf->physical_grade;

				if (standval->checkskill[
						du->dp->skillgroup] >= 100) {
					skill_check_score *= 100;
				}
			} else {
				skill_check_score = skill_real_score;
			}

			if (skill_check_score <
				standval->checkskill[du->dp->skillgroup]) {
#if DEBUG_LEVEL > 0
				if (debug_flag) {
					dprint ("SKILL92 fail (phy %d < %d)\n",
						skill_check_score,
						standval->checkskill[
							du->dp->skillgroup]);
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
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
				du->st->not_qualify_flag |= STNQF_SKILL_ITEM_NQ;
#endif
				return 0;
			}

			// 體育的術科成績 .... 也變成 10000 倍
			sksc = skill_real_score * 100;
		} else {
			// 不知道是否有體育但不檢定的 ... 也變成 10000 倍吧 !!
			sksc = skinf->physical_score * 100;
		}
#endif

		// 把術科成績丟進表中 ....
		// skinf->sksc_onwish[du->st->wish_index - 1] =
		//			du->st->skillscore = skinf->sksc;

		du->st->skillscore =
			skinf->sksc_onwish[du->st->wish_index - 1] = sksc;

#if DEBUG_LEVEL > 0
		if (skfirst != NULL) {
			if (skinf == skfirst) {
				dprint ("** [FIRST] %d skill score for "
					WISHID_FMT
					" %d store on %d [%p]\n",
					du->st->sid,
					du->dp->id,
					sksc,
					du->st->wish_index - 1,
					skinf);
			}
		}
		
		if (du->st->sid == trace_sid) {
			dprint ("%d skill score for " WISHID_FMT
					" %d store on %d [%p]\n",
					trace_sid,
					du->dp->id,
					sksc,
					du->st->wish_index - 1,
					skinf);
			skfirst = skinf;
		}
#endif              
		/*

		// - - - - - - - - - - - - - - - - - - - - - - - - - -

		// 去算術科分數 ? 看學生是否缺考 ?

		// - - - - - - - - - - - - - - - - - - - - - - - - - -

		if (du->dp->checkskill == 0) {
			// 沒有術科檢定
			du->flags |= DUTF_SKILLCHECK_IS_PASS;
			return 1;
		}

		// 應該只有體育才有

 		*/

		du->flags |= DUTF_SKILLCHECK_IS_PASS;
		return 1;
	}

	return (du->flags & DUTF_SKILLCHECK_IS_PASS);
}

#endif
#endif
