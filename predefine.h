/*
 *	predefine.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __PREDEFINE_H_
#define __PREDEFINE_H_

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__BORLANDC__)

/*
typedef char  int8_t;
typedef short int16_t;
typedef int   int32_t;
*/

typedef __int8	int8_t;
typedef __int16	int16_t;
typedef __int32	int32_t;
typedef __int64	int64_t;

#define USE_PROCESSOR_TIME

#endif


#define MAX_PER_STUDENT_WISH		80	/* 每個學生允許的志願數 */
#define MAX_STUDENT_LEN			6	/* 學生中文姓名長度 */

#define STUDENT_ID_LENGTH		8
#define DEPARTMENT_ID_LENGTH		4


// 允許程式支援學系榜
#ifndef ENABLE_DEPARTMENT_FIRST
#define ENABLE_DEPARTMENT_FIRST		1
#endif

// 允許程式支援平行演算法
#ifndef ENABLE_PARALLEL_ALGORITHM
#define ENABLE_PARALLEL_ALGORITHM	1
#endif

// 允許程式支援 Incremental Algorithm
#ifndef ENABLE_INCREMENTAL_ALGORITHM
#define ENABLE_INCREMENTAL_ALGORITHM	1
#endif

// 允許程式支援 PTHREAD
#ifndef ENABLE_PTHREAD
#define ENABLE_PTHREAD			0
#endif

// 92 年的一些修改
#ifndef ENABLE_DISPATCHER92
#define ENABLE_DISPATCHER92		1
#endif

#ifndef ENABLE_EXTEND_DISPATCHER92
# if ENABLE_DISPATCHER92 == 1
#    define ENABLE_EXTEND_DISPATCHER92	1
# else
#    define ENABLE_EXTEND_DISPATCHER92	0
# endif
#endif

// 92 年有三個術科
#ifndef NUMBER_OF_SKILL92
#define NUMBER_OF_SKILL92		3
#endif

// 由於是用陣列來放術科的計分比及分數限制，故加一個 index
#define SK92_RATIO_INDEX		0
#define SK92_SCORE_INDEX		1
#define SK92_SUBSCR_SET			2
#define SK92_SUBSCR_NUM			5

// 當所謂的術科任一小項不得為零代表 ─ 不再乎那些未採計的術科
#define DO_NOT_CARE_ABOUT_SCORE_ON_NO_USED	1

// 在乎術科缺一事
// #define CARE_ABOUT_SKILL_MISS_OR_ZERO		1
#define CARE_ABOUT_SKILL_MISS_OR_ZERO		0

#define DULL_ELEPHANT_OPTION			1

#define ENABLE_POSTCHECK			1

// 允許程式支援完整檢定
#define ENABLE_FULL_FAIL_RESULT_CHECK		1

// 允許程式支援李子霸教授的一些東東
#ifndef ENABLE_DR_LEE_SPECIAL
#define ENABLE_DR_LEE_SPECIAL		1
#endif

#ifndef ENABLE_ONSAME_FAILECNT
#define ENABLE_ONSAME_FAILECNT		1
#endif

#ifndef ENABLE_CHALLENGE_CNT
#define ENABLE_CHALLENGE_CNT		1
#endif

// 忽略特種生類別的大小寫
#ifndef IGNORE_CASE_ON_SPECIAL
#define IGNORE_CASE_ON_SPECIAL		1
#endif

#if ENABLE_PTHREAD == 1
#   define PTHREAD_MUTEX_LOCK(x)	pthread_mutex_lock(x)
#   define PTHREAD_MUTEX_UNLOCK(x)	pthread_mutex_unlock(x)
#else
#   define PTHREAD_MUTEX_LOCK(x)
#   define PTHREAD_MUTEX_UNLOCK(x)
#endif

#if !defined (_REENTRANT) && (ENABLE_PTHREAD == 1)
#define _REENTRANT
#endif

// -Dunix -D__svr4__ -D__SVR4 -Dsun -Asystem=svr4
#if defined (sun) && defined (__svr4__)
#define SOLARIS
#endif

// 允許支援大考中心的格式
#ifndef ENABLE_CEEC_FORMAT
#define ENABLE_CEEC_FORMAT		1
#endif

// 大考的學生序號長度
#ifndef STUDENT_CEEC_SN_LENGTH
#define STUDENT_CEEC_SN_LENGTH		6
#endif

