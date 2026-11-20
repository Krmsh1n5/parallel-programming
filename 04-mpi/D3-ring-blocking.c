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


void ring_exchange(double value, double *from_left, double *from_right, MPI_Comm comm)
{
    enum { TO_RIGHT = 0, TO_LEFT = 1 };
    int rank, size;
    MPI_Comm_rank(comm, &rank); MPI_Comm_size(comm, &size);
    int left = (rank + size - 1) % size, right = (rank + 1) % size;
    MPI_Sendrecv(&value, 1, MPI_DOUBLE, right, TO_RIGHT,
                 from_left, 1, MPI_DOUBLE, left, TO_RIGHT, comm, MPI_STATUS_IGNORE);
    MPI_Sendrecv(&value, 1, MPI_DOUBLE, left, TO_LEFT,
                 from_right, 1, MPI_DOUBLE, right, TO_LEFT, comm, MPI_STATUS_IGNORE);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    if (argc != 1) fail("Usage: d3_ring (no arguments)");
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); MPI_Comm_size(MPI_COMM_WORLD, &size);
    for (int trial = 0; trial < 3; ++trial) {
        double from_left = -1.0, from_right = -1.0;
        ring_exchange(rank + 1.0 + trial * 100, &from_left, &from_right, MPI_COMM_WORLD);
        check_close(from_left, (rank + size - 1) % size + 1.0 + trial * 100);
        check_close(from_right, (rank + 1) % size + 1.0 + trial * 100);
    }
    if (rank == 0) puts("PASS");
    MPI_Finalize(); return 0;
}
