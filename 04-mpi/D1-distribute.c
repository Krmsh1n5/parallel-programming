#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include <mpi.h>

static inline _Noreturn void fail(const char *message)
{
    fprintf(stderr, "%s\n", message);
    MPI_Abort(MPI_COMM_WORLD, 1); exit(EXIT_FAILURE);
}

static inline int argument(int argc, char **argv, int position,
                           int fallback, int minimum, int maximum)
{
    if (position >= argc) return fallback;
    char *end;
    errno = 0;
    long x = strtol(argv[position], &end, 10);
    if (errno || end == argv[position] || *end || x < minimum || x > maximum)
        fail("Invalid numerical argument.");
    return (int)x;
}

static inline void *allocate(size_t count, size_t width)
{
    if (count == 0) count = 1;
    if (count > SIZE_MAX / width) fail("Allocation size overflow.");
    void *p = calloc(count, width);
    if (p == NULL) fail("Allocation failed.");
    return p;
}

static inline double value(int i, int seed)
{
    return (double)((i + seed) % 97) / 16.0;
}

static inline double now_seconds(void)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) fail("Clock failed.");
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

static inline void check_close(double actual, double expected)
{
    if (!isfinite(actual) || fabs(actual - expected) > 1e-10 * fmax(1.0, fabs(expected)))
        fail("Incorrect numerical result.");
}

static inline int compare_times(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static inline double report(const char *label, double *times, int trials)
{
    if (trials < 1 || trials > 101) fail("Invalid trial count.");
    for (int k = 0; k < trials; ++k)
        printf("%s sample %d seconds %.9g\n", label, k + 1, times[k]);
    qsort(times, (size_t)trials, sizeof *times, compare_times);
    double median = trials % 2 ? times[trials / 2]
                             : 0.5 * (times[trials / 2 - 1] + times[trials / 2]);
    printf("%s median %.9g min %.9g max %.9g\n",
           label, median, times[0], times[trials - 1]);
    return median;
}


/* Arrays have one entry per communicator member. Displacements count doubles. */
void partition(int n, int size, int *counts, int *displs)
{
    int offset = 0;
    for (int r = 0; r < size; ++r) {
        counts[r] = n / size + (r < n % size);
        displs[r] = offset; offset += counts[r];
    }
}
void distribute(const double *global, double *local, const int *counts,
                const int *displs, int local_n, MPI_Comm comm)
{
    MPI_Scatterv(global, counts, displs, MPI_DOUBLE,
                 local, local_n, MPI_DOUBLE, 0, comm);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    if (argc > 2) fail("Usage: d1_distribute [N]");
    int n = argument(argc, argv, 1, 10, 0, 100000000), rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); MPI_Comm_size(MPI_COMM_WORLD, &size);
    int *counts = allocate((size_t)size, sizeof *counts);
    int *displs = allocate((size_t)size, sizeof *displs);
    partition(n, size, counts, displs);
    double *global = NULL;
    if (rank == 0) {
        global = allocate((size_t)n, sizeof *global);
        for (int i = 0; i < n; ++i) global[i] = value(i, 0);
    }
    int local_n = counts[rank];
    double *local = allocate((size_t)local_n, sizeof *local);
    for (int i = 0; i < local_n; ++i) local[i] = -1.0;
    distribute(global, local, counts, displs, local_n, MPI_COMM_WORLD);
    for (int i = 0; i < local_n; ++i)
        if (local[i] != value(displs[rank] + i, 0)) fail("Incorrect distributed element.");
    for (int r = 0; r < size; ++r) {
        MPI_Barrier(MPI_COMM_WORLD);
        if (rank == r) { printf("rank %d count %d start %d\n", rank, local_n, displs[rank]); fflush(stdout); }
    }
    if (rank == 0) puts("PASS");
    free(local); free(global); free(displs); free(counts); MPI_Finalize(); return 0;
}
