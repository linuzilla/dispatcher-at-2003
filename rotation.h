/*
 *	rotation.h
 *
 *	Copyright (c) 2001 Jiann-Ching Liu
 */

#ifndef __ROTATION_H_
#define __ROTATION_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct department_t;
struct student_on_wish_t;

struct department_internal_data_t {
	struct department_t	*depbuf;
	int			depnum;

	void		(*write_matched_wish)(void);
	int		(*first_admirer)(const int i, const int from, int *r);
	int		(*next_admirer)(const int i, const int from, int *r);
	void		(*invalid_rest_wishes)(struct student_on_wish_t *sw);
	int		(*compare_order)(struct student_on_wish_t *st1,
						struct student_on_wish_t *st2);
	int		(*do_require_fix)(void);
};

struct rotation_algorithm_t {
	int	(*init)(struct department_internal_data_t *); 
	int	(*prepare)(void);
	int	(*try_rotation)(int *ro, int *sf, int *rc);
};

int regist_rotation_algorithm (struct rotation_algorithm_t *ram);

void regist_rotation_algorithm_version_1 (void);
void regist_rotation_algorithm_version_2 (void);
void regist_rotation_algorithm_version_3 (void);
void regist_rotation_algorithm_version_4 (void);
void regist_rotation_algorithm_version_5 (void);

#if defined(__cplusplus)
}
#endif

#endif
