#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "bench.hpp"
#include <new>
#include <omp.h>
#include <sstream>
#ifdef __linux__
#include <cerrno>
#include <sched.h>
std::string allowed_cpus() {
    for (int n = 128; n <= 1048576; n *= 2) {
        std::size_t bytes = CPU_ALLOC_SIZE(n);
        cpu_set_t *p = CPU_ALLOC(n);
        if (!p)
            throw std::bad_alloc();
        CPU_ZERO_S(bytes, p);
        int rc = sched_getaffinity(0, bytes, p), error = errno;
        std::ostringstream out;
        if (rc == 0) {
            for (int i = 0; i < n; ++i)
                if (CPU_ISSET_S(i, bytes, p))
                    out << i << ' ';
            CPU_FREE(p);
            return out.str();
        }
        CPU_FREE(p);
        if (error != EINVAL)
            throw std::runtime_error("sched_getaffinity failed");
    }
    throw std::runtime_error("affinity mask exceeds diagnostic limit");
}
int current_cpu() {
    int c = sched_getcpu();
    if (c < 0)
        throw std::runtime_error("sched_getcpu failed");
    return c;
}
#else
std::string allowed_cpus() {
    return "not available: Linux-specific diagnostic";
}
int current_cpu() {
    return -1;
}
#endif
int main(int argc, char **argv) {
    try {
        if (argc > 2)
            throw std::invalid_argument("usage: b3_affinity [sample_count]");
        auto samples = lab::arg(argc, argv, 1, 100000, 10000000);
        std::cout << "Initial thread allowed logical CPUs: " << allowed_cpus() << "\n";
#pragma omp parallel
        {
            // Diagnostic sampling, not a timed arithmetic kernel. B3: explain its limits.
            int first = -1, last = -1, changes = 0;
            std::string allowed, error;
            try {
                allowed = allowed_cpus();
                first = last = current_cpu();
                for (std::size_t k = 1; k < samples; ++k) {
                    int c = current_cpu();
                    if (c != last) {
                        ++changes;
                        last = c;
                    }
                }
            } catch (const std::exception &e) {
                error = e.what();
            }
#pragma omp critical(placement_output)
            {
                std::cout << "thread=" << omp_get_thread_num() << " team=" << omp_get_num_threads()
                          << " place=" << omp_get_place_num() << " first_cpu=" << first
                          << " last_cpu=" << last << " observed_changes=" << changes << " allowed={"
                          << allowed << "} error=" << error << "\n";
            }
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
