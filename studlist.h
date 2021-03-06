/*
 *	studlist.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __STUDENT_LIST_H_
#define __STUDENT_LIST_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct student_t;

struct student_list_t {
	struct student_t	*st;
	void			*ptr;
	struct student_list_t	*next;
};

struct lost_student_list_t {
	int			(*put)(struct student_t *);
	int			(*put_list)(struct student_list_t *);
	struct student_t *	(*get)(void);
};

struct lost_student_list_t *	initial_lostlist_module (void);

struct student_list_t	*allocate_student_list (const int num);
void			free_student_list (struct student_list_t *list);

#if defined(__cplusplus)
}
#endif

#endif
