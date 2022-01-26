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

// �̦h�C�ӦP���Ѱu
#define MAXIMUM_ON_SAME_REFERENCE	7

struct student_t;
struct student_flag_t;
struct dispatcher_unit_t;
struct skillset_t;

typedef struct student_t *student_ptr_t;

#if ENABLE_DEPARTMENT_FIRST == 1
#define SWF_INVALID		1	/* �w�g���N�� */
#define SWF_ADMIRER_CHECK	2	/* �i�J Admirer Check �� */
#endif

#define BARF_CHECK_INITIALIZED	1	/* ��]��, �O�_�w�g initialized */
#define BARF_INVALID		2	/* ���i��i�J loop �� */
#define BARF_IN_LOOP		4	/* �i�J BAR loop �� */
#define BARF_LOOP_DETECT	8	/* ������b loop �� */

struct student_on_wish_t {
	union {
		struct student_t	*st;
		int			id;
	};
	int16_t			order;	// �b�Ǩt�W���W����
	int32_t			score;
	int32_t			onsame[MAXIMUM_ON_SAME_REFERENCE];
#if ENABLE_DISPATCHER92 == 1
	int32_t			skill_score;
#endif
#if ENABLE_DEPARTMENT_FIRST == 1
	int8_t			aflag;	//
	int16_t			wish_order;	// �q 1 �_��
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
	int16_t		index;		// ���� - �b list ����
#endif
	int32_t		id;		// ���@��
	int16_t		num;		// �W�B
	int8_t		sex;		// �ʧO���� 0, 1, 2
	int8_t		take_sort;	// �קO: ��(5)�B�A(6)�B��(1-4)
	int8_t		cstgrp;		// �ҡB�A�B�� (0 - 2)
	int8_t		check1;		// �Ǵ��@���˩w
	int8_t		test1[5];	// �Ǵ����쪺�з�:�L�B���B�e�B���B��B��
	int8_t		check2;		// ���w��ئҸժ��@���˩w
	int8_t		checkskill;	// �N���˩w
	int8_t		test2[9];	// �E�쪺�з�:���B���B�C
	int8_t		weight[9];	// �E�쪺�ĭp
	int8_t		skillweight;	// �N��ĭp
	int8_t		skillgroup;	// �N��էO
	int8_t		skillway;	// �N�즨�Z�p��覡:���[�v�B�T�w��
	int8_t		skill_main;	// �N��Ǭ�� - �Ǭ�
	int8_t		skill_skill;	// �N��Ǭ�� - �N��
	int16_t		skill_major;	// �N��D�פ��ƭ���
	int8_t		special;	// 1-> �@��ըt, 2-> �꨾ 3-> �v�d
	int8_t		rflag;		// rotation flag
					// ��]�άݬݬO���O initialized
	struct skillset_t
			*skillset;	// �N��D�ƭ׬�ح���
	int8_t		onsame[MAXIMUM_ON_SAME_REFERENCE]; // �P���Ѱu
	//	�H�U�� Run-time �ɤ~�ϥΪ�
	// int8_t	used_onsame;	// �ϥΨ쪺�P���Ѱu
	int8_t		max_onsame_use;	// �̰����P���Ѱu���
	int32_t		min_onsame[MAXIMUM_ON_SAME_REFERENCE];
					// �̤p���P���Ѱu ... ��]��
	int32_t		fail_onsame_sid;

#if ENABLE_DISPATCHER92 == 1
	int8_t		skill_lim[2][2][5];
	int8_t		skill_zero;
#endif
	int32_t		max_orgsum;	// ���t�̰�����l����
	int32_t		avg_orgsum_n;	// ���t������l���Ƥ�����
	int32_t		min_orgsum;	// ���t�̧C����l����
	int32_t		min_orgsum_n;	// ���t�̧C����l���� - �D�S�إ�
	int8_t		min_gradesum;	// ���t�̧C���`�Ť�
	double		variation;

	int16_t		number_of_special;	// �S�إͼ�
	int16_t		deplim_grp;		// ��
	int8_t		number_of_refcrs;	// �ĭp��ؼ� (�Ǭ�)
#if ENABLE_PTHREAD == 1
	pthread_mutex_t	mutex;
#endif
#if SKILL_WEIGHT_ON_MAIN_WEIGHT == 1
	double		ccnt;		// �ĭp���[�v��
#else
	int8_t		ccnt;		// �ĭp����ؼ�
#endif
	int8_t		crsnum_cnt;	// �ĭp����ؼ�
	int16_t		inlistcnt;	// �ثe�����H��
	int16_t		lastidx;	// lastidx on stdlist
	int16_t		llindex;	// lastlist index (�� lastlist ��)
					//   ���ѩ�b�Ǩt�]�L��, �ҥH�b�Ǩt
					//   �]��, �ΨӰO���N�Q�������̫�@
					//   �W������

	union {
		int32_t	minreq;		// �̧C�������� [�ǥͺ]]
		int	fix_require;	// �O�_�ݭn���� [�Ǩt�]]
	};

#if ENABLE_DEPARTMENT_FIRST == 1
	//int16_t		rotation;	// �O�_�i��ѻP rotation ?
	int16_t		real_num;	// �u��W�B
					// [for �Ǩt�]] ��}�l��, ���N���B
					//   �X�W, �o���ܼƥΨӰO����l���B
					//   ����ڹB�ή�, �ΨӰO�������W�B
	int16_t		student_cnt;	// �h�־ǥͦ��񦹧��@
					// �Φb�Ǩt�]�W
#endif
	int16_t		reject_cnt;	// �����
	int16_t		n_qualify_cnt;	// �˩w�� qualify
	struct student_list_t		*lastlist;	// �̫�@�W�S�P����
							// �b�Ǩt�]�W�O�L�Ϊ�
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
