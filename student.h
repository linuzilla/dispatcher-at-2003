/*
 *	student.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __STUDENT_H_
#define __STUDENT_H_

#include <sys/types.h>
#include <stdio.h>
#include "predefine.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define STUDENT_FLAG_PASS1		   1
#define STUDENT_FLAG_HAVE_SCORE		   2
#define STUDENT_FLAG_HAVE_INFO		   4
#define STUDENT_FLAG_GIRL		   8	/* 女生 */
#define STUDENT_FLAG_ADD		  16	/* 增加錄取總分 */
#define STUDENT_FLAG_LOW		  32	/* 降低錄取標準 */
#define STUDENT_FLAG_DISALLOW		  64	/* 不符合登記 */
#define STUDENT_FLAG_ADMIRER		 128	/* Student in Best Admirer */
#define STUDENT_FLAG_SKILL_MISS		 256
#define STUDENT_FLAG_HAVE_CEEC_BASIC	 512
#define STUDENT_FLAG_HAVE_CEEC_EXAM	1024
#define STUDENT_FLAG_HAVE_CEEC_INFO	2048
#define STUDENT_FLAG_NU_ONLY		4096	/* 只能考師範 */
#define STUDENT_FLAG_DF_ALLOW		8192	/* 允許考國防 */
// #define STUDENT_FLAG_AGAINST		  64	/* 違規 */
// #define STUDENT_FLAG_BOY		   2	/* 男生 */

#define STDSTATE_UNKNOW			 0
#define STDSTATE_FREE			 1
#define STDSTATE_FINAL			 2
#define STDSTATE_BOUND			 4
#define STDSTATE_REJECT			 8
#define STDSTATE_HAVE_SAFE		16

#define STDWR_FOUND_MATCH		 1
#define STDWR_FOUND_MATCH2		 2
#define STDWR_CHALLENGE_SCORE		 3
#define STDWR_CHALLENGE			 4
#define STDWR_WISH_NOT_EXIST		11
#define STDWR_SCORE_NOT_ENOUGH		12
#define STDWR_FAIL_ONSAME		13
#define STDWR_GENDER_NOT_QUALIFY	14
#define STDWR_NORMAL_UNIV_ONLY		15
#define STDWR_FAIL_ON_MILITARY		16
#define STDWR_NEED_SKILL_GROUP		17
#define STDWR_SKILL_MAJOR_SCORE		18
#define STDWR_FAIL_ON_INSTRUMENT	19
#define STDWR_FAIL_ON_SKILL_MISS	20
#define STDWR_FAIL_ON_SKILL_STANDARD	21
#define STDWR_NEED_C_SCORE		22
#define STDWR_NEED_PASS_C_SCORE		23
#define STDWR_NEED_PASS_C_STANDARD	24
#define STDWR_MISSING_CRS_ON_STDCHECK	25
#define STDWR_NEED_PASS_S_SCORE		26
#define STDWR_NEED_PASS_S_STANDARD	27
#define STDWR_NEED_EXAM_COURSE		28
#define STDWR_NEED_SKILL_SUB		29
#define STDWR_SKILL_SUB_ZERO		30
#define STDWR_SKILL_SUB_SCORE		31
#define STDWR_DISALLOW_ON_BASIC		32
#define STDWR_DISALLOW_ON_EXAM		33
#define STDWR_DISALLOW_ON_SKILL		34

#define STSCD_DISALLOW_BASIC		1
#define STSCD_DISALLOW_EXAM		2
#define STSCD_DISALLOW_SKILL		4

