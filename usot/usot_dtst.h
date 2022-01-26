/*
 *	usot/dtstruct.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __USOT_DT_STRUCT_H_
#define __USOT_DT_STRUCT_H_

#if defined(__cplusplus)
extern "C" {
#endif


/////////////////////////////////////////////////////////////////

#define MAX_USOT_WISHES		9
#define USOT_SID_LENGTH		8
#define MAX_USOT_EXAM_COURSE	5
#define USOT_DEPID_LENGTH	4
#define USOT_CRSID_LENGTH	4
#define USOT_CRSNAME_LENGTH	40
#define USOT_SCHNAME_LENGTH	26
#define USOT_DEPNAME_LENGTH	44

struct usot_basic_t {
	char	sid[USOT_SID_LENGTH];
	char	cname[12];
	char	gender[1];	// M/F
	char	ptype[1];	// �������O
	char	uclass[4];	// �p������
	char	pid[12];	// �����Ҹ�
	char	birth[9];	// 000-00-00
	char	tel[20];
	char	cell_phone[20];
	char	zip[3];
	char	addr[100];
	char	result[1];	// �f�d���G
};

/*
 * �������O:
 * 	1. ���q��
 * 	2. ����
 * 	3. �h��x�H
 * 	4. �W�[�`���w�B���Z�u�� 10%
 * 	5. 
 * 	A. ���C�����зǢw�h��x�H 10%
 * 	B. ���C�����зǢw�h��x�H 25%
 * 	C. �W�[�`���w�h��x�H 8%
 * 	D. �W�[�`���w�h��x�H 25%
 *
 * �f�d���G:
 * 	0. �L
 * 	1. �j�ǭ׷~�䶡�A�����ή���
 * 	2. �j�Ǥ@�~�ť��@�Ǵ����ή��ؾǤ��ƹF�ӾǴ��ײ߾Ǥ��ƤT�����@��
 * 	3. �ݦ� 1, 2 ��
 */

struct usot_course_map_t {
	char	crsid[USOT_CRSID_LENGTH];
	char	crsname[USOT_CRSNAME_LENGTH];
	char	crsgrp[1];	// 1. ���(�@) 2. ���(��) 3. ���(��), 4. �Ƹ�
};

struct usot_depcrs_t {		// �t�տ�w�Ҭ�N�X�� (skillset)
	char	depid[USOT_DEPID_LENGTH];
	char	limsn[2];	// ����է�, �� 0 �}�l
	char	limcnt[2];	// ����ռ�
	char	excrs[MAX_USOT_EXAM_COURSE][2];
	//char	excrs1[2];	// N1 �Ҹլ�ؤ@����ؼ�
	//char	excrs2[2];	// N2         �G
	//char	excrs3[2];	// N3         �T
	//char	excrs4[2];	// N4         �|
	//char	excrs5[2];	// N5         ��
	char	exid[4];	// C(04) x (Sum over Ni)
};

struct usot_deplim_t {
	char	depid[USOT_DEPID_LENGTH];
	char	num[3];
	char	gender[1];	// 0: No, 1: Male, 2: Female
	char	exam[MAX_USOT_EXAM_COURSE][6];
				// �ĭp 999v999, ���ĭp ------
	char	onsame[MAX_USOT_EXAM_COURSE];
				// �P���Ѱu
	char	lowsco[MAX_USOT_EXAM_COURSE][5];
				// �̧C���U��: 999v99 -----: ���]��
	char	lowws[6];	// �̧C�[�v�`�� 9999v99, -------: ���]��
	char	res[3];		// �ƨ��W�B
	char	lim[1];		// ���o��歭�� (����: basic ���f�d���G)
};

struct usot_depna_t {
	char	school[USOT_SCHNAME_LENGTH];
	char	depname[USOT_DEPNAME_LENGTH];
	char	uclass[4];	// �p������
	char	age[1];		// �~��
	char	depid[USOT_DEPID_LENGTH];	// �ըt(���@)�N�X
};

struct usot_wish_t {
	char	sid[USOT_SID_LENGTH];
	char	wishes[MAX_USOT_WISHES][USOT_DEPID_LENGTH];
};

struct usot_score_sub_t {
	char    crs[USOT_CRSID_LENGTH];
	char	src[5];
} __attribute__ ((__packed__));;

struct usot_score_t {
	char			sid[USOT_SID_LENGTH];
	struct usot_score_sub_t	sc[MAX_USOT_EXAM_COURSE];
} __attribute__ ((__packed__));;

struct usot_wishtemp_t {
	char	sid[USOT_SID_LENGTH];
	char	depid[USOT_DEPID_LENGTH];
	char	ispass[1];	// 1-> �X��, 2-> ���X��
	char	score[6];
};

struct usot_wishfinal_t {
	char	sid[USOT_SID_LENGTH];
	char	depid[USOT_DEPID_LENGTH];
	char	score[6];
	char	wisncnt[2];
};

struct usot_wishminimal_t {
	char	depid[USOT_DEPID_LENGTH];
	char	score[6];
	char	onsame[5][5];
	char	onsame_use[1];
};

/////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
}
#endif
#endif
