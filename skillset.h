/*
 *	skillset.h
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#ifndef __SKILLSET_H_
#define __SKILLSET_H_

#include <sys/types.h>
#include "predefine.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct skillset_t {
	int64_t			major;
	int64_t			minor;
	struct skillset_t	*next;
};

#if defined(__cplusplus)
}
#endif
#endif
