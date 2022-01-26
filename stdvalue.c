/*
 *	stdvalue.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include "predefine.h"
#include "stdvalue.h"
#include "global_var.h"
#include "sys_conf.h"
#include "textio.h"
#include "misclib.h"

#define STANDARD_FILE_RECORD_LEN	252

struct standard_values_t * initial_standard_values (void) {
	static struct standard_values_t	value;
	char				*filename;
	struct textfile_io_t		*tio = NULL; 
	int				len;
	int				i, j, k;
	char				*buffer;

	if ((filename = sysconfig->getstr ("standard-file")) == NULL) {
		misc->print (PRINT_LEVEL_SYSTEM,
                    "%% system variable (standard-file) not define!!\n");

		return NULL;
	} else if (filename[0] == '\0') {
		return NULL;
	}

	tio = initial_textfile_io_module ();

	misc->print (PRINT_LEVEL_SYSTEM, "Load standard file [ %s ] ... ",
			filename);

	if (tio->open (filename)) {
		len = tio->read (&buffer);
		tio->close ();
		if (len < STANDARD_FILE_RECORD_LEN) {
			misc->print (PRINT_LEVEL_SYSTEM, "error %d\r\n", len);
			return NULL;
		}
	} else {
		misc->perror (PRINT_LEVEL_SYSTEM);
		return NULL;
	}
	misc->print (PRINT_LEVEL_SYSTEM, "ok\r\n");

	k = 0;
	value.gradesum = misc->dec (&buffer[k], 2);
	k += 2;

	for (i = 0; i < 5; i++) {
		value.grademin[i] = misc->dec (&buffer[k], 2);
		k+= 2;
	}

	for (i = 0; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			value.grade[i][j] = misc->dec (&buffer[k], 2);
			k+= 2;
		}
	}

	for (i = 0; i < 4; i++) {
		value.check[i] = misc->score_conv (&buffer[k]);
		k += 5;
	}

	for (i = 1; i < 8; i++) {
		value.checkskill[i] = misc->score_conv (&buffer[k]);
		k += 5;
	}

#if SKILL_COUNT_VERSION == 0
	// 因為美術二比例不同, 但檢定相同, so ...
	value.checkskill[8] = value.checkskill[5];
#endif

	for (i = 0; i < 9; i++) {
		for (j = 0; j < 3; j++) {
			value.test[i][j] = misc->score_conv (&buffer[k]);
			k += 5;
		}
	}

#if ENABLE_DISPATCHER92 == 1
	if (use_disp92) {	// 將美術檢定的分數調整到定位
		i = value.checkskill[2];
		value.checkskill[2] = value.checkskill[5];
		value.checkskill[5] = i;
	}
#endif

	return &value;
}
