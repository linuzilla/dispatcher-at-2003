/*
 *	postcheck.h
 *
 *	Copyright (c) 2003, Jiann-Ching Liu
 */

#ifndef __POST_CHECK_H__
#define __POST_CHECK_H__

#include <sys/types.h>
#include "predefine.h"

struct student_t;
struct department_t;
struct student_module_t;
struct department_module_t;

struct postcheck_t {
	int	(*open)(const char *filename);
	void	(*close)(void);
	void	(*start)(struct student_t *st);
	void	(*finish)(void);
	void	(*found_match)(struct student_t *st,
				const int i, const int wish, const int sc,
				const int mysc, const int reqsc);
	void	(*found_match2)(struct student_t *st,
				const int i, const int wish,
				const int volun, const int ord);
	void	(*challenge)(struct student_t *st,
				const int i, const int wish);
	void	(*challenge_score)(struct student_t *st,
				const int i, const int wish,
				const int32_t score, const int32_t minreq);
	void	(*wish_not_exists)(struct student_t *st,
				const int i, const int wish);
	void	(*score_not_enough)(struct student_t *st,
				const int i, const int wish,
				const int32_t score, const int32_t minreq);
	void	(*fail_onsame)(struct student_t *st,
				const int i, const int wish,
				const int level, const int oscrs);
	void	(*gender_not_qualify)(struct student_t *st,
				const int i, const int wish, const int req);
	void	(*normal_univ_only)(struct student_t *st,
				const int i, const int wish);
	void	(*fail_on_military)(struct student_t *st,
				const int i, const int wish);
	void	(*need_skill_group)(struct student_t *st,
				const int i, const int wish, const int grp,
				const int mygrp);
	void	(*skill_major_score)(struct student_t *st,
				const int i, const int wish,
				const int mysc, const int reqsc);
	void	(*fail_on_instrument)(struct student_t *st,
				const int i, const int wish,
				const int mj, const int mi);
	void	(*fail_on_skill_miss)(struct student_t *st,
				const int i, const int wish,
				const int grp, const int crs);
	void	(*fail_on_skill_standard)(struct student_t *st,
				const int i, const int wish,
				const int mysc, const int stsc);
	void	(*need_c_score)(struct student_t *st,
				const int i, const int wish);
	void	(*need_pass_c_score)(struct student_t *st,
				const int i, const int wish);
	void	(*need_pass_c_standard)(struct student_t *st,
				const int i, const int wish,
				const int crs, const int crsst,
				const int scr, const int stdscr);
	void	(*missing_crs_on_stdcheck)(struct student_t *st,
				const int i, const int wish,
				const int check2);
	void	(*need_pass_s_score)(struct student_t *st,
				const int i, const int wish,
				const int check2,
				const int mysc, const int stsc);
	void	(*need_pass_s_standard)(struct student_t *st,
				const int i, const int wish,
				const int crs, const int crsst,
				const int scr, const int stdscr);
	void	(*need_exam_course)(struct student_t *st,
				const int i, const int wish,
				const int crs);

	void	(*need_skill_sub)(struct student_t *st,
				const int i, const int wish,
				const int grp, const int si);
	void	(*skill_sub_zero)(struct student_t *st,
				const int i, const int wish,
				const int grp, const int si);
	void	(*skill_sub_score)(struct student_t *st,
				const int i, const int wish,
				const int grp, const int si,
				const int mysc, const int stdsc);

	void	(*disallow_on_basic)(struct student_t *st,
				const int i, const int wish);
	void	(*disallow_on_exam)(struct student_t *st,
				const int i, const int wish);
	void	(*disallow_on_skill)(struct student_t *st,
				const int i, const int wish);
	void	(*print_mscboard)(const int level);
	void	(*write_media)(const int mtype);
	void	(*write_resultcheck)(struct student_t *st,
				const int i, const int wish);
};

struct postcheck_t	* init_postcheck_module (
				struct student_module_t *st,
				struct department_module_t *dp);

#endif
