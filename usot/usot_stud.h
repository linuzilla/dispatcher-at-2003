/*
 *	usot/usot_stud.h
 *
 *	Copyright (c) 2003, Jainn-Ching Liu
 */

#ifndef __USOT_STUD_H__
#define __USOT_STUD_H__

#include "usot_dtst.h"

#define USOT_STUDENT_FLAG_HAVE_INFO	1
#define USOT_STUDENT_FLAG_HAVE_SCORE	2

struct usot_student_t {
	int32_t		sid;
	int16_t		sflag;
	int16_t		wish_index;
	int8_t		have_wish[MAX_USOT_WISHES];
	int16_t		wish[MAX_USOT_WISHES];
	int16_t		uclass;
	int8_t		gender;
	int8_t		ptype;
	int8_t		check;	// 審
	int16_t		crs[MAX_USOT_EXAM_COURSE];	// 選考科目
	int32_t		sco[MAX_USOT_EXAM_COURSE];	// 成績
	int8_t		miss[MAX_USOT_EXAM_COURSE];
};

struct usot_student_module_t {
	int			(*load_score)(void);
	int			(*load_wishes)(void);
	int			(*load_info)(void);
	int			(*prepare_sorted_stack)(void);
	void			(*free_stack) (void);
	struct usot_student_t *	(*pop)(void);
	int			(*push)(struct usot_student_t *st);
	int16_t			(*nextwish)(struct usot_student_t *st);
	void			(*reset_wish) (struct usot_student_t *st);
};

struct usot_student_module_t  *initial_usot_student_module (void);
#endif