#define STNQF_WISH_NOT_FOUND		       1
#define STNQF_STUD_NOT_QUALIFY		       2
#define STNQF_MILITARY_NOT_QUALIFY	       4
#define STNQF_NORMAL_UNIV_ONLY		       8
#define STNQF_GENDER			      16
#define STNQF_BASIC_STANDARD		      32
#define STNQF_BASIC_CRS(x)		(((x) == 0) ? 64 : (64 << (x)))
//		64/128/256/512/1024
#define STNQF_EXAM(x)			(((x) == 0) ? 2048 : (2048 << (x)))
//		2048/4096/8192/16384/32768/65536/131072/262144/524288
#define STNQF_EXAM_STANDARD		 1048576
#define STNQF_NO_SKILL_ITEM		 2097152
#define STNQF_SKILL_ITEM_ZERO		 4194304
#define STNQF_SKILL_ITEM_NQ		 8388608
#define STNQF_INSTRUMENT		16777216
#define STNQF_SCORE_DISALLOW		33554432
#define STNQF_SCORE_NOT_ENOUGH		67108864

struct student_extend_info_t {
	char	name[MAX_STUDENT_LEN * 2 + 1];
	int	birthday;
};

struct student_extend_info_level2_t {
	struct student_extend_info_t;
	char	person_id[10];
	char	regist_sn[8];
	char	address[74];
	char	cmaddr[60];
	char	edu_level[3];
	char	tel_no[10];
	char	zip_code[3];
	char	edu_year[2];
	char	guard_man[12];
};

struct stdwish_result_t {
	int16_t	rcode;
	int16_t	wish;
	int32_t	args[4];
};

struct student_check_info_t {
//	int	check_sn;
	char	check_sn[7];
	char	*check_description;
	int	matched_idx;
	int	new_matched_idx;
	//int16_t	wish[MAX_PER_STUDENT_WISH];	// 學生的志願
	struct stdwish_result_t	rs[MAX_PER_STUDENT_WISH];
};


#if ENABLE_DISPATCHER92 == 1
struct student_skill_info_t {
#  if TRY_CEEC_MERGE_SN == 1
	int32_t		serial;		// student's serial number
#  endif
#  if ENABLE_EXTEND_DISPATCHER92
	int32_t		exam_sid;
	int8_t		skill_group;
	int32_t		skill_score;
#  endif
	int32_t		own_by;
	int32_t		skill_sid[NUMBER_OF_SKILL92];
	int16_t		music_major;
	int16_t		music_minor;
	int8_t		sk_miss[SK92_SUBSCR_SET][SK92_SUBSCR_NUM];
	int8_t		physical_miss;
	int16_t		sk_score[SK92_SUBSCR_SET][SK92_SUBSCR_NUM];
	int16_t		physical_score;
	int16_t		physical_grade;
	// int32_t	sksc;	// 術科成績 x 10000
	int32_t		sksc_onwish[MAX_PER_STUDENT_WISH];
	int8_t		disallow;
}; //  __attribute__ ((__packed__));
#endif

struct student_t {
	int32_t		sid;		// student's identifier
#if ENABLE_DISPATCHER92 == 1
	int32_t		extsid;		// sid for checking
#endif
#if ENABLE_CEEC_FORMAT
#  if ENABLE_CEEC_MERGE_SN == 1
	int32_t		sn;		// student's serial number
#  else
#    if TRY_CEEC_MERGE_SN == 1
	int32_t		serial;
#    endif
	int32_t		usedid[3];	// 三個考試的 student id
#  endif
#endif
	int8_t		score_disallow;
	int8_t		gradscore[5];	// 學測級分 -1, 0 - 15
	int8_t		gradmiss[5];
	int8_t		gradsum;	// 總級分
	int16_t		testscore[9];	// 指定科目成績 x 100
	int8_t		testmiss[9];	// 指科缺考
	int8_t		skill_group;	// 術科組別
	int32_t		skillscore;	// 術科成績

#if SKILL_CHECK_BY_CALCULATE == 0
	int32_t		skillcheck;	// 術科的加權合計, 檢定用
#endif
	int16_t		skillmajor_score;// 術科主修科目成績
	int16_t		skillmajor;	// 術科主修科目
	int16_t		skillminor;	// 術科副修科目
	int8_t		wish_index;	// 程式中使用, 暫時分發志願序
	char		ptype;
	union {
		int16_t		matched_wish;	// 驗榜程式用
					// 但後來做學系榜時, 因 rotation 的
					// 需要而使用。
		int16_t		max_possible;
	};
	int32_t		org_score;	// 原始成績
	int32_t		final_score;	// 錄取分數
	int16_t		sflag;		// 0 -> undef or fail, 1 -> pass
	int8_t		ratio;		// 降低標準或增加總分的百分比
	int8_t		number_of_wishes;
#if ENABLE_ONSAME_FAILECNT == 1
	int8_t		onsame_failcnt;
	int8_t		grp_onsame_failcnt[4];
#endif
#if ENABLE_CHALLENGE_CNT == 1
	int8_t		challenge_cnt;
#endif
	int16_t		wish[MAX_PER_STUDENT_WISH];	// 學生的志願
	int16_t		*org_wish;

