/*
 *	rotation.c
 *
 *	Copyright (c) 2001 Jiann-Ching Liu
 */

#if ENABLE_DEPARTMENT_FIRST == 1

static void write_matched_wish (void) {
	int	i, j;

	for (i = 0; i < depnum; i++) {
		for (j = 0; j < depbuf[i].llindex; j++) {
			if ((depbuf[i].stdlist[j].aflag & SWF_INVALID) == 0) {
				depbuf[i].stdlist[j].st->matched_wish =
					depbuf[i].stdlist[j].wish_order;
				/*
				depbuf[i].stdlist[j].st->matched_wish = i;
				*/
#if DEBUG_LEVEL > 0
				if (depbuf[i].stdlist[j].st->sid ==
						trace_sid) {
					dprint ("%d Matched on " WISHID_FMT
						" (%d)\r\n",
						trace_sid, depbuf[i].id,
						depbuf[i].stdlist[j].st->
							matched_wish);

				}
#endif
			}
		}
	}
}

static struct admirer_chain_t	*admirer_chain;
static int			admirer_chain_idx;

#if BEST_ADMIRER_ROTATION_VERSION > 0
static int first_admirer (const int i, const int from) {
	int	found;
	int	j, k = 0, retval = -1;

	if ((j = from) == -1) j = depbuf[i].llindex;

	for (found = 0; j < depbuf[i].inlistcnt; j++) {
		if ((depbuf[i].stdlist[j].aflag & SWF_INVALID) != 0) {
			continue; // 跳過那些已經高就的 ....
		}

		if (found) {
			if (compare_order (&depbuf[i].stdlist[j],
					&depbuf[i].stdlist[k]) != 0) {
				break;
			}
		}

		if (depbuf[i].stdlist[j].st->matched_wish == -1) {
			found = 1;
			k = j;
		} else if (depbuf[i].stdlist[j].wish_order <
					depbuf[i].stdlist[j].st->matched_wish) {
			found = 1;
			retval = j;
			break;
		}
	}

	return retval;
}

#endif

#if BEST_ADMIRER_ROTATION_VERSION > 0

static int next_admirer (const int i, const int j) {
	int	k;

	for (k = j + 1; k < depbuf[i].inlistcnt; k++) {
		if (compare_order (&depbuf[i].stdlist[j],
					&depbuf[i].stdlist[k]) != 0) {
			break;
		}

		if ((depbuf[i].stdlist[k].aflag & SWF_INVALID) != 0) {
			continue; // 跳過那些已經高就的 ....
		}

		if (depbuf[i].stdlist[k].st->matched_wish == -1) {
			continue; // 跳過落榜生 ...
		}

		if (depbuf[i].stdlist[k].wish_order <
				depbuf[i].stdlist[k].st->matched_wish) {
			return k;
		}
	}

	return -1;
}

#endif

#if BEST_ADMIRER_ROTATION_VERSION > 0
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
			if (x) depbuf[i].admirer_idx = j + 1;
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
#endif




