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
	char	ptype[1];	// 身份類別
	char	uclass[4];	// 聯招類組
	char	pid[12];	// 身份證號
	char	birth[9];	// 000-00-00
	char	tel[20];
	char	cell_phone[20];
	char	zip[3];
	char	addr[100];
	char	result[1];	// 審查結果
};

/*
 * 身份類別:
 * 	1. 普通生
 * 	2. 僑生
 * 	3. 退伍軍人
 * 	4. 增加總分─運動績優生 10%
 * 	5. 
 * 	A. 降低錄取標準─退伍軍人 10%
 * 	B. 降低錄取標準─退伍軍人 25%
 * 	C. 增加總分─退伍軍人 8%
 * 	D. 增加總分─退伍軍人 25%
 *
 * 審查結果:
 * 	0. 無
 * 	1. 大學修業其間，有不及格科目
 * 	2. 大學一年級任一學期不及格科目學分數達該學期修習學分數三分之一者
 * 	3. 兼有 1, 2 者
 */

struct usot_course_map_t {
	char	crsid[USOT_CRSID_LENGTH];
	char	crsname[USOT_CRSNAME_LENGTH];
	char	crsgrp[1];	// 1. 初試(共) 2. 初試(必) 3. 初試(選), 4. 複試
};

struct usot_depcrs_t {		// 系組選定考科代碼檔 (skillset)
	char	depid[USOT_DEPID_LENGTH];
	char	limsn[2];	// 條件組序, 由 0 開始
	char	limcnt[2];	// 條件組數
	char	excrs[MAX_USOT_EXAM_COURSE][2];
	//char	excrs1[2];	// N1 考試科目一之科目數
	//char	excrs2[2];	// N2         二
	//char	excrs3[2];	// N3         三
	//char	excrs4[2];	// N4         四
	//char	excrs5[2];	// N5         五
	char	exid[4];	// C(04) x (Sum over Ni)
};

struct usot_deplim_t {
	char	depid[USOT_DEPID_LENGTH];
	char	num[3];
	char	gender[1];	// 0: No, 1: Male, 2: Female
	char	exam[MAX_USOT_EXAM_COURSE][6];
				// 採計 999v999, 不採計 ------
	char	onsame[MAX_USOT_EXAM_COURSE];
				// 同分參酌
	char	lowsco[MAX_USOT_EXAM_COURSE][5];
				// 最低分下限: 999v99 -----: 不設限
	char	lowws[6];	// 最低加權總分 9999v99, -------: 不設限
	char	res[3];		// 備取名額
	char	lim[1];		// 分發資格限制 (對應: basic 的審查結果)
};

struct usot_depna_t {
	char	school[USOT_SCHNAME_LENGTH];
	char	depname[USOT_DEPNAME_LENGTH];
	char	uclass[4];	// 聯招類組
	char	age[1];		// 年級
	char	depid[USOT_DEPID_LENGTH];	// 校系(志願)代碼
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
	char	ispass[1];	// 1-> 合格, 2-> 不合格
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
