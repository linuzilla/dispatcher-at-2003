/*
 *	rotation_v5.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#include "predefine.h"
#include "department.h"
#include "rotation.h"
#include "misclib.h"
#include "global_var.h"


#if ENABLE_DEPARTMENT_FIRST == 1

//	rotation version 5
//

static struct department_t			*depbuf;
static int					depnum;
static struct department_internal_data_t	*didt = NULL;

static int init (struct department_internal_data_t *ptr) {
	didt = ptr;

	depbuf = didt->depbuf;
	depnum = didt->depnum;

	/*
	misc->print (PRINT_LEVEL_INFO,
			"Regist rotation algorithm version 5\r\n");
	*/

	return 1;
}

static int prepare (void) { return 1; }

static int try_rotation (int *ro, int *sf, int *rc) {
	//misc->print (PRINT_LEVEL_INFO, "try rotation version 2\r\n");
	return 0;
}

void regist_rotation_algorithm_version_5 (void) {
	static struct rotation_algorithm_t	rtam;

	rtam.init         = init;
	rtam.try_rotation = try_rotation;
	rtam.prepare	  = prepare;

	regist_rotation_algorithm (&rtam);
}

#endif
