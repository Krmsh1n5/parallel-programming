#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

static unsigned char initial_cell(int row, int col)
{
    uint32_t x = (uint32_t)row * 747796405u
               ^ (uint32_t)col * 2891336453u ^ 12345u;
    x ^= x >> 16;
    return (unsigned char)(x % 5u == 0);
}

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

static inline _Noreturn void fail(const char *message)
{
    fprintf(stderr, "%s\n", message);
    MPI_Abort(MPI_COMM_WORLD, 1); exit(EXIT_FAILURE);
}

static inline void *allocate(size_t count, size_t width)
{
    if (count == 0) count = 1;
    if (count > SIZE_MAX / width) fail("Allocation size overflow.");
    void *p = calloc(count, width);
    if (p == NULL) fail("Allocation failed.");
    return p;
}

static void partition(int n, int size, int *counts, int *starts)
{
    int offset = 0;
    for (int r = 0; r < size; ++r) {
        counts[r] = n / size + (r < n % size);
        starts[r] = offset;
        offset += counts[r];
    }
}

static void exchange_halos(unsigned char *grid, int rows, int n, int rank, int size)
{
    enum { TO_UP = 0, TO_DOWN = 1 };
    int up = rank > 0 ? rank - 1 : MPI_PROC_NULL;
    int down = rank + 1 < size ? rank + 1 : MPI_PROC_NULL;
    size_t width = (size_t)n;
    unsigned char *top_halo = grid;
    unsigned char *first_row = grid + width;
    unsigned char *last_row = grid + (size_t)rows * width;
    unsigned char *bottom_halo = grid + (size_t)(rows + 1) * width;
    MPI_Sendrecv(first_row, n, MPI_UNSIGNED_CHAR, up, TO_UP,
                 bottom_halo, n, MPI_UNSIGNED_CHAR, down, TO_UP,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Sendrecv(last_row, n, MPI_UNSIGNED_CHAR, down, TO_DOWN,
                 top_halo, n, MPI_UNSIGNED_CHAR, up, TO_DOWN,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

static unsigned long long evolve(const unsigned char *current, unsigned char *next,
                                 int rows, int n)
{
    unsigned long long alive = 0;
    size_t width = (size_t)n;
#pragma omp parallel for default(none) shared(current, next, rows, n, width) \
    schedule(static) reduction(+ : alive)
    for (int row = 1; row <= rows; ++row) {
        for (int col = 0; col < n; ++col) {
            int neighbours = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    int r = row + dr, c = col + dc;
                    if ((dr != 0 || dc != 0) && c >= 0 && c < n)
                        neighbours += current[(size_t)r * width + c];
                }
            }
            size_t k = (size_t)row * width + col;
            next[k] = (unsigned char)(neighbours == 3
                   || (current[k] && neighbours == 2));
            alive += next[k];
        }
    }
    return alive;
}

static void export_grid(const unsigned char *owned, int n, const int *counts,
                        const int *starts, int rank, int size)
{
    int *cell_counts = NULL, *cell_starts = NULL;
    unsigned char *grid = NULL;
    if (rank == 0) {
        cell_counts = allocate((size_t)size, sizeof *cell_counts);
        cell_starts = allocate((size_t)size, sizeof *cell_starts);
        grid = allocate((size_t)n * n, sizeof *grid);
        for (int r = 0; r < size; ++r) {
            cell_counts[r] = counts[r] * n;
            cell_starts[r] = starts[r] * n;
        }
    }
    MPI_Gatherv(owned, counts[rank] * n, MPI_UNSIGNED_CHAR,
                grid, cell_counts, cell_starts, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);
    if (rank == 0 && !save_pbm("life-final.pbm", grid, n))
        fail("Cannot write life-final.pbm.");
    free(grid); free(cell_starts); free(cell_counts);
}

int main(int argc, char **argv)
{
    int provided, rank, size;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (provided < MPI_THREAD_FUNNELED) {
        if (rank == 0) fprintf(stderr, "MPI does not support MPI_THREAD_FUNNELED.\n");
        MPI_Finalize();
        return 1;
    }

    int n = 64, steps = 20;
    if (argc > 3
        || (argc > 1 && !read_number(argv[1], 1, 46340, &n))
        || (argc > 2 && !read_number(argv[2], 0, INT_MAX, &steps))) {
        if (rank == 0)
            fprintf(stderr, "Usage: %s [N [steps]] (1 <= N <= 46340, steps >= 0)\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    if (size > n) {
        if (rank == 0)
            fprintf(stderr, "Cannot run %d ranks on %d rows: the rank count must not exceed N.\n",
                    size, n);
        MPI_Finalize();
        return 1;
    }

    int *counts = allocate((size_t)size, sizeof *counts);
    int *starts = allocate((size_t)size, sizeof *starts);
    partition(n, size, counts, starts);
    int rows = counts[rank], first = starts[rank];
    size_t width = (size_t)n, local_cells = (size_t)(rows + 2) * width;
    unsigned char *current = allocate(local_cells, sizeof *current);
    unsigned char *next = allocate(local_cells, sizeof *next);

#pragma omp parallel for default(none) shared(current, rows, n, width, first) schedule(static)
    for (int i = 0; i < rows; ++i)
        for (int col = 0; col < n; ++col)
            current[(size_t)(i + 1) * width + col] = initial_cell(first + i, col);

    for (int step = 0; step < steps; ++step) {
        exchange_halos(current, rows, n, rank, size);
        unsigned long long local_alive = evolve(current, next, rows, n), alive = 0;
        MPI_Reduce(&local_alive, &alive, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM,
                   0, MPI_COMM_WORLD);
        unsigned char *tmp = current; current = next; next = tmp;
        if (rank == 0) printf("step %d alive %llu\n", step + 1, alive);
    }

    export_grid(current + width, n, counts, starts, rank, size);

    free(current); free(next); free(starts); free(counts);
    MPI_Finalize();
    return 0;
}