	int16_t		highsch;		// 高中代碼

#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
	int32_t		not_qualify_flag;	// 檢定未通過原因
#endif
	union {
		struct student_extend_info_t		*info;
		struct student_extend_info_level2_t	*infol2;
	};
#if ENABLE_DISPATCHER92 == 1
	struct student_skill_info_t	*skinfo;
	int32_t				skill_sid[NUMBER_OF_SKILL92];
#endif
#if ENABLE_CHECKLIST == 1
	struct student_check_info_t	*ck;
#endif

	union {
		int16_t	atleast_wish;   // 最少會被錄取的志願 (學系榜)
		int16_t	still_checking;	// 找到了 match wish (驗榜)
	};
#if ENABLE_DEPARTMENT_FIRST == 1
	int16_t		*wishidx;	// 加速用, 不要每次
						       // 都要來一次 Bsearch
	union {
		int16_t	*order_on_wish; 		// 也是加速用,
							// 記錄每個志願的名次
		int16_t stdstate;
	};
	//struct student_on_wish_t	*stdwish[MAX_PER_STUDENT_WISH];

#  if BEST_ADMIRER_ROTATION_VERSION > 0
	int16_t		admire_order;
#  endif
#endif
}; //  __attribute__ ((__packed__));

struct student_module_t {
	int			(*load_score)   (const int additional);
#if ENABLE_INCREMENTAL_ALGORITHM == 1
	int			(*load_all_students)(const int dummywish);
#endif
	int			(*load_wishes)  (const int additional,
						 const int addonly);
	int			(*load_new_wishes)  (void);
	int			(*load_checklist) (const int level);
	int			(*load_checklist_all) (const int level);
	int			(*load_info)	(const int additional);
//	int			(*load_special) (void);
	int			(*prepare_sorted_stack) (const int sorted);
	void			(*free_stack) (void);
	struct student_t *	(*pop)        (void);
	int			(*push)       (struct student_t *st);
	int16_t			(*nextwish)   (struct student_t *st);
	int16_t			(*nextwish_with_order) (struct student_t *st,
						int *order);
	void			(*reset_wish) (struct student_t *st);
	int			(*get_wish_index) (struct student_t *st);
#if ENABLE_PARALLEL_ALGORITHM == 1
	struct student_t *	(*get_stdbuf)(void);
#endif
	int			(*number_of_students) (void);
	struct student_t *	(*find)(const int32_t);
	void			(*print_all)(FILE *);
#if ENABLE_DEPARTMENT_FIRST == 1
	void			(*set_wishidx) (struct student_t *st,
						const int nw, const int idx);
	void			(*set_valid_wish_mask)(struct student_t *st,
					const short *mask);
#endif
#if ENABLE_DENYLIST == 1
	int			(*load_deny) (void);
	int			(*deny)(const int32_t sid, const int16_t wish);
#endif
//	struct student_flag_t *	(*check_special)  (struct student_t *st);
};

struct student_module_t *	initial_student_module (void);

#if defined(__cplusplus)
}
#endif

#endif
