/*
 *	dispunit.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __DISPATCHER_UNIT_H_
#define __DISPATCHER_UNIT_H_

#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct student_t;
struct student_module_t;
struct department_t;
struct department_module_t;

// 結合學生與志願的資料 - 用以分發的各種檢核
//
// 目的是存放一些狀態資料, 只要不重新 load 或 bind
// 各種檢驗的 state 可以分開執行, 以方便驗證

#define DUTF_CHECK1_IS_CHECK		  1
#define DUTF_CHECK2_IS_CHECK		  2
#define DUTF_CHECK1_IS_PASS		  4
#define DUTF_CHECK2_IS_PASS		  8
#define DUTF_HAVE_CHECK_ACCEPT		 16
#define DUTF_IS_ACCEPT			 32
#define DUTF_HAVE_SCORE			 64
#define DUTF_ADD_TO_STDLIST		128
#define DUTF_SEX_IS_CHECK		256
#define DUTF_SEXCHECK_IS_PASS		512
#define DUTF_SKILL_IS_CHECK		1024
#define DUTF_SKILLCHECK_IS_PASS		2048


struct dispatcher_unit_t {
	struct department_t		*dp;
	struct student_t		*st;
	int16_t				flags;
	int32_t				score;
	int32_t				orgscore;
	int32_t				addtotoal;
#if ENABLE_DISPATCHER92 == 1
	int32_t				skill_score;
#endif
	int	 (*load)(struct dispatcher_unit_t *du, const int16_t wish);
	int	 (*load_index)(struct dispatcher_unit_t *du, const int idx);
	int	 (*load_test)(struct dispatcher_unit_t *du, const int wmonly);
	void	 (*bind)(struct dispatcher_unit_t *du, struct student_t *st);
	int	 (*bind_student)(struct dispatcher_unit_t *du,
			const int32_t sid);
	int	 (*check_sex)(struct dispatcher_unit_t *du);
	int	 (*check_basic)(struct dispatcher_unit_t *du);
	int	 (*check_exam)(struct dispatcher_unit_t *du);
	int	 (*check_skill)(struct dispatcher_unit_t *du);
	int	 (*check_special)(struct dispatcher_unit_t *du);
	int32_t	 (*min_require)(struct dispatcher_unit_t *du);
	int32_t	 (*get_score)(struct dispatcher_unit_t *du);
	int	 (*try_accept)(struct dispatcher_unit_t *du);
	int32_t* (*get_onsame)(struct dispatcher_unit_t *du);
#if ENABLE_PRE_TESTING
	int	 (*pre_checkall)(struct dispatcher_unit_t *du, const int mask);
#endif
#if ENABLE_DEPARTMENT_FIRST == 1
	int	 (*load_wish_order)(struct dispatcher_unit_t *du, const int od);
	void	 (*set_wishidx)(struct dispatcher_unit_t *du);
	void	 (*do_accept)(struct dispatcher_unit_t *du);
	void	 (*inc_valid_student)(struct dispatcher_unit_t *du);
#endif
	struct lost_student_list_t *
		 (*lostlist) (struct dispatcher_unit_t *du);
};

struct dispatcher_unit_t *
		allocate_dispatcher_unit (struct department_module_t *,
				struct student_module_t *);

#if defined(__cplusplus)
}
#endif

#endif
