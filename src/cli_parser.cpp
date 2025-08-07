#include "cli_parser.h"
#include <stdio.h>
#include <string.h>
#include "util.h" // exit_with_error
#include "preprocess.h" // sanity_checks, etc.
#include "global.h" // for height, width globals

// Forward declaration from sokoban_solver.cpp
int preprocess_level(board b);

// External declaration for verbose flag
extern int verbose;

int parse_ascii_to_board(const char* filename, board b, int verbose_flag) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Could not open file: %s\n", filename);
        return -1;
    }

    char lines[MAX_SIZE][MAX_SIZE];
    int height_local = 0, max_width = 0;

    // Read all lines
    while (fgets(lines[height_local], MAX_SIZE, fp) && height_local < MAX_SIZE) {
        remove_newline(lines[height_local]);
        int len = strlen(lines[height_local]);
        if (len > max_width) max_width = len;
        height_local++;
    }
    fclose(fp);

    if (height_local == 0) {
        printf("Empty file\n");
        return -1;
    }

    if (verbose_flag) printf("Read %d lines, max width %d\n", height_local, max_width);

    // Pad to rectangular
    for (int i = 0; i < height_local; i++) {
        int len = strlen(lines[i]);
        if (len < max_width) {
            memset(lines[i] + len, ' ', max_width - len);
            lines[i][max_width] = '\0';
        }
    }

    zero_board(b);

    // Map characters to board
    for (int y = 0; y < height_local; y++) {
        for (int x = 0; x < max_width; x++) {
            char c = lines[y][x];
            switch (c) {
                case ' ': b[y][x] = SPACE; break;
                case '#': b[y][x] = WALL; break;
                case '@': b[y][x] = SOKOBAN; break;
                case '.': b[y][x] = TARGET; break;
                case '$': b[y][x] = BOX; break;
                case '*': b[y][x] = BOX | TARGET; break;
                case '+': b[y][x] = SOKOBAN | TARGET; break;
                default: 
                    printf("Invalid character '%c' at position (%d,%d)\n", c, y, x);
                    return -1;
            }
        }
    }

    // Set global dimensions
    height = height_local;
    width = max_width;

    if (verbose_flag) printf("Set board dimensions: %dx%d\n", width, height);

    // Debug: print the parsed board before preprocessing (only in verbose mode)
    if (verbose_flag) {
        printf("Parsed board before preprocessing:\n");
        for (int y = 0; y < height_local; y++) {
            printf("Row %d: ", y);
            for (int x = 0; x < max_width; x++) {
                char c = '?';
                if (b[y][x] == SPACE) c = ' ';
                else if (b[y][x] == WALL) c = '#';
                else if (b[y][x] == BOX) c = '$';
                else if (b[y][x] == TARGET) c = '.';
                else if (b[y][x] == SOKOBAN) c = '@';
                else if (b[y][x] == (BOX | TARGET)) c = '*';
                else if (b[y][x] == (SOKOBAN | TARGET)) c = '+';
                printf("%c", c);
            }
            printf(" (");
            for (int x = 0; x < max_width; x++) {
                printf("%d ", b[y][x]);
            }
            printf(")\n");
        }
    }

    // Try preprocessing
    int result = preprocess_level(b);
    if (result == 0) {
        printf("Preprocessing failed\n");
        return -1;
    }

    if (verbose_flag) printf("Preprocessing successful\n");
    return 0;
}
