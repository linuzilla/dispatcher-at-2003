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


#define MAX_PER_STUDENT_WISH		80	/* �C�Ӿǥͤ��\�����@�� */
#define MAX_STUDENT_LEN			6	/* �ǥͤ���m�W���� */

#define STUDENT_ID_LENGTH		8
#define DEPARTMENT_ID_LENGTH		4


// ���\�{���䴩�Ǩt�]
#ifndef ENABLE_DEPARTMENT_FIRST
#define ENABLE_DEPARTMENT_FIRST		1
#endif

// ���\�{���䴩����t��k
#ifndef ENABLE_PARALLEL_ALGORITHM
#define ENABLE_PARALLEL_ALGORITHM	1
#endif

// ���\�{���䴩 Incremental Algorithm
#ifndef ENABLE_INCREMENTAL_ALGORITHM
#define ENABLE_INCREMENTAL_ALGORITHM	1
#endif

// ���\�{���䴩 PTHREAD
#ifndef ENABLE_PTHREAD
#define ENABLE_PTHREAD			0
#endif

// 92 �~���@�ǭק�
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

// 92 �~���T�ӳN��
#ifndef NUMBER_OF_SKILL92
#define NUMBER_OF_SKILL92		3
#endif

// �ѩ�O�ΰ}�C�ө�N�쪺�p����Τ��ƭ���A�G�[�@�� index
#define SK92_RATIO_INDEX		0
#define SK92_SCORE_INDEX		1
#define SK92_SUBSCR_SET			2
#define SK92_SUBSCR_NUM			5

// ��ҿת��N����@�p�����o���s�N�� �w ���A�G���ǥ��ĭp���N��
#define DO_NOT_CARE_ABOUT_SCORE_ON_NO_USED	1

// �b�G�N��ʤ@��
// #define CARE_ABOUT_SKILL_MISS_OR_ZERO		1
#define CARE_ABOUT_SKILL_MISS_OR_ZERO		0

#define DULL_ELEPHANT_OPTION			1

#define ENABLE_POSTCHECK			1

// ���\�{���䴩�����˩w
#define ENABLE_FULL_FAIL_RESULT_CHECK		1

// ���\�{���䴩���l�Q�бª��@�ǪF�F
#ifndef ENABLE_DR_LEE_SPECIAL
#define ENABLE_DR_LEE_SPECIAL		1
#endif

#ifndef ENABLE_ONSAME_FAILECNT
#define ENABLE_ONSAME_FAILECNT		1
#endif

#ifndef ENABLE_CHALLENGE_CNT
#define ENABLE_CHALLENGE_CNT		1
#endif

// �����S�إ����O���j�p�g
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

// ���\�䴩�j�Ҥ��ߪ��榡
#ifndef ENABLE_CEEC_FORMAT
#define ENABLE_CEEC_FORMAT		1
#endif

// �j�Ҫ��ǥͧǸ�����
#ifndef STUDENT_CEEC_SN_LENGTH
#define STUDENT_CEEC_SN_LENGTH		6
#endif

// ���w�j�ҥH�Ǹ� merge ���Z��
#ifndef ENABLE_CEEC_MERGE_SN
#define ENABLE_CEEC_MERGE_SN		0
#endif

#ifndef TRY_CEEC_MERGE_SN
#define TRY_CEEC_MERGE_SN		1
#endif


// �䴩�ҥͽƬd���G
#ifndef ENABLE_CHECKLIST
#define ENABLE_CHECKLIST		1
#endif


#ifndef ENABLE_PRE_TESTING
#define ENABLE_PRE_TESTING		0
#endif

// �{���B�z rotation ���覡 ...
// �b 0.1.10 �����e��, �� 0
// ���� 0.1.11 ���H�᪺, ���ճB�z�P���W�B����, �� 1
#ifndef BEST_ADMIRER_ROTATION_VERSION
#define BEST_ADMIRER_ROTATION_VERSION	1
#endif

// ���\�{���䴩���ո�Ʋ���
#ifndef WITH_RANDOM_DATA_GENERATING
#  if defined(__BORLANDC__)
#    define WITH_RANDOM_DATA_GENERATING	0
#  else
#    define WITH_RANDOM_DATA_GENERATING	1
#  endif
#endif

// �{���n�T�w�P�@�ӧ��@����⦸�H�W, (���Ф��B�z) �h�]�� 1
#ifndef NO_MULTIPLE_SELECT_ON_SAME_WISH
#define NO_MULTIPLE_SELECT_ON_SAME_WISH	1
#endif

// �{���O�_�n�䴩 deny list, default �O�_
#ifndef ENABLE_DENYLIST
#define ENABLE_DENYLIST			0
#endif

// �ª��� (0.1.4 ���e) �N��p�� patch
// 0 ���ª�, ���� 0.1.4 ���H�e����k
// 1 ��s��

#ifndef SKILL_COUNT_VERSION
#define SKILL_COUNT_VERSION		1
#endif

// �N���˩w�����ƬO�p��Ӫ� ...
// �b�ª��� (0.1.5 ���e) �N���˩w�O������ score �ɤW�w�g��n��
// ���Z (�����ª����n�]�� 0)

#ifndef SKILL_CHECK_BY_CALCULATE
#define SKILL_CHECK_BY_CALCULATE	1
#endif


#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL	1
#endif

// �ϥΥ[���᪺��l�`�����˩w
#ifndef USE_ADDED_VALUE_AS_CHECK
#define USE_ADDED_VALUE_AS_CHECK	1
#endif

// �N��̾Ǭ�[�v��, �A�p��Ǭ�N���
// �b 90 �~�� SKILL_WEIGHT_ON_MAIN_WEIGHT == 0
//    91 �~��²���]�O�n�]�� 0
//    ���i��|�令�]�� 1 �~����X�z
//
// �� 0.1.16 ������, ���n�]�� 1, �ӹ�ڭp���k�b run-time ��
// �� config file �� skill-relative-weight = on/off �ӨM�w
#ifndef SKILL_WEIGHT_ON_MAIN_WEIGHT
#define SKILL_WEIGHT_ON_MAIN_WEIGHT	1
#endif

// (((�Ǭ�[�v) + (�S�إͥ[��)) * �Ǭ��) + (�N�즨�Z * �Ǭ�[�v * �N���)
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
