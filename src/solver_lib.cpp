// solver_lib.cpp - Festival solver functions without main()
// This file contains the solver functions needed by fess-cli

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef THREADS
#include <pthread.h>
#endif

#include "global.h"
#include "board.h"
#include "distance.h"
#include "deadlock.h"
#include "engine.h"
#include "rooms.h"
#include "io.h"
#include "advanced_deadlock.h"
#include "debug.h"
#include "hotspot.h"
#include "sol.h"
#include "util.h"
#include "deadlock_cache.h"
#include "packing_search.h"
#include "park_order.h"
#include "perimeter.h"
#include "mpdb2.h"
#include "envelope.h"
#include "k_dist_deadlock.h"
#include "oop.h"
#include "imagine.h"
#include "preprocess.h"
#include "holes.h"
#include "rooms_deadlock.h"
#include "girl.h"
#include "helper.h"
#include "stuck.h"
#include "dragonfly.h"
#include "snail.h"

tree search_trees[8];
helper helpers[8];

int forced_alg = -1;

// Copy all the functions from sokoban_solver.cpp except main()

void allocate_search_trees()
{
	int log_size = 23; // about 1.5GB per core
	int i;

	if ((cores_num != 1) && (cores_num != 2) && (cores_num != 4) && (cores_num != 8))
		exit_with_error("Number of cores should be 1/2/4/8");

#ifdef VISUAL_STUDIO
	log_size = 22; // should fit in a the 2GB memory limit...
#endif

	log_size += extra_mem;

	for (i = 0; i < cores_num; i++)
	{
		if (i == 7) log_size -= 3; // dragonfly does not need the tree

		if (verbose >= 4) printf("Allocating search tree for core %d\n", i);
		init_tree(&search_trees[i], log_size);
	}
}

void free_search_trees()
{
	int i;
	for (i = 0; i < cores_num; i++)
		free_tree(search_trees + i);
}

void allocate_helpers()
{
	int i;

	for (i = 0; i < cores_num; i++)
	{
		init_helper(helpers + i);
		init_helper_extra_fields(helpers + i);
		helpers[i].my_core = i;
	}
}

void free_helpers()
{
	int i;

	for (i = 0; i < cores_num; i++)
		free_helper(helpers + i);
}

int preprocess_level(board b)
{
	int res;

	strcpy(global_fail_reason, "Unknown reason");

	if ((height == 0) || (width == 0)) return 0;

	if (sanity_checks(b) == 0)
		return 0;

	clear_deadlock_cache();
	clear_k_dist_hash();
	clear_perimeter();

	turn_decorative_boxes_to_walls(b);
	close_holes_in_board(b);

	init_inner(b);
	init_index_x_y();

	keep_boxes_in_inner(b);
	save_initial_board(b);
	expand_sokoban_cloud(b);

	set_forbidden_tunnel();
	mark_target_holes(b);

	if (verbose >= 3)
	{
		printf("\nLevel %d:\n", level_id);
		print_board(b);
	}

	set_distances(b);

	res = analyse_rooms(b);
	if (res == 0) return 0;

	init_rooms_deadlock();

	init_hotspots(b);

	build_mpdb2();
	build_pull_mpdb2();

	init_envelope_patterns();
	
	init_girl_variables(b);
	init_stuck_patterns();

	detect_snail_level(b);

	return 1;
}

int setup_plan_features(int search_type, helper *h)
{
	if (search_type == HF_SEARCH) return 1; 
	if (search_type == BICON_SEARCH) return 1;
	if (search_type == MAX_DIST_SEARCH) return 1;
	if (search_type == MAX_DIST_SEARCH2) return 1;
	if (search_type == REV_SEARCH) return 1;
	if (search_type == NAIVE_SEARCH) return 1;
	if (search_type == DRAGONFLY) return 1;

	if (h->parking_order_num == 0)
	{
		if (verbose >= 4)
			printf("No packing order\n");
		strcpy(global_fail_reason, "Could not find packing order");
		return 0;
	}

	verify_parking_order(h);
	reduce_parking_order(h);
	show_parking_order(h);

	if (search_type == GIRL_SEARCH) return 1;

	prepare_oop_zones(h);

	return 1;
}

void packing_search_control(board b, int time_allocation, int search_type, tree* t, helper* h)
{
	int end_time = (int)time(0) + time_allocation;

	if (time_allocation <= 0) return;

	h->weighted_search = 1;

	search_type = set_snail_parameters(search_type, 1, h); 
	search_type = set_netlock_parameters(search_type, 1, h);
	
	if (search_type == SNAIL_SEARCH)   time_allocation *= 2;
	if (search_type == NETLOCK_SEARCH) time_allocation *= 2;
	if (search_type == DRAGONFLY)      time_allocation *= 3;

	if (search_type == REV_SEARCH)
	{
		FESS(b, time_allocation, search_type, t, h);
		return;
	}

	if (search_type == DRAGONFLY)
	{
		dragonfly_search(b, time_allocation, h);
		return;
	}

	packing_search(b, time_allocation, search_type, t, h);
}

