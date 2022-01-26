/*
 *	rotation_v3.c
 *
 *	Copyright (c) 2002, Jiann-Ching Liu
 */

#include <sys/types.h>
#include "predefine.h"
#include "department.h"
#include "student.h"
#include "rotation.h"
#include "misclib.h"
#include "global_var.h"

#if ENABLE_DEPARTMENT_FIRST == 1

//	rotation version 3
//
//	這個演算法是尚不知道沒有一個很好的 universal algorithm
//	可以處理所有這類增額 rotation 的問題
//
//	當然, 這個演算法並不完整, 也就是說, 她並不能解決所有問題
//	但至少可以解決部份的問題。
//
//	雖然程式已經開始走向多種演算法去解不同狀況的架構, 但我還
//	是決定把這個演算法放在第二號 ...
//
//	- - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//	這個演算法開始前, 要先計算所有學系增額錄取的人數 (lastin)
//	及 Best Admirer 的人數。
//
//	如果被射中的學系, lastin 將減少, 當減到 1 時, 才可以用
//	best admirer 去射別人, 如果再被射中, 則表示 loop 已經被偵
//	測到了


struct department_flag {
	int8_t			dflag;
	int			rotation;
	int			lastin;
	int			admirer;
	int			start_from;
	int			admirer_idx;
};

struct admirer_chain_t {
	int	depidx;
	int	stdidx;
	int	admirer_flag;
//	int	srcdep;
//	int	srcstd;
};

static struct department_flag			*depflag;
static struct admirer_chain_t			*admirer_chain;
static int					admirer_chain_idx;

#define DFLAG_IN_SINK		 1
#define DFLAG_OVER		 2
#define DFLAG_UNDER		 4
#define	DFLAG_HAVE_DONE		 8
#define DFLAG_NO_BEST_ADMIRER	16
#define DFLAG_INVALID		32
#define DFLAG_IN_LOOP		64

// --------------------------------------------------------------------

static struct department_t			*depbuf;
static int					depnum;
static struct department_internal_data_t	*didt = NULL;


static int init (struct department_internal_data_t *ptr) {
	if (didt == NULL) {
		didt = ptr;

		depbuf = didt->depbuf;
		depnum = didt->depnum;

		// misc->print (PRINT_LEVEL_INFO,
		//		"Regist rotation algorithm version 3 ... ");

		depflag = misc->malloc (
				depnum * sizeof (struct department_flag));

		if (depflag == NULL) {
			misc->perror (PRINT_LEVEL_INFO);
			return 0;
		}

		admirer_chain =
			misc->malloc (depnum * sizeof (struct admirer_chain_t));
		admirer_chain_idx = 0;

		// misc->print (PRINT_LEVEL_INFO, "ok\r\n");
	}

	return 1;
}

static int prepare (void) {
	int	i, j, k, n;


	for (i = 0; i < depnum; i++) {
		depflag[i].dflag       = 0;

		if (depbuf[i].num < depbuf[i].real_num) {
			// 增額錄取
			// depflag[i].dflag |= DFLAG_IN_SINK;
			depflag[i].dflag |= DFLAG_OVER;
			depflag[i].lastin = depbuf[i].real_num -
						depbuf[i].num + 1;
		} else if (depbuf[i].num < depbuf[i].real_num) {
			// 不足額錄取
			depflag[i].dflag |= DFLAG_IN_SINK;
			depflag[i].dflag |= DFLAG_UNDER;
			depflag[i].lastin = 0;
		} else {
			depflag[i].lastin = 1;
		}

		if ((j = didt->first_admirer (i, -1, NULL)) < 0) {
			depflag[i].admirer = 0;
			depflag[i].admirer_idx = -1;
		} else {
			depflag[i].admirer_idx = j;

			for (n = 1; (k = didt->next_admirer (i, j, NULL)) != -1;
								j = k) {
				n++;
			}

			depflag[i].admirer = n;
		}
	}

	return 1;
}

