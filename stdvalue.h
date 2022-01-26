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
	int16_t		gradesum;		// �`�Ť�
	int16_t		grademin[5];		// ���쪺�Ť�
						// 0  1  2  3  4
						// �� �^ �� �� ��

	int16_t		grade[5][5];		// ���쪺���ؼ�
						// 0   1   2   3   4
						// ���B�e�B���B��B��

	int16_t		check[4];		// �|�ث��w��ؤ@���˩w
	int32_t		checkskill[9];		// �C�سN��@���˩w
						// 0  1  2  3  4  5  6  7
						// �� �� �� �R �� �� �� ��
						// �� �� �| �� �N �@ �@ �N
						//			�G

	int16_t		test[9][3];		// �E�ث��w��ئҸժ������C��
						// ��B�^�B�ƥҡB�ƤA�B
						// ���B�a�B���B�ơB��
};

struct standard_values_t * initial_standard_values (void);

#if defined(__cplusplus)
}
#endif

#endif
