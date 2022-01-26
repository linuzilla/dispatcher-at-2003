/*
 *	dtstruct.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __DT_STRUCT_H_
#define __DT_STRUCT_H_

#include "predefine.h"

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////

struct wishminimal_t {
	char	wishid[DEPARTMENT_ID_LENGTH];
	char	score[6];
	char	onsame[7][5];
	char	use[1];
};

struct checklist_t {
	char	sn[5];
	char	studid[STUDENT_ID_LENGTH];
	char	description[200];
};

struct ncu_depname_t {
	char	sch_name[26];
	char	dep_name[44];
	char	wishid[DEPARTMENT_ID_LENGTH];
	char	num[3];
	char	schid[3];
	char	skst[4];
};

struct ncu_deplim {
	char	wishid[DEPARTMENT_ID_LENGTH];
	char	num[3];
	char	sex;
	char	grade_check;
	char	grade_std[5];
	char	exam_check;
	char	skill_check;
	char	exam_std[9];
	char	exam_use[9];
	char	skillweight;
	char	skillgroup;
	char	skill_how;
	char	skill_main[2];
	char	skill_skill[2];
	char	onsame[7];
	char	major[4];
	char	special;
};

#if ENABLE_DISPATCHER92 == 1

struct ncu_deplim92 {
	struct ncu_deplim;
	char	sk_ratio_score[2][2][5][3];
	char	skill_zero[1];
};

#endif

struct ncu_skillset {
	char	wishid[DEPARTMENT_ID_LENGTH];
	char	sn[2];
	char	num[2];
	char	major[2];
	char	minor[2];
	char	code[1][4];
};

/////////////////////////////////////////////////////////////////

struct ncu_stdwishes {
	char	studid[STUDENT_ID_LENGTH];
	char	wishes[MAX_PER_STUDENT_WISH][DEPARTMENT_ID_LENGTH];
};

#if ENABLE_DISPATCHER92 == 1
struct ncu_stdwishes92 {
	char	studsid[STUDENT_ID_LENGTH];	// 指考准考證號
	char	studcid[STUDENT_ID_LENGTH];	// 學測准考證號
	char	wishes[MAX_PER_STUDENT_WISH][DEPARTMENT_ID_LENGTH];
};
#endif

struct ncu_stdinfo {
	char	studid[STUDENT_ID_LENGTH];
	char	zone[2];
	char	name[12];
	char	pid[10];
	char	sex;
	char	ptype;
	char	disallow;
	char	reserve[7];
	char	birth[6];
};

struct ncu_score {
	char	studid[STUDENT_ID_LENGTH];
	char	zone[2];
	char	sid_basic[STUDENT_ID_LENGTH];
	char	grade[5][2];
	char	sid_mend[STUDENT_ID_LENGTH];
	char	pass;
	char	sid_exam[STUDENT_ID_LENGTH];
	char	score[9][5];
	char	skillgroup[1];
	char	skillmajor[5];
	char	skillminor[5];
	char	skillmiss;
	char	skillscore[5];
	char	reserve[5];
	char	skillmajor_score[5];
};

/////////////////////////////////////////////////////

struct ceec_stdinfo {
	char	serial[STUDENT_CEEC_SN_LENGTH];
	char	name[12];
	char	sex;
	char	birth[6];
	char	person_id[10];
	char	reserve1[2];	// 畢業年次
	char	edu_level[3];	// 學力代碼
	char	reserve3[3];	// 郵遞區號
	char	reserve4[60];	// 通訊地址
	char	reserve5[74];	// 戶籍地址
	char	reserve6[12];	// 家長或監護人
	char	reserve7[10];	// 電話
	char	skillgroup[1];
	char	skillmajor[4];
	char	skillminor[4];
	char	sid_basic[STUDENT_ID_LENGTH];
	char	sid_mend[STUDENT_ID_LENGTH];
	char	sid_exam[STUDENT_ID_LENGTH];
	char	regist_sn[8];
	char	ptype;
	char	disallow;	// 是否符合檢定
	char	special;	// 1 -> 一般生, 2 -> 國防, 3 ->
};

#if ENABLE_DISPATCHER92 == 1

struct ceec_stdinfo92 {
	char	serial[STUDENT_CEEC_SN_LENGTH];
	char	name[12];
	char	sex;
	char	birth[6];
	char	person_id[10];
	char	edu_year[2];	// 畢業年次
	char	edu_level[3];	// 學力代碼
	char	zip_code[3];		// 郵遞區號
	char	cmaddr[60];	// 通訊地址
	char	address[74];	// 戶籍地址
	char	guard_man[12];	// 家長或監護人
	char	tel_no[10];	// 電話
	char	low_income[1];	// 低收入戶
//	char	skillgroup[1];
	char	skillmajor[4];
	char	skillminor[4];
	char	sid_basic[STUDENT_ID_LENGTH];
	char	sid_mend[STUDENT_ID_LENGTH];
	char	sid_exam[STUDENT_ID_LENGTH];
	char	sid_skill[NUMBER_OF_SKILL92][STUDENT_ID_LENGTH];
	char	regist_sn[8];
	char	ptype;
	char	disallow;	// 是否符合檢定
	char	special;	// 1 -> 一般生, 2 -> 國防, 3 ->
	char	disallow_reason[1];
};

#endif

struct ceec_grade_score {
	char	serial[STUDENT_CEEC_SN_LENGTH];
	char	grade[5][2];
	char	sum[2];
	char	pass;
	char	sid_basic[STUDENT_ID_LENGTH];
	char	sid_mend[STUDENT_ID_LENGTH];
};

struct ceec_exam_score {
	char	serial[STUDENT_CEEC_SN_LENGTH];
	char	score[9][5];
	char	skill[7][5];
	char	major[5];
	char	sid_exam[STUDENT_ID_LENGTH];
};

#if ENABLE_DISPATCHER92 == 1

struct ceec_grade_score92 {
	struct ceec_grade_score;
	char	disallow[1];
};

struct ceec_exam_score92 {
	char	serial[STUDENT_CEEC_SN_LENGTH];
	char	score[9][5];
	char	sid_exam[STUDENT_ID_LENGTH];
	char	disallow[1];
};

struct ceec_skill_score92 {
	char	serial[STUDENT_CEEC_SN_LENGTH];
	char	m_major[4];
	char	m_minor[4];
	char	sk_score[SK92_SUBSCR_SET][SK92_SUBSCR_NUM][5];
	char	p_score[5];
	char	p_grade[2];
	char	skill_sid[NUMBER_OF_SKILL92][STUDENT_ID_LENGTH];
	char	disallow[1];
};

#if ENABLE_EXTEND_DISPATCHER92 == 1


struct ceec_extend_skill_score92 {
	struct ceec_skill_score92;
	char	sid_exam[STUDENT_ID_LENGTH];
	char	skill_group[1];
	char	skill_score[5];
};
#endif

#endif


#if defined(__cplusplus)
}
#endif
#endif