static int add_to_admirer_loop (const int i, const int j, const int x) {
	if ((depbuf[i].stdlist[j].st->sflag & STUDENT_FLAG_ADMIRER) == 0) {
		// 該生未曾進入 Admirer loop 中
#if DEBUG_LEVEL > 0
		if (trace_sid == depbuf[i].stdlist[j].st->sid) {
			dprint ("Student: %d on " WISHID_FMT
					", Into Admire (order=%d)[i=%d,j=%d]\n",
					trace_sid,
					depbuf[i].id,
					depbuf[i].stdlist[j].wish_order,
					i, j);
		}
#endif
		depbuf[i].stdlist[j].st->sflag |= STUDENT_FLAG_ADMIRER;
		depbuf[i].stdlist[j].st->admire_order =
						depbuf[i].stdlist[j].wish_order;

		admirer_chain[admirer_chain_idx].depidx       = i;
		admirer_chain[admirer_chain_idx].stdidx       = j;
		admirer_chain[admirer_chain_idx].admirer_flag = 0;
		admirer_chain_idx++;
		return 1;
	} else {
#if DEBUG_LEVEL > 0
		if (trace_sid == depbuf[i].stdlist[j].st->sid) {
			dprint ("Student: %d on " WISHID_FMT
					", Already in Admire",
					trace_sid,
					depbuf[i].id);
		}
#endif
		if (depbuf[i].stdlist[j].st->admire_order <
					     depbuf[i].stdlist[j].wish_order) {
			// 原先在 Admirer loop 中的, 有較好
			// 的志願 找下個 admirer 來補
#if DEBUG_LEVEL > 0
			if (trace_sid == depbuf[i].stdlist[j].st->sid) {
				dprint ("(%d < %d)\n",
					depbuf[i].stdlist[j].st->admire_order,
					depbuf[i].stdlist[j].wish_order);
			}
#endif
			if (x) depflag[i].admirer_idx = j + 1;
			return -1;
		} else {
			// 放棄這條線 ...
			// return 0;
#if DEBUG_LEVEL > 0
			if (trace_sid == depbuf[i].stdlist[j].st->sid) {
				dprint ("(%d > %d) give up\n",
					depbuf[i].stdlist[j].st->admire_order,
					depbuf[i].stdlist[j].wish_order);
			}
#endif
			return 0;
		}
	}
}

static void to_clean_admirer_chain (const int start) {
	int	i, j, n;

	if (admirer_chain_idx == 0) return;

	for (n = start; n < admirer_chain_idx; n++) {
		i = admirer_chain[n].depidx;
		j = admirer_chain[n].stdidx;

		depbuf[i].stdlist[j].st->sflag &= (~STUDENT_FLAG_ADMIRER);
		depflag[i].dflag &= (~DFLAG_IN_LOOP);

		// 順便清一些 rotation 的狀態

		depflag[i].admirer_idx = -1;

		if (depflag[i].admirer > 0) {
			depflag[i].rotation = depflag[i].lastin;
		} else {
			depflag[i].rotation = -1;
			depflag[i].dflag |= DFLAG_INVALID;
		}
	}

	admirer_chain_idx = start;

}

