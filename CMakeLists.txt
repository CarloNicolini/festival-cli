cmake_minimum_required(VERSION 3.5...3.28)
project(Festival)

set(FESTIVAL_MAIN_SRC	src/sokoban_solver.cpp)
include_directories(${CMAKE_SOURCE_DIR}/src)

set(FESTIVAL_SRC_FILES
	src/advanced_deadlock.cpp
	src/advisors.cpp
	src/bfs.cpp
	src/biconnect.cpp
	src/board.cpp
	src/corral_deadlock.cpp
	src/corral.cpp
	src/cycle_deadlock.cpp
	src/deadlock_cache.cpp
	src/deadlock_utils.cpp
	src/deadlock.cpp
	src/debug.cpp
	src/diags.cpp
	src/distance.cpp
	src/dragonfly_queue.cpp
	src/dragonfly.cpp
	src/engine.cpp
	src/envelope.cpp
	src/fixed_boxes.cpp
	src/girl.cpp
	src/global.cpp
	src/graph.cpp
	src/greedy.cpp
	src/helper.cpp
	src/heuristics.cpp
	src/hf_search.cpp
	src/holes.cpp
	src/hotspot.cpp
	src/hungarian.cpp
	src/imagine.cpp
	src/io.cpp
	src/k_dist_deadlock.cpp
	src/lurd.cpp
	src/match_distance.cpp
	src/max_dist.cpp
	src/mini_search.cpp
	src/moves.cpp
	src/mpdb2.cpp
	src/naive.cpp
	src/oop.cpp
	src/opener.cpp
	src/order.cpp
	src/overlap.cpp
	src/packing_search.cpp
	src/park_order.cpp
	src/perimeter.cpp
	src/positions.cpp
	src/preprocess.cpp
	src/queue.cpp
	src/request.cpp
	src/rooms_deadlock.cpp
	src/rooms.cpp
	src/score.cpp
	src/snail.cpp
	src/sol.cpp
	src/stuck.cpp
	src/textfile.cpp
	src/tree.cpp
	src/util.cpp
	src/wobblers.cpp
	src/xy_deadlock.cpp
)

add_library(festival ${FESTIVAL_SRC_FILES})
add_executable(fess-cli ${FESTIVAL_MAIN_SRC})
target_link_libraries(fess-cli festival -pthread)
