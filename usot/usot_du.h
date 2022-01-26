/*
 *	usot/usot_du.h
 *
 *	Copyright (c) 2003, Jainn-Ching Liu
 */

#ifndef __USOT_DU_H__
#define __USOT_DU_H__

#include "usot_dtst.h"

#define MAX_USOT_CRS_TYPE	4

struct usot_department_t;
struct usot_student_t;
struct usot_department_module_t;
struct usot_student_module_t;
struct lost_student_list_t;

struct usot_dispatcher_unit_t {
	struct usot_department_t	*dp;
	struct usot_student_t		*st;
	int32_t				flags;
	int32_t				score;
	int32_t				orgscore;
	int32_t				orgsc[MAX_USOT_CRS_TYPE];
	int8_t				passok;
	struct usot_courseset_t		*crset;
	int16_t				crsid[MAX_USOT_EXAM_COURSE];

	int		(*load)(struct usot_dispatcher_unit_t *du,
					const int16_t wish);
	void		(*bind)(struct usot_dispatcher_unit_t *du,
				struct usot_student_t *st);
	int		(*check_all_req)(struct usot_dispatcher_unit_t *du);
	int		(*is_pass)(struct usot_dispatcher_unit_t *du);
	int		(*is_acceptable)(struct usot_dispatcher_unit_t *du,
				const int tryres);
	int		(*try_accept)(struct usot_dispatcher_unit_t *du,
				const int tryres);
	int32_t*	(*get_onsame)(struct usot_dispatcher_unit_t *du);

	struct lost_student_list_t *
			(*lostlist) (struct usot_dispatcher_unit_t *du);
};

struct usot_dispatcher_unit_t * allocate_usot_dispatcher_unit (
		struct usot_department_module_t *,
		struct usot_student_module_t *);

#endif