static int find_a_loop (const int i, const int level, int *rescue) {
	static int	wanted = 0;
	int		j, k, x, y;
	int		retval;
	int		maybe;
#if DEBUG_LEVEL > 0
	int		debug_on;

	debug_on = 0;

	if (depbuf[i].id == trace_wishid) debug_on = 1;
#endif

#if DEBUG_LEVEL > 0
	if (debug_on) {
		misc->print (PRINT_LEVEL_DEBUG,
			"[%d] Department: " WISHID_FMT
					"(%d), lastin=%d. admirer=%d",
			level, depbuf[i].id, i, depflag[i].lastin,
			depflag[i].admirer);
	}
#endif

	if ((depflag[i].dflag & DFLAG_INVALID) != 0) {
#if DEBUG_LEVEL > 0
		if (debug_on) {
			misc->print (PRINT_LEVEL_DEBUG, "[Already invalid]\n");
		}
#endif
		return -1;
	}

	if (level == 0) {
		wanted = i;
#if DEBUG_LEVEL > 0
		if (debug_on) {
			misc->print (PRINT_LEVEL_DEBUG, "[first]\n");
		}
#endif
	} else {
		if (wanted == i) {
			if (depflag[i].rotation > 1) {
#if DEBUG_LEVEL > 0
				if (debug_on) {
					misc->print (PRINT_LEVEL_DEBUG,
						"[hint=%d]\r\n",
						depflag[i].rotation);
				}
#endif
				depflag[i].rotation--;
				return 0;
			}

#if DEBUG_LEVEL > 0
			if (debug_on) {
				misc->print (PRINT_LEVEL_DEBUG, "[got it]\n");
			}
#endif

			return 1;
		}

		if (depflag[i].lastin > 1) {
			// 暫時不處理這種問題啦 ...
#if DEBUG_LEVEL > 0
			if (debug_on) {
				misc->print (PRINT_LEVEL_DEBUG, "[drop it]\n");
			}
#endif

			return 0;
		}

		if (depflag[i].rotation > 1) {
#if DEBUG_LEVEL > 0
			if (debug_on) {
				misc->print (PRINT_LEVEL_DEBUG, "(over = %d)\n",
					depflag[i].rotation);
			}
#endif
			depflag[i].rotation--;

			return 0;
#if DEBUG_LEVEL > 0
		} else {
			if (debug_on) misc->print (PRINT_LEVEL_DEBUG, "(ok)\n");
#endif
		}
	}

	if ((depflag[i].dflag & DFLAG_IN_LOOP) != 0) {
#if DEBUG_LEVEL > 0
		if (debug_on) {
			misc->print (PRINT_LEVEL_DEBUG,
				"[%d] Match point:" WISHID_FMT " (retval=%d)\n",
				level, depbuf[i].id, depflag[i].start_from);
		}
#endif

		j = didt->first_admirer (i, -1, NULL);

#if DEBUG_LEVEL > 0
		if (debug_on) {
			misc->print (PRINT_LEVEL_DEBUG,
				"[%d] Invalid rest [" WISHID_FMT "] - %d",
				level, depbuf[i].id,
				depbuf[i].stdlist[j].st->sid);
		}
#endif
		didt->invalid_rest_wishes (&depbuf[i].stdlist[j]);

		while ((j = didt->next_admirer (i, j, NULL)) != -1) {
			didt->invalid_rest_wishes (&depbuf[i].stdlist[j]);

			/*
			x = depbuf[i].stdlist[j].st->matched_wish - 1;
			y = depbuf[i].stdlist[j].st->wishidx[x];
			depbuf[y].rflag |= BARF_LOOP_DETECT;
			*/
#if DEBUG_LEVEL > 0
			if (debug_on) {
				misc->print (PRINT_LEVEL_DEBUG,
					", %d", depbuf[i].stdlist[j].st->sid);
			}
#endif
		}
#if DEBUG_LEVEL > 0
		if (debug_on) misc->print (PRINT_LEVEL_DEBUG, "\n");
#endif
		return depflag[i].start_from;
	}

	depflag[i].dflag |= DFLAG_IN_LOOP;

	while (1) {
		depflag[i].admirer_idx = j
			= didt->first_admirer (i, depflag[i].admirer_idx, NULL);

		if (j == -1) return -1; // 沒有 BEST Admirer

		if ((k = add_to_admirer_loop (i, j, 1)) == 0) {
			return -1;
		} else if (k > 0) {
			depflag[i].start_from = admirer_chain_idx;
			break;
		}
	}

	// j = depbuf[i].admirer_idx;
	x = depbuf[i].stdlist[j].st->matched_wish - 1;
	y = depbuf[i].stdlist[j].st->wishidx[x];

#if DEBUG_LEVEL > 0
	//      dprint ("Admire = " WISHID_FMT "\r\n", depbuf[y].id);
	if (debug_on) {
		misc->print (PRINT_LEVEL_DEBUG,
			"[%d] On " WISHID_FMT ", Admirer = %d ("
			WISHID_FMT ")\n",
			level,
			depbuf[i].id,
			depbuf[i].stdlist[j].st->sid,
			depbuf[y].id);
	}
#endif

	// y = depbuf[i].stdlist[j].st->order_on_wish[x];
	//depbuf[i].admirer_idx       = j;
	//depbuf[i].number_of_admirer = n;

	maybe = -1;

	k = admirer_chain_idx - 1; // save

	if ((retval = find_a_loop (y, level + 1, rescue)) > 0) {
		return retval;
	} else if (retval == 0) {
		// 也許是個有效的
		admirer_chain[k].admirer_flag = 2;
		maybe = 0;
	} else {
		admirer_chain[k].admirer_flag = 1;
	}

	while ((j = didt->next_admirer (i, j, NULL)) != -1) {
		depflag[i].admirer_idx = j + 1;

		if (add_to_admirer_loop (i, j, 0) <= 0) continue;

		x = depbuf[i].stdlist[j].st->matched_wish - 1;
		y = depbuf[i].stdlist[j].st->wishidx[x];

		k = admirer_chain_idx - 1; // save

		if ((retval = find_a_loop (y, level + 1, rescue)) > 0) {
			return retval;
		} else if (retval == 0) {
			admirer_chain[k].admirer_flag = 2;
			maybe = 0;
		} else {
			admirer_chain[k].admirer_flag = 1;
		}
	}

	return maybe;
}

