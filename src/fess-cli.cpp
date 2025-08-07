// fess-cli.cpp: Command-line interface for Festival Sokoban Solver
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for getopt
#include <string.h>
#include <time.h>
#include "board.h"
#include "sol.h" // For solution handling
#include "io.h" // For print_board if verbose
#include "cli_parser.h" // For parse_ascii_to_board
#include "util.h" // For errors
#include "global.h"
#include "helper.h" // For helpers array
#include "lurd.h" // For moves_to_lurd
#include "tree.h" // For tree type
#include "moves.h"

// Forward declarations from sokoban_solver.cpp
extern tree search_trees[8];
extern helper helpers[8];
void solve_with_time_control(board b);
void allocate_search_trees();
void allocate_helpers();

// Forward declarations from other modules
void allocate_perimeter();
void allocate_deadlock_cache();
void init_dragonfly();
void read_deadlock_patterns(int pull_mode);
int count_pushes(char *sol, int len);
int find_sol_move(board b_in,
                  int start_sok_index, int start_box_index,
                  int end_sok_index, int end_box_index,
                  char *moves);
void apply_move(board b, move *m, int pull_mode);




int main(int argc, char **argv) {
    int opt;
    char *input_file = NULL;
    int time_alloc = 60; // Default seconds
    int verbose = 0;
    char *output_file = NULL;
    int search_mode = 0; // Default, map from option if added

    while ((opt = getopt(argc, argv, "t:vo:h")) != -1) {
        switch (opt) {
            case 't': time_alloc = atoi(optarg); break;
            case 'v': verbose = 1; break;
            case 'o': output_file = optarg; break;
            case 'h':
                printf("Usage: %s [options] <input_file>\n"
                       "Options:\n"
                       "  -t <sec>  Time allocation (default 60)\n"
                       "  -v        Verbose output\n"
                       "  -o <file> Output file\n", argv[0]);
                return 0;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: Input file required.\n");
        return 1;
    }
    input_file = argv[optind];
    // Load the ASCII map into memory for JSON output
    char *map_buffer = NULL;
    long map_len = 0;
    {
        FILE *mf = fopen(input_file, "r");
        if (!mf) exit_with_error("can't open map file");
        fseek(mf, 0, SEEK_END);
        map_len = ftell(mf);
        rewind(mf);
        map_buffer = (char*)malloc(map_len + 1);
        if (!map_buffer) exit_with_error("can't allocate memory for map_buffer");
        fread(map_buffer, 1, map_len, mf);
        map_buffer[map_len] = '\0';
        fclose(mf);
    }

    // Set globals
    cores_num = 1;
    time_limit = time_alloc;
    start_time = time(0);
    level_id = 1; // Dummy
    strcpy(global_level_set_name, "cli"); // Dummy
    strcpy(global_dir, "."); // Current directory
    verbose = verbose; // Use the verbose flag from command line
    
    // Initialize solver components (same as main() in sokoban_solver.cpp)
    allocate_perimeter();
    allocate_deadlock_cache();
    allocate_search_trees();
    allocate_helpers();
    init_dragonfly();
    
    read_deadlock_patterns(0); // normal mode
    read_deadlock_patterns(1); // pull mode

    board b;
    zero_board(b);
    if (parse_ascii_to_board(input_file, b, verbose) != 0) {
        fprintf(stderr, "Error parsing file.\n");
        return 1;
    }

    if (verbose) {
        printf("Initial board:\n");
        print_board(b);
    }

    solve_with_time_control(b);

    if (helpers[0].level_solved) {
        // Always simulate to build the full LURD sequence
        char full_lurd[1024 * 1024] = {0};
        int pos = 0;
        board sim_b;
        copy_board(initial_board, sim_b);
        int sok_y, sok_x;
        get_sokoban_position(sim_b, &sok_y, &sok_x);
        int total_steps = 0;
        int total_pushes = 0;

        for (int i = 0; i < helpers[0].sol_len; i++) {
            move m = helpers[0].sol_move[i];
            int start_box_index = m.from;
            int end_box_index = m.to;
            int end_box_y, end_box_x;
            index_to_y_x(end_box_index, &end_box_y, &end_box_x);
            int next_sok_y = end_box_y + delta_y[m.sokoban_position];
            int next_sok_x = end_box_x + delta_x[m.sokoban_position];
            char buffer[10000] = {0};
            int moves_num = find_sol_move(sim_b,
                                          y_x_to_index(sok_y, sok_x),
                                          start_box_index,
                                          y_x_to_index(next_sok_y, next_sok_x),
                                          end_box_index,
                                          buffer);
            for (int j = 0; j < moves_num; j++) {
                if (pos < (int)sizeof(full_lurd) - 1) full_lurd[pos++] = buffer[j];
            }
            full_lurd[pos] = '\0';
            total_steps += moves_num;
            total_pushes += count_pushes(buffer, moves_num);
            apply_move(sim_b, &m, NORMAL);
            sok_y = next_sok_y;
            sok_x = next_sok_x;
        }

        if (verbose) {
            printf("Solution found! %d moves in solution\n", helpers[0].sol_len);
   
            // Debug: print the moves
            printf("Raw moves:\n");
            for (int i = 0; i < helpers[0].sol_len; i++) {
                int from_y, from_x, to_y, to_x;
                index_to_y_x(helpers[0].sol_move[i].from, &from_y, &from_x);
                index_to_y_x(helpers[0].sol_move[i].to, &to_y, &to_x);
                printf("Move %d: from=%d(%d,%d) to=%d(%d,%d) pull=%d\n", i,
                       helpers[0].sol_move[i].from, from_y, from_x,
                       helpers[0].sol_move[i].to, to_y, to_x,
                       helpers[0].sol_move[i].pull);
            }

            // Proper LURD generation using simulation approach like save_sol_moves
            printf("LURD: %s (%d steps, %d pushes)\n", full_lurd, total_steps, total_pushes);
        }

        // Write JSON output to a .json file
        {
            const char *fname = strrchr(input_file, '/');
            if (fname) fname++;
            else fname = input_file;
            char json_filename[2000];
            strncpy(json_filename, fname, sizeof(json_filename)-1);
            json_filename[sizeof(json_filename)-1] = '\0';
            char *dot = strrchr(json_filename, '.');
            if (dot) *dot = '\0';
            strcat(json_filename, ".json");
            FILE *jf = fopen(json_filename, "w");
            if (jf) {
                fprintf(jf, "{\n");
                fprintf(jf, "  \"filename\": \"%s\",\n", fname);
                fprintf(jf, "  \"map\": \"");
                for (long _i = 0; _i < map_len; _i++) {
                    char c = map_buffer[_i];
                    if (c == '\\') fprintf(jf, "\\\\");
                    else if (c == '\"') fprintf(jf, "\\\"");
                    else if (c == '\n') fprintf(jf, "\\n");
                    else if (c == '\r') continue;
                    else fprintf(jf, "%c", c);
                }
                fprintf(jf, "\",\n");
                fprintf(jf, "  \"lurd\": \"%s\",\n", full_lurd);
                fprintf(jf, "  \"elapsed\": \"%d\"\n", end_time - start_time);
                fprintf(jf, "}\n");
                fclose(jf);
            } else {
                fprintf(stderr, "Error: cannot write JSON file %s\n", json_filename);
            }
        }
    } else {
        std::cerr << "No solution found within time limit" << std::endl;
    }

    // No free, assume program ends
    if (map_buffer) free(map_buffer);
    return 0;
}
