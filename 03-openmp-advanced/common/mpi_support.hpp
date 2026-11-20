#pragma once
#include <algorithm>
#include <mpi.h>
#include <stdexcept>
#include <vector>
struct Partition {
    std::vector<int> counts, offsets;
    int local = 0, start = 0;
};
inline Partition partition(int n, MPI_Comm comm) {
    int rank = 0, p = 0;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &p);
    Partition d;
    d.counts.resize(p);
    d.offsets.resize(p);
    int at = 0;
    for (int r = 0; r < p; ++r) {
        d.counts[r] = n / p + (r < n % p ? 1 : 0);
        d.offsets[r] = at;
        at += d.counts[r];
    }
    d.local = d.counts[rank];
    d.start = d.offsets[rank];
    return d;
}
inline double max_time(double local, MPI_Comm comm) {
    double result = 0;
    MPI_Allreduce(&local, &result, 1, MPI_DOUBLE, MPI_MAX, comm);
    return result;
}

namespace lab {
inline void mpi_require(int ok, const char *message, MPI_Comm comm) {
    MPI_Allreduce(MPI_IN_PLACE, &ok, 1, MPI_INT, MPI_MIN, comm);
    if (!ok)
        throw std::runtime_error(message);
}
} // namespace lab