static int rotation_admirer_chain (const int from) {
	int	i = 0, j, n;
	int	cnt = 0;

	if (from == 0) return 0;
	if (admirer_chain_idx == 0) return 0;

	for (n = from - 1; n < admirer_chain_idx; n++) {
		i = admirer_chain[n].depidx;
		j = admirer_chain[n].stdidx;

		if (admirer_chain[n].admirer_flag == 0) {
			didt->invalid_rest_wishes (&depbuf[i].stdlist[j]);
			cnt++;
		}
	}

	if (cnt > 0) {
		i = 0; while (didt->do_require_fix ()) i++;
#if DEBUG_LEVEL > 9
		dprint ("[%d] %d walking\r\n", cnt, i);
#endif
	}

#if DEBUG_LEVEL > 2
	{
		int	k, x;
		dprint ("\n");
		for (n = 0; n < admirer_chain_idx; n++) {
			i = admirer_chain[n].depidx;
			j = admirer_chain[n].stdidx;

			x = depbuf[i].stdlist[j].st->matched_wish - 1;
			//y = depbuf[i].stdlist[j].st->order_on_wish[x];
			k = depbuf[i].stdlist[j].st->wishidx[x];

			if (admirer_chain[n].admirer_flag != 0) {
				dprint ("*");
			} else {
				dprint (" ");
			}

			if (n >= (from - 1)) {
				dprint ("+ ");
			} else {
				dprint ("- ");
			}

			dprint (WISHID_FMT ": %d (on " WISHID_FMT ")\n",
					depbuf[i].id,
					depbuf[i].stdlist[j].st->sid,
					depbuf[k].id);
		}
	}
#endif

	if ((cnt > 0) && (i == 0)) {
#if DEBUG_LEVEL > 0
		int	k, x;

		dprint ("error\n\n");

		for (n = 0; n < admirer_chain_idx; n++) {
			i = admirer_chain[n].depidx;
			j = admirer_chain[n].stdidx;

			x = depbuf[i].stdlist[j].st->matched_wish - 1;
			//y = depbuf[i].stdlist[j].st->order_on_wish[x];
			k = depbuf[i].stdlist[j].st->wishidx[x];

			if (admirer_chain[n].admirer_flag != 0) {
				dprint ("*");
			} else {
				dprint (" ");
			}

			if (n >= (from - 1)) {
				dprint ("+ ");
			} else {
				dprint ("- ");
			}

			dprint (WISHID_FMT ": %d (on " WISHID_FMT ")\n",
					depbuf[i].id,
					depbuf[i].stdlist[j].st->sid,
					depbuf[k].id);
		}
#endif
		return -1;
	}

	return cnt;
}