#if BEST_ADMIRER_ROTATION_VERSION > 0
static int find_admirer_v2 (const int i, const int level) {
	// static int	first_station = 0;
	int		j = 0, k, x, y;
	int		retval;
#if DEBUG_LEVEL > 0
	int		debug_on;
#endif

	// if (level == 0) first_station = i;

	// rotation 為 -1 時, 表示沒有 Admirer 否則, 則為 lastin 的人數
	// lastin 是固定的, rotation 則當某些條件成立時
	// 可以遞減, 直到 rotation 變成 0 時, 表示這個學系已
	// 經具備 rotation 的條件


	/*
	if (depbuf[i].number_of_admirer == 0) return 0;
	if (depbuf[i].admirer_idx < 0) return 0;
	*/

#if DEBUG_LEVEL > 0
	debug_on = 0;

	// if (depbuf[i].id == 0753) debug_on = 1;
	// if (depbuf[i].id == 0722) debug_on = 1;
	if (depbuf[i].id == trace_wishid) debug_on = 1;

	if (level == -1) debug_on = 1;
#endif


#if DEBUG_LEVEL > 0
	if (debug_on) {
		dprint ("[%d] Department: " WISHID_FMT
				"(%d), Admirer=%d, lastin=%d. ",
			level,
			depbuf[i].id, i,
			depbuf[i].number_of_admirer,
			depbuf[i].number_of_lastin);
	}
#endif

	if ((depbuf[i].rflag & BARF_INVALID) != 0) {
#if DEBUG_LEVEL > 0
		if (debug_on) dprint ("Already invalid\n");
#endif
		return -1;
	}

	if (depbuf[i].rotation > 1) {
		// 溢收
#if DEBUG_LEVEL > 0
		if (debug_on) dprint ("(over)\n");
#endif
		depbuf[i].rotation--;

		return 0;
#if DEBUG_LEVEL > 0
	} else {
		if (debug_on) dprint ("(ok)\n");
#endif
	}

	if ((depbuf[i].rflag & BARF_IN_LOOP) != 0) {
		// if (first_station != i) return 0;

		// 已經參與 rotation ...
		// ok
		// loop found
		// return 1
		// 請注意, 在此時, 程式已經認定這個 loop 已經找到,
		// 所以可以堅信去 invalid_rest_wishes 之後, 一定會
		// 導至 loop 回自己所 admirer 的志願, 如果這個條件
		// 不成立, 會使該生意外落榜

		// invalid_rest_wishes (&depbuf[i].stdlist[j]);

		// ok! 此時此刻 ....
#if DEBUG_LEVEL > 0
		if (debug_on) {
			dprint ("[%d] Match point:" WISHID_FMT " (retval=%d)\n",
				level, depbuf[i].id, depbuf[i].start_from);
		}
#endif

		j = first_admirer (i, -1);
#if DEBUG_LEVEL > 0
		if (debug_on) {
			dprint ("[%d] Invalid rest [" WISHID_FMT "] - %d",
					level,
					depbuf[i].id,
					depbuf[i].stdlist[j].st->sid);
		}
#endif
		invalid_rest_wishes (&depbuf[i].stdlist[j]);

		/*
		x = depbuf[i].stdlist[j].st->matched_wish - 1;
		y = depbuf[i].stdlist[j].st->wishidx[x];
		depbuf[y].rflag |= BARF_LOOP_DETECT;
		*/

		while ((j = next_admirer (i, j)) != -1) {
			invalid_rest_wishes (&depbuf[i].stdlist[j]);

			/*
			x = depbuf[i].stdlist[j].st->matched_wish - 1;
			y = depbuf[i].stdlist[j].st->wishidx[x];
			depbuf[y].rflag |= BARF_LOOP_DETECT;
			*/
#if DEBUG_LEVEL > 0
			if (debug_on) {
				dprint (", %d", depbuf[i].stdlist[j].st->sid);
			}
#endif
		}
#if DEBUG_LEVEL > 0
		if (debug_on) dprint ("\n");
#endif
		return depbuf[i].start_from;
	}

	depbuf[i].rflag |= BARF_IN_LOOP;


	while (1) {
		depbuf[i].admirer_idx = j
				= first_admirer (i, depbuf[i].admirer_idx);

		if (j == -1) return -1; // 沒有 BEST Admirer

		if ((k = add_to_admirer_loop (i, j, 1)) == 0) {
			return -1;
		} else if (k > 0) {
			depbuf[i].start_from = admirer_chain_idx;
			break;
		}
	}


	// j = depbuf[i].admirer_idx;
	x = depbuf[i].stdlist[j].st->matched_wish - 1;
	y = depbuf[i].stdlist[j].st->wishidx[x];

#if DEBUG_LEVEL > 0
//	dprint ("Admire = " WISHID_FMT "\r\n", depbuf[y].id);
	if (debug_on) {
		dprint ("[%d] On " WISHID_FMT ", Admirer = %d ("
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

	k = admirer_chain_idx; // save

	if ((retval = find_admirer_v2 (y, level + 1)) > 0) {
#if DEBUG_LEVEL > 9
		dprint ("Found [" WISHID_FMT "] - %d\n",
				depbuf[i].id, depbuf[i].stdlist[j].st->sid);
#endif
		// invalid_rest_wishes (&depbuf[i].stdlist[j]);
		return retval;
	} else if (retval < 0) {
		// clean_student_status_in_admirer_chain (k - 1);
		admirer_chain[k - 1].admirer_flag = 1;
		// admirer_chain_idx = k; // restore
		// Oops !! 照理說是這樣, 但奇怪的是, 這樣反而不 Work ....
#if DEBUG_LEVEL > 0
		if (debug_on) {
			dprint ("[%d] Cut On " WISHID_FMT ", index = %d\n",
				level, depbuf[i].id, k);
		}
#endif
	}

	// ------------------------------------------------------------

	while ((j = next_admirer (i, j)) != -1) {
		depbuf[i].admirer_idx = j + 1;

		if (add_to_admirer_loop (i, j, 0) <= 0) continue;

		x = depbuf[i].stdlist[j].st->matched_wish - 1;
		y = depbuf[i].stdlist[j].st->wishidx[x];

#if DEBUG_LEVEL > 9
		dprint ("[%d] * On " WISHID_FMT ", Admirer = %d ("
						WISHID_FMT ")\n",
			level,
			depbuf[i].id,
			depbuf[i].stdlist[j].st->sid,
			depbuf[y].id);
#endif

		k = admirer_chain_idx; // save

		if ((retval = find_admirer_v2 (y, level + 1)) > 0) {
#if DEBUG_LEVEL > 9
			dprint ("found [" WISHID_FMT "]\n", depbuf[y].id);
#endif
			// invalid_rest_wishes (&depbuf[i].stdlist[j]);
			return retval;
		} else if (retval < 0) {
			// clean_student_status_in_admirer_chain (k - 1);
			admirer_chain[k - 1].admirer_flag = 1;
			// admirer_chain_idx = k; // restore
		}
	}

#if DEBUG_LEVEL > 9
	dprint ("[%d] Department: " WISHID_FMT " <give up>\n",
			level, depbuf[i].id);
#endif

	// depbuf[i].rflag |= BARF_INVALID;
	return -1;
}
#endif

#if BEST_ADMIRER_ROTATION_VERSION == 0 
static int to_find_admirer (const int i) {
	int	j, k, m, x, y, w;
	int	ii, jj, kk, ww;
	int	found;

	// return value:
	//     -1:	不正常的 loop, 要放棄部份
	//      0:	不可能成為 loop, 放棄
	// 	1:	找到 admirer chain loop
	

	// 要考慮一下那些已經同分增額錄取的校系
	
	if (depbuf[i].num < depbuf[i].real_num) {
#if DEBUG_LEVEL > 1
		dprint ("On Wish " WISHID_FMT " - %d %d %d\r\n",
				depbuf[i].id,
				depbuf[i].real_num,
				depbuf[i].num,
				depbuf[i].inlistcnt);
#endif
		return 0;
	}

	if (depbuf[i].rotation == 0) return 0;

	if (depbuf[i].rotation == 2) {
		// 已經形成 loop
		depbuf[i].rotation = 3; // 註記成 loop 的啟始
#if DEBUG_LEVEL > 1
		dprint ("LOOP detected\r\n");
#endif
		return 1;
	}

	for (j = depbuf[i].llindex; j < depbuf[i].inlistcnt; j++) {
		if ((depbuf[i].stdlist[j].aflag & SWF_INVALID) != 0) {
			continue; // 跳過那些已經高就的 ....
		}

		if (depbuf[i].stdlist[j].st->matched_wish == -1) {
			// 表面上是沒望了, 因為是落榜生
			// 但實際上, 也許這個落榜生與 Best Admier 是完
			// 全同分 .... 所以還要再檢查看看 ...
			
			found = 0;

			for (m = j + 1; m < depbuf[i].inlistcnt; m++) {
				if (compare_order (&depbuf[i].stdlist[j],
						&depbuf[i].stdlist[m]) != 0) {
					break;
				}

				// 果然找到同分的 ...

				if ((depbuf[i].stdlist[m].aflag &
							SWF_INVALID) != 0) {
					continue; // 跳過那些已經高就的 ....
				}


				if (depbuf[i].stdlist[m].st->matched_wish
								== -1) {
					continue; // 跳過其它落榜生
				}

				found = 1;
				break;
			}

			if (found) {
				j = m;
			} else {
				depbuf[i].rotation = 0;
#if DEBUG_LEVEL > 1
				dprint ("lost\r\n");
#endif
				return 0;
			}
		}

		if ((depbuf[i].stdlist[j].st->sflag &
					STUDENT_FLAG_ADMIRER) != 0) {
			// 該生在其它的志願中, 已經進入了 Admirer Check
			//
			// 這時, 是否應該比較一下志願的次序 ?
			//
			// 有兩種 case
			//   1. 現在的志願序比已經在 Admirer Check
			//      中的志願序還後 ... 如果是這樣, 就放棄這個
			//      志願, 找下個人當 BEST Admirer
			//   2. 反之, 則放棄整個 Admirer Chain, 而且,
			//      在這個學生之前的所有 BEST Admirer 均視為
			//      無效

			w  = depbuf[i].stdlist[j].wish_order;
			ww = -1;
			ii = 0;

			for (kk = 0; kk < admirer_chain_idx; kk++) {
				ii = admirer_chain[kk].depidx;
				jj = admirer_chain[kk].stdidx;

				if (depbuf[i].stdlist[j].st ==
						depbuf[ii].stdlist[jj].st) {
					ww = depbuf[ii].stdlist[jj].wish_order;
					break;
				}
			}

			if (ww == -1) {
#if DEBUG_LEVEL > 0
				dprint ("Program Error on %s line %d\n",
						__FILE__, __LINE__);
#endif
			} else if (w > ww) {
				for (kk = 0; kk < admirer_chain_idx; kk++) {
					jj = admirer_chain[kk].depidx;

					// 已經不可能參與 Rotation 了 ???
					// depbuf[jj].rotation = 0;

					if (ii == jj) break;
				}
#if DEBUG_LEVEL > 1
				dprint ("%d skip [ " WISHID_FMT " , "
					WISHID_FMT " ](%d,%d)\r\n",
					depbuf[i].stdlist[j].st->sid,
					depbuf[i].id,
					depbuf[ii].id,
					w, ww);
#endif
				return -1;
			} else if (w < ww) {
				// skip
				continue;
			} else {
#if DEBUG_LEVEL > 0
				dprint ("Program Error on %s line %d\n",
						__FILE__, __LINE__);
#endif
			}
			continue;
		}

/*
		if ((depbuf[i].stdlist[j].aflag & SWF_ADMIRER_CHECK) != 0) {
			// 已經在 Admirer check 所以 ...
#if DEBUG_LEVEL > 0
			dprint ("loop detected\r\n");
#endif
			return 1;
		}
*/

		if (depbuf[i].stdlist[j].wish_order <
				depbuf[i].stdlist[j].st->matched_wish) {
			// BEST Admirer Got
			depbuf[i].rotation = 2;
			depbuf[i].stdlist[j].st->sflag |= STUDENT_FLAG_ADMIRER;
			// depbuf[i].stdlist[j].aflag |= SWF_ADMIRER_CHECK;

			x = depbuf[i].stdlist[j].st->matched_wish - 1;
			y = depbuf[i].stdlist[j].st->order_on_wish[x];

			/*
			k = search_department (depbuf[i].stdlist[j].st->wish[
				depbuf[i].stdlist[j].st->matched_wish - 1]);
				*/
			k = depbuf[i].stdlist[j].st->wishidx[x];

			admirer_chain[admirer_chain_idx].depidx  = i;
			admirer_chain[admirer_chain_idx].stdidx  = j;
			//admirer_chain[admirer_chain_idx].srcdep  = k;
			//admirer_chain[admirer_chain_idx].srcstd  = y;
			admirer_chain_idx++;
#if DEBUG_LEVEL > 1
			dprint ("(" WISHID_FMT ",%d)",
					depbuf[i].id,
					depbuf[i].stdlist[j].st->sid);
#endif

#if DEBUG_LEVEL > 1
			dprint ("Best Admirer: "
					WISHID_FMT " - %d (%d) on "
					WISHID_FMT " (%d)\r\n",
				depbuf[i].id,
				depbuf[i].stdlist[j].st->sid,
				depbuf[i].stdlist[j].wish_order,
				depbuf[i].stdlist[j].st->wish[
				depbuf[i].stdlist[j].st->matched_wish - 1],
				depbuf[i].stdlist[j].st->matched_wish);
#endif

			return to_find_admirer (k);
		}
	}

	depbuf[i].rotation = 0;
	return 0;
}
#endif

#if BEST_ADMIRER_ROTATION_VERSION > 0
static void to_clean_admirer_chain (void) {
	int	i, j, n;

	if (admirer_chain_idx == 0) return;

	for (n = 0; n < admirer_chain_idx; n++) {
		i = admirer_chain[n].depidx;
		j = admirer_chain[n].stdidx;

		depbuf[i].stdlist[j].st->sflag &= (~STUDENT_FLAG_ADMIRER);
	}
	admirer_chain_idx = 0;
}
#endif

#if BEST_ADMIRER_ROTATION_VERSION == 0
static void to_clean_admirer_chain (const int value) {
	int	i, j, n, start;

	if (admirer_chain_idx == 0) return;

	for (n = 0, start = -1; n < admirer_chain_idx; n++) {
		i = admirer_chain[n].depidx;
		j = admirer_chain[n].stdidx;
		// k = admirer_chain[n].srcdep;
		// m = admirer_chain[n].srcstd;

		depbuf[i].stdlist[j].st->sflag &= (~STUDENT_FLAG_ADMIRER);

		if (value > 0) {
			if (depbuf[i].rotation == 3) start = n;

			depbuf[i].rotation = 1;

			// 改在這裡將 matched_wish 寫對

			if (start >= 0) {
				if ((depbuf[i].stdlist[j].aflag &
							SWF_INVALID) == 0) {
					depbuf[i].stdlist[j].st->matched_wish =
						depbuf[i].stdlist[j].wish_order;
				}
			}
		} else if (value == 0) {
			depbuf[i].rotation = 0;
		} else if (depbuf[i].rotation >= 2) {
			depbuf[i].rotation = 1;
		}
	}

	admirer_chain_idx = 0;
}
#endif

#if BEST_ADMIRER_ROTATION_VERSION > 0
static int rotation_admirer_chain (const int from) {
	int	i = 0, j, n;
	int	cnt = 0;

	if (from == 0) return 0;
	if (admirer_chain_idx == 0) return 0;

	for (n = from - 1; n < admirer_chain_idx; n++) {
		i = admirer_chain[n].depidx;
		j = admirer_chain[n].stdidx;

		if (admirer_chain[n].admirer_flag == 0) {
			invalid_rest_wishes (&depbuf[i].stdlist[j]);
			cnt++;
		}
	}

	if (cnt > 0) {
		i = 0; while (do_require_fix ()) i++;
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
#endif

#if BEST_ADMIRER_ROTATION_VERSION == 0
static int rotation_admirer_chain (void) {
	static int	cnt = 0;
	int		i, j, n, start;

	if (admirer_chain_idx == 0) return 0;

#if DEBUG_LEVEL > 0
	if (cnt == 0) misc->print (PRINT_LEVEL_DEBUG, "\n");

	misc->print (PRINT_LEVEL_DEBUG, "%3d. ", cnt + 1);
#endif
	cnt++;

	for (n = 0, start = -1; n < admirer_chain_idx; n++) {
		i = admirer_chain[n].depidx;

		if (depbuf[i].rotation == 3) start = n;

#if DEBUG_LEVEL > 0
		if (start >= 0) {
			j = admirer_chain[n].stdidx;
			//k = admirer_chain[n].srcdep;
			//m = admirer_chain[n].srcstd;

			misc->print (PRINT_LEVEL_DEBUG,
					"(" WISHID_FMT ",%d)",
					depbuf[i].id,
					depbuf[i].stdlist[j].st->sid);

			// 先不考慮同分參酌、增額錄取等情形
			/*
			dprint ("[%d] Best Admirer: "
				WISHID_FMT " - %d (%d) on "
				WISHID_FMT " (%d) [%d/%d] %d\r\n",
					cnt,
					depbuf[i].id,
					depbuf[i].stdlist[j].st->sid,
					depbuf[i].stdlist[j].wish_order,
					depbuf[k].id,
					depbuf[i].stdlist[j].st->matched_wish,
					m, j,
					depbuf[k].stdlist[m].st->sid);
			*/
		}
#else
		if (start >= 0) break;
#endif
	}

	if (start < 0) return 0;

	i = admirer_chain[start].depidx;
	j = admirer_chain[start].stdidx;
	//k = admirer_chain[start].srcdep;
	//m = admirer_chain[start].srcstd;

#if DEBUG_LEVEL > 0
	misc->print (PRINT_LEVEL_DEBUG, "\n");
#endif

#if DEBUG_LEVEL > 1
	dprint ("Start from (%d,%d,%d)\r\n", start, i, j);
#endif
	// depbuf[k].stdlist[m].aflag |= SWF_INVALID;
	// depbuf[k].fix_require++; 

	// 請注意, 在此時, 程式已經認定這個 loop 已經找到,
	// 所以可以堅信去 invalid_rest_wishes 之後, 一定會
	// 導至 loop 回自己所 admirer 的志願, 如果這個條件
	// 不成立, 會使該生意外落榜
	invalid_rest_wishes (&depbuf[i].stdlist[j]);

	i = 0; while (do_require_fix ()) i++;

	if ((cnt > 0) && (i == 0)) return -1;

#if DEBUG_LEVEL > 1
	dprint ("[%d] %d walking\r\n", cnt, i);
#endif
	return admirer_chain_idx - start;
}
#endif

#if BEST_ADMIRER_ROTATION_VERSION > 0
static void write_all_ranklist_order (void) {
	int	i, j, k, order;

	misc->print (PRINT_LEVEL_INFO,
			"Write student\'s order for all ranklist ... ");
	misc->timer_started ();

	///////////////////////////////////////////////////////////////////
	//
	// 這個動作滿大的, 主要是除了名次表上的次序之外, 希望得到每個
	// 學生的排名, 以方便在 Rotation 上使用 ...
	// 先暫時寫寫看, 以後再決定要不要 Speedup
	
	for (i = 0; i < depnum; i++) {
		for (j = order = 0; j < depbuf[i].llindex; j++) {
			if ((depbuf[i].stdlist[j].aflag & SWF_INVALID) == 0) {
				depbuf[i].stdlist[j].order = ++order;
				break;
			}
		}

		for (k = j + 1; k < depbuf[i].llindex; k++) {
			if ((depbuf[i].stdlist[k].aflag & SWF_INVALID) == 0) {
				if (compare_order (&depbuf[i].stdlist[j],
						&depbuf[i].stdlist[k]) == 0) {
					// 同名
					depbuf[i].stdlist[k].order = order;
				} else {
					depbuf[i].stdlist[k].order = ++order;
				}

				j = k;
			}
		}
	}

	///////////////////////////////////////////////////////////////////

	misc->print (PRINT_LEVEL_INFO, "ok.  [ %.3f second(s) ]\r\n",
			misc->timer_ended ());
}
#endif

#if BEST_ADMIRER_ROTATION_VERSION > 0
#  if DEBUG_LEVEL > 1
static int	just_once = 1;
#  endif
static void prepare_admirer_data (const int i) {
	int	j, k, n;
#  if DEBUG_LEVEL > 1
	int	show_best_admirer = 0;
#  endif


	if (depbuf[i].num < depbuf[i].real_num) {
		depbuf[i].number_of_lastin =
				depbuf[i].real_num - depbuf[i].num  + 1;
	} else {
		depbuf[i].number_of_lastin = 1;
	}

#if DEBUG_LEVEL > 1
	if (just_once) {
		// if (depbuf[i].id == 0722) show_best_admirer = 1;
		// if (depbuf[i].id == 0753) show_best_admirer = 1;
		if (depbuf[i].id == trace_wishid) show_best_admirer = 1;
	}

	if (show_best_admirer) {
		dprint (WISHID_FMT ", Best Admirer:", depbuf[i].id);
	}
#endif

	if ((j = first_admirer (i, -1)) < 0) {
		depbuf[i].number_of_admirer = 0;
		depbuf[i].admirer_idx       = -1;
#if DEBUG_LEVEL > 1
		if (show_best_admirer) dprint ("N/A");
#endif
	} else {
		depbuf[i].admirer_idx       = j;
#if DEBUG_LEVEL > 1
		if (show_best_admirer) {
			dprint ("%d", depbuf[i].stdlist[j].st->sid);
		}
#endif
		for (n = 1; (k = next_admirer (i, j)) != -1; j = k) {
			n++;
#if DEBUG_LEVEL > 1
			if (show_best_admirer) {
				dprint (", %d", depbuf[i].stdlist[k].st->sid);
			}
#endif
		}
		depbuf[i].number_of_admirer = n;
	}
#if DEBUG_LEVEL > 1
	if (show_best_admirer) dprint ("\n");
#endif
}
#endif

#if BEST_ADMIRER_ROTATION_VERSION > 0
static void prepare_admirer_data_for_all (void) {
	int	i;

	/*
	misc->print (PRINT_LEVEL_INFO, "Prepare best admirer data ... ");

	misc->timer_started ();
	*/

	for (i = 0; i < depnum; i++) prepare_admirer_data (i);

	/*
	misc->print (PRINT_LEVEL_INFO,
			"ok.  [ %.3f second(s) ]\r\n",
			misc->timer_ended ());
	*/
#  if DEBUG_LEVEL > 1
	just_once = 0;
#  endif
}
#endif


static void set_all_department_rotation_flag (const int val) {
	int	i;

	for (i = 0; i < depnum; i++) {
#if BEST_ADMIRER_ROTATION_VERSION > 0
		depbuf[i].admirer_idx = -1;
		if (val < 0) {
			depbuf[i].rflag = 0;

			if (depbuf[i].number_of_admirer > 0) {
				depbuf[i].rotation =
					depbuf[i].number_of_lastin;
			} else {
				depbuf[i].rotation = -1;
				depbuf[i].rflag |= BARF_INVALID;
			}
		} else {
			depbuf[i].rotation = val;
		}
#else
		depbuf[i].rotation = val;
#endif
	}
}

static const char * rotation_engine_version (void) {
#if BEST_ADMIRER_ROTATION_VERSION == 0
	static const char	*version = "1.0";
#else
	static const char	*version = "2.0.7";
#endif
	return version;
}

static int do_rotate (void) {
	int			i, j, k;
	int			rotation_cnt;
	int			rotation_memb_cnt;
	int			have_best_admirer;

	// 因 rotation 的需要, 先把 matched_wish 寫好
	write_matched_wish ();

#if BEST_ADMIRER_ROTATION_VERSION > 0
	write_all_ranklist_order ();
#endif

	misc->print (PRINT_LEVEL_SYSTEM, "Trying to do rotation ... ");
	misc->timer_started ();

	// Admirer_chain 的長度不可能會超過志願數 ...
	//
	// [] !!!!! 注意: 在新的演算法中, Admirer Chain 可能超長 ...
	//                會當掉的 ... 不可不慎 !! (未來要改掉)

	admirer_chain = misc->malloc (depnum * sizeof (struct admirer_chain_t));
	admirer_chain_idx = 0;

	// to find "BEST ADMIRER" and try to find the loop

	rotation_cnt = rotation_memb_cnt = 0;

#if BEST_ADMIRER_ROTATION_VERSION == 0
	// 先假定每個學系都可以參與 rotation
	set_all_department_rotation_flag (1);

	do {	// 這個 loop 只能處理沒有增額錄取的那些學系
		for (i = j = have_best_admirer = 0; i < depnum; i++) {
			// 跳掉不參與 rotation 的學系志願
			if (depbuf[i].rotation == 0) continue;

			if ((j = to_find_admirer (i)) > 0) {
				have_best_admirer = 1;
				rotation_cnt++;

				if ((k = rotation_admirer_chain ()) < 0) {
					have_best_admirer = 0;
				}

				rotation_memb_cnt += k;
				// write_matched_wish ();
			}

			to_clean_admirer_chain (j);

			// if (j > 0) break;
		}
		// break;
	} while (have_best_admirer);
#endif

#if BEST_ADMIRER_ROTATION_VERSION > 0
	do {	// 這個 loop 是用來處理那些有增額錄取的學系 ....
		prepare_admirer_data_for_all ();
		set_all_department_rotation_flag (-1);

#if DEBUG_LEVEL > 9
		for (i = j = 0; i < depnum; i++) {
			if (depbuf[i].number_of_lastin > 1) {
				dprint (WISHID_FMT " lastin=%d\r\n",
					depbuf[i].id,
					depbuf[i].number_of_lastin);
			}

			if (depbuf[i].number_of_admirer > 1) {
				dprint (WISHID_FMT " admirer=%d\r\n",
					depbuf[i].id,
					depbuf[i].number_of_admirer);
			}
		}
#endif

		for (i = j = have_best_admirer = 0; i < depnum; i++) {
			// SWF_ADMIRER_CHECK
			// 跳掉不參與 rotation 的學系志願
			if ((depbuf[i].rflag & BARF_INVALID) != 0) continue;

#if DEBUG_LEVEL > 0
			if (depbuf[i].id == trace_wishid) {
				dprint ("On(" WISHID_FMT ")\n", depbuf[i].id);
			}
#endif

			to_clean_admirer_chain ();

			if ((j = find_admirer_v2 (i, 0)) > 0) {
				have_best_admirer = 1;
				rotation_cnt++;
				//rotation_memb_cnt += j;
				//if (rotation_cnt < 2) have_best_admirer = 1;
				//
				if ((k = rotation_admirer_chain (j)) < 0) {
					have_best_admirer = 0;
				} else {
					rotation_memb_cnt += k;
					write_matched_wish ();
				}
				// break;
			} else {
				set_all_department_rotation_flag (-1);
			}

			// if (j > 0) break;
		}
		// break;
	} while (have_best_admirer);
#endif

	misc->print (PRINT_LEVEL_SYSTEM,
			"ok.  [ %d rotate, %d shift, %.3f sec. ]\r\n",
			rotation_cnt,
			rotation_memb_cnt,
			misc->timer_ended ());

#if BEST_ADMIRER_ROTATION_VERSION == 0
	if (admirer_chain != NULL) free (admirer_chain);
#endif

	return 1;
}

#endif
