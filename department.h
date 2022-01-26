/*
 *	department.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __DEPARTMENT_H_
#define __DEPARTMENT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <sys/types.h>
#include <stdio.h>
#include "predefine.h"

// 最多七個同分參酌
#define MAXIMUM_ON_SAME_REFERENCE	7

struct student_t;
struct student_flag_t;
struct dispatcher_unit_t;
struct skillset_t;

typedef struct student_t *student_ptr_t;

#if ENABLE_DEPARTMENT_FIRST == 1
#define SWF_INVALID		1	/* 已經高就的 */
#define SWF_ADMIRER_CHECK	2	/* 進入 Admirer Check 中 */
#endif

#define BARF_CHECK_INITIALIZED	1	/* 驗榜用, 是否已經 initialized */
#define BARF_INVALID		2	/* 不可能進入 loop 中 */
#define BARF_IN_LOOP		4	/* 進入 BAR loop 中 */
#define BARF_LOOP_DETECT	8	/* 偵測到在 loop 中 */

struct student_on_wish_t {
	union {
		struct student_t	*st;
		int			id;
	};
	int16_t			order;	// 在學系上的名次用
	int32_t			score;
	int32_t			onsame[MAXIMUM_ON_SAME_REFERENCE];
#if ENABLE_DISPATCHER92 == 1
	int32_t			skill_score;
#endif
#if ENABLE_DEPARTMENT_FIRST == 1
	int8_t			aflag;	//
	int16_t			wish_order;	// 從 1 起算
#endif
};

struct university_t {
	char			*sch_name;
	int			schid;
	struct university_t	*next;
};

struct department_t {
	struct university_t	*univ;
	char			*cname;
#if ENABLE_DEPARTMENT_FIRST == 1
	int16_t		index;		// 索引 - 在 list 中的
#endif
	int32_t		id;		// 志願號
	int16_t		num;		// 名額
	int8_t		sex;		// 性別限制 0, 1, 2
	int8_t		take_sort;	// 案別: 甲(5)、乙(6)、丙(1-4)
	int8_t		cstgrp;		// 甲、乙、丙 (0 - 2)
	int8_t		check1;		// 學測一般檢定
	int8_t		test1[5];	// 學測五科的標準:無、頂、前、均、後、底
	int8_t		check2;		// 指定科目考試的一般檢定
	int8_t		checkskill;	// 術科檢定
	int8_t		test2[9];	// 九科的標準:高、均、低
	int8_t		weight[9];	// 九科的採計
	int8_t		skillweight;	// 術科採計
	int8_t		skillgroup;	// 術科組別
	int8_t		skillway;	// 術科成績計算方式:單科加權、固定比
	int8_t		skill_main;	// 術科學科比 - 學科
	int8_t		skill_skill;	// 術科學科比 - 術科
	int16_t		skill_major;	// 術科主修分數限制
	int8_t		special;	// 1-> 一般校系, 2-> 國防 3-> 師範
	int8_t		rflag;		// rotation flag
					// 驗榜用看看是不是 initialized
	struct skillset_t
			*skillset;	// 術科主副修科目限制
	int8_t		onsame[MAXIMUM_ON_SAME_REFERENCE]; // 同分參酌
	//	以下為 Run-time 時才使用的
	// int8_t	used_onsame;	// 使用到的同分參酌
	int8_t		max_onsame_use;	// 最高的同分參酌比對
	int32_t		min_onsame[MAXIMUM_ON_SAME_REFERENCE];
					// 最小的同分參酌 ... 驗榜用
	int32_t		fail_onsame_sid;

#if ENABLE_DISPATCHER92 == 1
	int8_t		skill_lim[2][2][5];
	int8_t		skill_zero;
#endif
	int32_t		max_orgsum;	// 本系最高的原始分數
	int32_t		avg_orgsum_n;	// 本系錄取原始分數之平均
	int32_t		min_orgsum;	// 本系最低的原始分數
	int32_t		min_orgsum_n;	// 本系最低的原始分數 - 非特種生
	int8_t		min_gradesum;	// 本系最低的總級分
	double		variation;