static void set_all_department_rotation_flag (void) {
	int	i;

	for (i = 0; i < depnum; i++) {
		// depflag[i].dflag = 0;
		depflag[i].admirer_idx = -1;

		if (depflag[i].admirer > 0) {
			depflag[i].rotation = depflag[i].lastin;
		} else {
			depflag[i].rotation = -1;
			depflag[i].dflag |= DFLAG_INVALID;
		}
	}
}

static int try_rotation (int *rotation, int *shift, int *rescue) {
	int	i, j, k, idx;
	int	have_best_admirer;

	*rotation = 0;
	*shift    = 0;
	*rescue   = 0;

#if 0
	for (i = 0; i < depnum; i++) {
		if ((depflag[i].dflag & DFLAG_IN_SINK) == 0) {
			depflag[i].dflag = 0;
		}
	}
#endif
	set_all_department_rotation_flag ();
	to_clean_admirer_chain (0);
	idx = 0;

	for (i = j = have_best_admirer = 0; i < depnum; i++) {
		// if (depflag[i].admirer <= 1) continue;
		if (depflag[i].lastin <= 1) continue;

		// 這個演算法只針對已經增額錄取的學系,
		// 如果已經增額錄取, 則嘗試看看會不會有射回兩次以上 ...

		if ((depflag[i].dflag & DFLAG_IN_SINK) != 0) continue;
		if ((depflag[i].dflag & DFLAG_HAVE_DONE) != 0) continue;
		if ((depflag[i].dflag & DFLAG_INVALID) != 0) continue;

#if DEBUG_LEVEL > 1
		if (trace_wishid == depbuf[i].id) {
			misc->print (PRINT_LEVEL_DEBUG,
				"Start from " WISHID_FMT "(%d)\r\n",
				depbuf[i].id, i);
		}
#endif

		if ((j = find_a_loop (i, 0, rescue)) > 0) {
			// dprint ("got it (%d)\r\n", j);

			if ((k = rotation_admirer_chain (j)) < 0) {
				have_best_admirer = 0;
			} else {
				(*rotation)++;
				(*shift) += k;
			}
			break;
		} else if (j == 0) {
			// idx = admirer_chain_idx;
			idx = 0;
#if DEBUG_LEVEL > 1
			// dprint ("not enought (%d)\r\n", i);
#endif
			// 射中的次數不足
			to_clean_admirer_chain (idx);
		} else {
			// set_all_department_rotation_flag ();
			// 完全沒有射回來 ...
			to_clean_admirer_chain (idx);
		}

		// if (have_best_admirer) break;
	}

#if DEBUG_LEVEL > 1
	if (*rotation > 0) {
		misc->print (PRINT_LEVEL_DEBUG,
		    "Rotation version 3: %d rotate, %d shift, %d rescue\r\n",
		    *rotation, *shift, *rescue);
	}
#endif

	return *rotation;
}

void regist_rotation_algorithm_version_3 (void) {
	static struct rotation_algorithm_t	rtam;

	rtam.init         = init;
	rtam.try_rotation = try_rotation;
	rtam.prepare	  = prepare;

	regist_rotation_algorithm (&rtam);
}

#endif
