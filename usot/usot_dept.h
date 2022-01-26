/*
 *	usot/usot_dept.h
 *
 *	Copyright (c) 2003, Jainn-Ching Liu
 */

#ifndef __USOT_DEPT_H__
#define __USOT_DEPT_H__

#include "usot_dtst.h"

#define USOT_DEPT_FLAG_HAVE_CRSSET	1
#define USOT_DEPT_FLAG_HAVE_DEPNAME	2

struct student_list_t;

struct usot_course_t {
	int16_t		crsid;
	char		crsname[USOT_CRSNAME_LENGTH + 1];
	int		crsgrp;
};

struct usot_courseset_t {
	int16_t			excrs[MAX_USOT_EXAM_COURSE];
	int16_t			*crslist;
	int16_t			num;
	struct usot_courseset_t	*next;
};

struct usot_student_on_wish_t {
	struct usot_student_t	*st;
	int32_t			score;
	int16_t			order;
	int16_t			wish_index;
	int32_t			onsame[MAX_USOT_EXAM_COURSE];
};

struct usot_department_t {
	int32_t			depid;
	int16_t			dflag;
	int16_t			num;	// 錄取名額
	int16_t			res;	// 備取名額
	int8_t			gender;
	int32_t			exam[MAX_USOT_EXAM_COURSE];
	char			onsame[MAX_USOT_EXAM_COURSE];
	int32_t			lowsco[MAX_USOT_EXAM_COURSE];
	int32_t			lowsum;
	int8_t			lim;
	int16_t			limcnt;	// 組數
	int16_t			limsum;	// 組數
	struct usot_courseset_t	*crset;

	char			school[USOT_SCHNAME_LENGTH + 1];
	char			depname[USOT_DEPNAME_LENGTH + 1];
	int16_t			uclass;
	int8_t			age;

	int32_t			minreq;	// 最低錄取分數

	int16_t			inlistcnt;  // 目前錄取人數
	int16_t			lastidx;    // lastidx on stdlist
	int16_t			llindex;    // lastlist index (給 lastlist 用)

	int8_t			max_onsame_use;	// 最高的同分參酌比對
	int32_t			min_onsame[MAX_USOT_EXAM_COURSE];

	struct usot_student_on_wish_t	*stdlist;
	struct student_list_t	*lastlist;

	struct usot_department_t	*rdep;
	// int16_t			res_lastidx;
	// struct usot_student_on_wish_t	*res_stdlist;
};

struct usot_department_module_t {
	int		(*load_limits)(void);
	int		(*load_course_set)(void);
	////////////////////////////////////////////////////
	int		(*load_courses)(void);
	int		(*load_depna)(void);

	int		(*allocate_space)(void);

	int		(*sort_students)(struct usot_department_t *dp);
	void		(*write_wishfinal)(void);
	void		(*write_wishminimal)(void);

	struct usot_department_t *	(*find)(const int16_t);
	int		(*find_course_group)(const int16_t);
};

struct usot_department_module_t  *initial_usot_department_module (void);

#endif
