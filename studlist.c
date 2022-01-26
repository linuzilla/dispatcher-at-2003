/*
 *	studlist.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <stdio.h>
#include <stdlib.h>
#include "predefine.h"

#if ENABLE_PTHREAD == 1
#include <pthread.h>
#endif

#include "global_var.h"
#include "misclib.h"
#include "studlist.h"

static struct student_list_t	*lslhead = NULL, *lslfreehead = NULL;

#if ENABLE_PTHREAD == 1
static pthread_mutex_t		mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static int lsl_put (struct student_t *stud) {
	struct student_list_t	*ptr;

#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_LOCK   (&mutex);
#endif
	if (lslfreehead != NULL) {
		ptr = lslfreehead;
		lslfreehead = ptr->next;
	} else {
		ptr = misc->malloc (sizeof (struct student_list_t));
		ptr->ptr = NULL;
	}

	ptr->st   = stud;
	ptr->next = lslhead;
	lslhead   = ptr;

#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_UNLOCK   (&mutex);
#endif
	return 1;
}

static int lsl_put_list (struct student_list_t *ptr) {
	struct student_list_t	*q;

	if ((q = ptr) == NULL) return 0;

#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_LOCK   (&mutex);
#endif
	while (q->next != NULL) q = q->next;

	q->next = lslhead;
	lslhead  = ptr;
#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_UNLOCK   (&mutex);
#endif

	return 1;
}

static struct student_t * lsl_get (void) {
	struct student_t	*stud = NULL;
	struct student_list_t	*ptr;

#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_LOCK   (&mutex);
#endif
	if (lslhead != NULL) {
		stud = lslhead->st;

		ptr = lslhead;
		lslhead = ptr->next;

		ptr->next = lslfreehead;
		lslfreehead = ptr;
	}
#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_UNLOCK   (&mutex);
#endif

	return stud;
}

struct lost_student_list_t *  initial_lostlist_module (void) {
	static struct lost_student_list_t	lslval;
	lslval.get      = lsl_get;
	lslval.put      = lsl_put;
	lslval.put_list = lsl_put_list;

	return &lslval;
}

struct student_list_t	*allocate_student_list (const int num) {
	struct student_list_t	*ptr, *q;
	int			i;

	i = 0;
#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_LOCK   (&mutex);
#endif

	if ((ptr = q = lslfreehead) != NULL) {
		for (i = 1; i < num; i++) {
			if (q->next != NULL) {
				q = q->next;
			} else {
				break;
			}
		}

		lslfreehead = q->next;
		q->next = NULL;
	}

	for (; i < num; i++) {
		q = misc->malloc (sizeof (struct student_list_t));
		q->ptr = NULL;
		q->next = ptr;
		ptr = q;
	}
#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_UNLOCK   (&mutex);
#endif

	return ptr;
}

void free_student_list (struct student_list_t *list) {
	struct student_list_t	*ptr;

#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_LOCK   (&mutex);
#endif
	if ((ptr = list) != NULL) {
		while (ptr->next == NULL) ptr = ptr->next;

		ptr->next = lslfreehead;
		lslfreehead = list;
	}
#if ENABLE_PTHREAD == 1
	PTHREAD_MUTEX_UNLOCK   (&mutex);
#endif
}
