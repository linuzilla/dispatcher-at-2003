#ifndef __GLOBAL_VARIABLES_H__
#define __GLOBAL_VARIABLES_H_

#include "predefine.h"
#include <sys/types.h>
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct sysconf_t;
struct misc_libraries_t;
struct postcheck_t;

extern struct sysconf_t 	*sysconfig;
extern struct misc_libraries_t	*misc;
extern struct postcheck_t	*pstck;
extern int			debug_flag;
extern int			verbose;
extern int			use_disp92;
extern int			selected_algorithm;
extern int			trace_sid;
extern int			trace_wishid;
extern int			skill_relative_weight;
extern int			weighted_skill_standard;
extern int			include_student_extend_info;
extern int			use_ncu_format;
extern int			disallow_same_instrument;
extern int			grade_on_skill_physical;
extern int			check_for_duplicate;
extern struct standard_values_t	*standval;

extern char			**dpg_basic_crs;
extern char			**dpg_exam_crs;
extern char			**dpg_onsame_crs;

extern int			logwish;
extern FILE			*logwishfp;
extern FILE			*cklfp;
extern int			no_wish_order;
extern int			highsch_only;

extern short			have_level_2_info;
#if ENABLE_FULL_FAIL_RESULT_CHECK == 1
extern short			check_all_rules;
#endif

#if ENABLE_INCREMENTAL_ALGORITHM == 1
extern short			getstdid_only;
#endif

extern short			myschool_id;
extern int			x_option;
extern short			dispatching_finalized;

#if DULL_ELEPHANT_OPTION == 1
extern short			de_is_skip_first_line;
extern short			de_wish_use_oct;
#endif

#if ENABLE_DR_LEE_SPECIAL == 1
extern short	dr_lee_optionflag;
extern int	dr_lee_rjstcnt;
extern int32_t	dr_lee_studid;
extern int16_t	dr_lee_wishid;
#endif

extern void print_memory (void);
extern void initial_global_variables (void);

#if defined(__cplusplus)
}
#endif

#endif