	int16_t		number_of_special;	// 特種生數
	int16_t		deplim_grp;		// 組
	int8_t		number_of_refcrs;	// 採計科目數 (學科)
#if ENABLE_PTHREAD == 1
	pthread_mutex_t	mutex;
#endif
#if SKILL_WEIGHT_ON_MAIN_WEIGHT == 1
	double		ccnt;		// 採計的加權數
#else
	int8_t		ccnt;		// 採計的科目數
#endif
	int8_t		crsnum_cnt;	// 採計的科目數
	int16_t		inlistcnt;	// 目前錄取人數
	int16_t		lastidx;	// lastidx on stdlist
	int16_t		llindex;	// lastlist index (給 lastlist 用)
					//   但由於在學系榜無用, 所以在學系
					//   榜時, 用來記錄將被錄取的最後一
					//   名的索引

	union {
		int32_t	minreq;		// 最低錄取分數 [學生榜]
		int	fix_require;	// 是否需要重排 [學系榜]
	};

#if ENABLE_DEPARTMENT_FIRST == 1
	//int16_t		rotation;	// 是否可能參與 rotation ?
	int16_t		real_num;	// 真實名額
					// [for 學系榜] 剛開始時, 先將員額
					//   擴增, 這個變數用來記錄原始員額
					//   等實際運用時, 用來記錄錄取名額
	int16_t		student_cnt;	// 多少學生有填此志願
					// 用在學系榜上
#endif
	int16_t		reject_cnt;	// 落選生
	int16_t		n_qualify_cnt;	// 檢定不 qualify
	struct student_list_t		*lastlist;	// 最後一名又同分的
							// 在學系榜上是無用的
	struct student_on_wish_t	*stdlist;
};

struct department_module_t {
	int		(*load_limits)	  (void);
	int		(*analy_deplim_grp)(void);
	int		(*load_skillset)  (void);
	int		(*load_wishminimal)(void);
	int		(*load_depna)	  (void);
	int		(*allocate_space) (void);
	int		(*sort_students)  (struct department_t *dp);
	int16_t		(*reset_wish)	  (void);
	int16_t		(*next_wish)	  (void);
	int		(*print_list)	  (FILE *, const int16_t);
	int		(*print_minimal)  (FILE *, const int16_t,
						const int, const int);
	int		(*print_minimal_proto)  (FILE *, const int16_t,
							const int, const int,
							const int8_t *);
	int		(*print_minimal_special)  (FILE *, const int16_t);
	int64_t		(*skill_bitmap)	  (const int grp, const int crs);
//	void		(*write_student_matched_wish) (void);
	void		(*calculate_statistic) (const int16_t wish);
#if ENABLE_PARALLEL_ALGORITHM == 1
	struct department_t *	(*get_depbuf)(void);
	int			(*number_of_departments) (void);
	int			(*compare_order)(
					struct student_on_wish_t *st1,
                                        struct student_on_wish_t *st2);
#endif
#if ENABLE_DEPARTMENT_FIRST == 1
	int		(*do_require_fix) (void);
	int		(*do_rotate) (void);
	//const char*	(*rotation_engine_version)(void);
	void		(*re_arrange_ranklist)  (void);
	void		(*invalid_wishes_after) (void);
	void		(*sort_all_ranklist) (void);
	void		(*fix_all_department_member)(void);
	void		(*set_wish_cross_reference) (void);
	void		(*extend_space)   (void);
#  if NO_MULTIPLE_SELECT_ON_SAME_WISH == 1
	void		(*clear_use_log)(void);
	int		(*test_and_set)(const int idx);
#  endif
#endif
	//int		(*load)(int16_t, struct dispatcher_unit_t *);
	struct department_t *	(*find)(const int16_t);
	struct department_t *	(*find_index)(const int);
};

struct department_module_t *	initial_department_module (const int outform);


#if defined(__cplusplus)
}
#endif

#endif
