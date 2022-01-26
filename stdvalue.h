/*
 *	stdvalue.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __STANDARD_VALUES_H_
#define __STANDARD_VALUES_H_

#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct standard_values_t {
	int16_t		gradesum;		// 總級分
	int16_t		grademin[5];		// 五科的級分
						// 0  1  2  3  4
						// 國 英 數 社 自

	int16_t		grade[5][5];		// 五科的五種標
						// 0   1   2   3   4
						// 頂、前、均、後、底

	int16_t		check[4];		// 四種指定科目一般檢定
	int32_t		checkskill[9];		// 七種術科一般檢定
						// 0  1  2  3  4  5  6  7
						// 音 國 體 舞 美 國 戲 美
						// 樂 樂 育 蹈 術 劇 劇 術
						//			二

	int16_t		test[9][3];		// 九種指定科目考試的高均低標
						// 國、英、數甲、數乙、
						// 歷、地、物、化、生
};

struct standard_values_t * initial_standard_values (void);

#if defined(__cplusplus)
}
#endif

#endif
