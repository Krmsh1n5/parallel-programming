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
double distributed_vectors(const double *x, const double *y, double *gathered,
                           int n, MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_rank(comm, &rank); MPI_Comm_size(comm, &size);
    int *counts = allocate((size_t)size, sizeof *counts);
    int *displs = allocate((size_t)size, sizeof *displs);
    partition(n, size, counts, displs);
    int local_n = counts[rank];
    double *local_x = allocate((size_t)local_n, sizeof *local_x);
    double *local_y = allocate((size_t)local_n, sizeof *local_y);
    double *local_z = allocate((size_t)local_n, sizeof *local_z);
    MPI_Scatterv(x, counts, displs, MPI_DOUBLE, local_x, local_n, MPI_DOUBLE, 0, comm);
    MPI_Scatterv(y, counts, displs, MPI_DOUBLE, local_y, local_n, MPI_DOUBLE, 0, comm);
    double partial = 0.0, total = 0.0;
    for (int i = 0; i < local_n; ++i) {
        local_z[i] = 2.0 * local_x[i] + 1.0;
        partial += local_x[i] * local_y[i];
    }
    MPI_Allreduce(&partial, &total, 1, MPI_DOUBLE, MPI_SUM, comm);
    MPI_Gatherv(local_z, local_n, MPI_DOUBLE, gathered, counts, displs, MPI_DOUBLE, 0, comm);
    free(local_z); free(local_y); free(local_x); free(displs); free(counts);
    return total;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    if (argc > 3) fail("Usage: d2_vectors [N [trials]]");
    int n = argument(argc, argv, 1, 1048576, 0, 100000000);
    int trials = argument(argc, argv, 2, 7, 1, 101), rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    double *x = NULL, *y = NULL, *gathered = NULL, expected = 0.0;
    if (rank == 0) {
        x = allocate((size_t)n, sizeof *x); y = allocate((size_t)n, sizeof *y);
        gathered = allocate((size_t)n, sizeof *gathered);
        for (int i = 0; i < n; ++i) { x[i] = value(i, 0); y[i] = value(i, 7); expected += x[i] * y[i]; }
    }
    MPI_Bcast(&expected, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    double times[101];
    for (int k = -1; k < trials; ++k) { /* k=-1 is a discarded warm-up sample. */
        if (rank == 0) for (int i = 0; i < n; ++i) gathered[i] = -1.0;
        MPI_Barrier(MPI_COMM_WORLD);
        double begin = MPI_Wtime();
        double answer = distributed_vectors(x, y, gathered, n, MPI_COMM_WORLD);
        double local_time = MPI_Wtime() - begin, slowest = 0.0;
        MPI_Reduce(&local_time, &slowest, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        check_close(answer, expected);
        if (rank == 0) {
            for (int i = 0; i < n; ++i) check_close(gathered[i], 2.0 * x[i] + 1.0);
            if (k >= 0) times[k] = slowest;
        }
    }
    if (rank == 0) { report("distributed", times, trials); puts("PASS"); }
    free(gathered); free(y); free(x); MPI_Finalize(); return 0;
}
