/* Sequential reference. Build: cc -std=c11 -O2 life.c -o life_serial */
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Depends only on global coordinates, never on a rank or thread number. */
static unsigned char initial_cell(int row, int col)
{
    uint32_t x = (uint32_t)row * 747796405u
               ^ (uint32_t)col * 2891336453u ^ 12345u;
    x ^= x >> 16;
    return (unsigned char)(x % 5u == 0);
}

/* P1 is the plain-text PBM format: 1 = black (alive), 0 = white (dead). */
static int save_pbm(const char *name, const unsigned char *grid, int n)
{
    FILE *file = fopen(name, "w");
    if (file == NULL) return 0;
    fprintf(file, "P1\n%d %d\n", n, n);
    for (int row = 0; row < n; ++row)
        for (int col = 0; col < n; ++col)
            fprintf(file, "%d%c", grid[(size_t)row * n + col],
                    col + 1 == n || (col + 1) % 32 == 0 ? '\n' : ' ');
    int ok = !ferror(file);
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int read_number(const char *text, int minimum, int maximum, int *value)
{
    char *end;
    errno = 0;
    long x = strtol(text, &end, 10);
    if (errno || *text == '\0' || *end != '\0' || x < minimum || x > maximum)
        return 0;
    *value = (int)x;
    return 1;
}

int main(int argc, char **argv)
{
    int n = 64, steps = 20;
    if (argc > 3
        || (argc > 1 && !read_number(argv[1], 1, 46340, &n))
        || (argc > 2 && !read_number(argv[2], 0, INT_MAX, &steps))) {
        fprintf(stderr, "Usage: %s [N [steps]] (1 <= N <= 46340, steps >= 0)\n", argv[0]);
        return 1;
    }
    size_t cells = (size_t)n * n;
    unsigned char *current = calloc(cells, sizeof *current);
    unsigned char *next = calloc(cells, sizeof *next);
    if (current == NULL || next == NULL) {
        fprintf(stderr, "Cannot allocate the grids.\n");
        free(current); free(next);
        return 1;
    }
    for (int row = 0; row < n; ++row)
        for (int col = 0; col < n; ++col)
            current[(size_t)row * n + col] = initial_cell(row, col);

    for (int step = 0; step < steps; ++step) {
        unsigned long long alive = 0;
        for (int row = 0; row < n; ++row) {
            for (int col = 0; col < n; ++col) {
                int neighbours = 0;
                for (int dr = -1; dr <= 1; ++dr) {
                    for (int dc = -1; dc <= 1; ++dc) {
                        int r = row + dr, c = col + dc;
                        if ((dr != 0 || dc != 0) && r >= 0 && r < n && c >= 0 && c < n)
                            neighbours += current[(size_t)r * n + c];
                    }
                }
                size_t k = (size_t)row * n + col;
                next[k] = (unsigned char)(neighbours == 3
                       || (current[k] && neighbours == 2));
                alive += next[k];
            }
        }
        unsigned char *tmp = current; current = next; next = tmp;
        printf("step %d alive %llu\n", step + 1, alive);
    }
    int ok = save_pbm("life-final.pbm", current, n);
    if (!ok) fprintf(stderr, "Cannot write life-final.pbm.\n");
    free(current); free(next);
    return ok ? 0 : 1;
}