// 假定大考以序號 merge 成績檔
#ifndef ENABLE_CEEC_MERGE_SN
#define ENABLE_CEEC_MERGE_SN		0
#endif

#ifndef TRY_CEEC_MERGE_SN
#define TRY_CEEC_MERGE_SN		1
#endif


// 支援考生複查結果
#ifndef ENABLE_CHECKLIST
#define ENABLE_CHECKLIST		1
#endif


#ifndef ENABLE_PRE_TESTING
#define ENABLE_PRE_TESTING		0
#endif

// 程式處理 rotation 的方式 ...
// 在 0.1.10 版之前的, 用 0
// 版本 0.1.11 版以後的, 嘗試處理同分增額錄取, 用 1
#ifndef BEST_ADMIRER_ROTATION_VERSION
#define BEST_ADMIRER_ROTATION_VERSION	1
#endif

// 允許程式支援測試資料產生
#ifndef WITH_RANDOM_DATA_GENERATING
#  if defined(__BORLANDC__)
#    define WITH_RANDOM_DATA_GENERATING	0
#  else
#    define WITH_RANDOM_DATA_GENERATING	1
#  endif
#endif

// 程式要確定同一個志願不填兩次以上, (重覆不處理) 則設為 1
#ifndef NO_MULTIPLE_SELECT_ON_SAME_WISH
#define NO_MULTIPLE_SELECT_ON_SAME_WISH	1
#endif

// 程式是否要支援 deny list, default 是否
#ifndef ENABLE_DENYLIST
#define ENABLE_DENYLIST			0
#endif

// 舊版的 (0.1.4 之前) 術科計算 patch
// 0 表舊版, 維持 0.1.4 版以前之算法
// 1 表新版

#ifndef SKILL_COUNT_VERSION
#define SKILL_COUNT_VERSION		1
#endif

// 術科檢定的分數是計算來的 ...
// 在舊版中 (0.1.5 之前) 術科檢定是直接用 score 檔上已經算好的
// 成績 (維持舊版的要設成 0)

#ifndef SKILL_CHECK_BY_CALCULATE
#define SKILL_CHECK_BY_CALCULATE	1
#endif


#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL	1
#endif

// 使用加分後的原始總分做檢定
#ifndef USE_ADDED_VALUE_AS_CHECK
#define USE_ADDED_VALUE_AS_CHECK	1
#endif

// 術科依學科加權後, 再計算學科術科比
// 在 90 年時 SKILL_WEIGHT_ON_MAIN_WEIGHT == 0
//    91 年的簡章也是要設成 0
//    但可能會改成設成 1 才比較合理
//
// 自 0.1.16 版之後, 都要設成 1, 而實際計算方法在 run-time 時
// 由 config file 的 skill-relative-weight = on/off 來決定
#ifndef SKILL_WEIGHT_ON_MAIN_WEIGHT
#define SKILL_WEIGHT_ON_MAIN_WEIGHT	1
#endif

// (((學科加權) + (特種生加分)) * 學科比) + (術科成績 * 學科加權 * 術科比)
//
// ROUND_ON_MAIN_WEIGHT
// ROUND_ON_SPECIAL
// ROUND_ON_MAIN_WEIGHT_PLUS_SPECIAL
// ROUND_ON_MAIN_SUBTOTAL
// ROUND_ON_SKILL_SUBTOTAL
//
#ifndef ROUND_ON_MAIN_WEIGHT
#define ROUND_ON_MAIN_WEIGHT			0
#endif

#ifndef ROUND_ON_SPECIAL
#define ROUND_ON_SPECIAL			0
#endif

#ifndef ROUND_ON_MAIN_WEIGHT_PLUS_SPECIAL
#define	ROUND_ON_MAIN_WEIGHT_PLUS_SPECIAL	0
#endif

#ifndef ROUND_ON_MAIN_SUBTOTAL
#define ROUND_ON_MAIN_SUBTOTAL			0
#endif

#ifndef ROUND_ON_SKILL_SUBTOTAL
#define ROUND_ON_SKILL_SUBTOTAL			0
#endif

#if DEPARTMENT_ID_LENGTH == 4
#  define WISHID_FMT	"%04o"
#else
#  define WISHID_FMT	"%05d"
#endif


#if defined(__cplusplus)
}
#endif

#endif