void forward_search_control(board b, int time_allocation, int search_type, tree* t, helper* h)
{
	int end_time = (int)time(0) + time_allocation;

	if (time_allocation <= 0) return;

	h->weighted_search = 1;

	search_type = set_snail_parameters(search_type, 0, h);
	search_type = set_netlock_parameters(search_type, 0, h);

	if (search_type == HF_SEARCH)
	{
		h->weighted_search = 0;
		FESS(b, time_allocation * 3 / 4, search_type, t, h);
		if (h->level_solved) return;

		h->weighted_search = 1;
		time_allocation = end_time - (int)time(0);
		FESS(b, time_allocation, search_type, t, h);

		return;
	}

	if (search_type == REV_SEARCH)
	{
		packing_search(b, time_allocation, search_type, t, h);
		if (h->perimeter_found == 0) return;
		time_allocation = end_time - (int)time(0);
	}

	FESS(b, time_allocation, search_type, t, h);
	return;
}

void solve_with_alg(board b, int time_allocation, int strategy_index, helper *h)
{
	// Strategy A: eliminate boxes via sink squares. Stop packing search when boxes are removed from targets.
	// Strategy B: do not eliminate boxes. Use 1/3 of the time for preparing the perimeter.
	// Strategy C: girl mode 
	// strategy D: HF search

	int local_start_time, remaining_time;
	int search_type;
	tree *t;

	int backward_search_type[8] = 
	{
		BASE_SEARCH,
		MAX_DIST_SEARCH2, //NORMAL, 
		GIRL_SEARCH, 
		HF_SEARCH , 
		BICON_SEARCH, 
		MAX_DIST_SEARCH, 
		REV_SEARCH,
		DRAGONFLY
	};
	
	int forward_search_type[8] = 
	{
		FORWARD_WITH_BASES, 
		HF_SEARCH, //NORMAL, 
		GIRL_SEARCH, 
		HF_SEARCH , 
		HF_SEARCH,    
		HF_SEARCH, 
		REV_SEARCH,
		NAIVE_SEARCH,
	};

	if (time_allocation <= 0) return;

	reset_helper(h);
	t = search_trees + h->my_core;
	
	// backward search

	search_type = backward_search_type[strategy_index];

	if (verbose >= 4)
	{
		printf("Starting strategy %c. ", 'A' + strategy_index);
		printf(" Time limit: %d seconds\n", time_allocation);
	}

	local_start_time = (int)time(0);
	
	packing_search_control(b, time_allocation / 3, search_type, t, h);

	if (setup_plan_features(search_type, h) == 0)
		return;

	// forward search

	remaining_time = local_start_time + time_allocation - (int)time(0);

	search_type = forward_search_type[strategy_index];;

	forward_search_control(b, remaining_time, search_type, t, h);
}

typedef struct
{
	board b;
	int time_allocation;
	int alg;
	helper *h;
} work_element;

void *solve_work_element(void *we_in)
{
	work_element *we = (work_element*)we_in;

	if ((cores_num > 1) && (verbose >= 4))
		printf("core %d starting\n", we->h->my_core);

	solve_with_alg(we->b, we->time_allocation, we->alg, we->h);

	if ((cores_num > 1) && (verbose >= 4))
		printf("core %d ending\n", we->h->my_core);
	
	return NULL;
}

void prepare_work_element(board b, int time_allocation, int alg, helper *h, work_element *we)
{
	copy_board(b, we->b);
	we->time_allocation = time_allocation;
	we->alg = alg;
	we->h = h;
}

int get_search_time(double ratio)
{
	int remaining_time;

	remaining_time = start_time + time_limit - (int)time(0);
	return (int)(remaining_time * ratio);
}

void solve_with_time_control_single_core(board b)
{
	work_element we;
	int search_time;
	int i;

	if (forced_alg != -1)
	{
		search_time = get_search_time(1.0);
		prepare_work_element(b, search_time, forced_alg, helpers + 0, &we);
		solve_work_element((void*)(&we));
		return;
	}

	for (i = 0; i < 8; i++)
	{
		search_time = get_search_time(1 / (double)(8 - i));

		prepare_work_element(b, search_time, i, helpers + 0, &we);
		solve_work_element((void*)(&we));
		if (helpers[0].level_solved) return;
	}
}

void solve_with_time_control(board b)
{
	int i;

	start_time = (int)time(0);
	any_core_solved = 0;

	for (i = 0; i < cores_num; i++)
		reset_helper(helpers + i); // remove leftovers solutions from previous levels

	if (preprocess_level(b) == 1)
	{
		if (cores_num == 1) solve_with_time_control_single_core(b);
		// For CLI, we'll only use single core for simplicity
	}
	else
	{
		if (verbose >= 4)
			printf("preprocess failed\n");
	}

	end_time = (int)time(0);

	if (end_time < start_time) end_time = start_time;
}
