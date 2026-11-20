#include "bench.hpp"
#include <atomic>
#include <memory>
#include <omp.h>
struct Packed {
    std::atomic<std::uint64_t> value;
};
struct alignas(64) Separated {
    std::atomic<std::uint64_t> value;
};
template <class Counter> void run(const char *label, std::size_t iterations, int trials) {
    auto counters = std::make_unique<Counter[]>(omp_get_max_threads());
    for (int repeat = 0; repeat < trials; ++repeat) {
        for (int t = 0; t < omp_get_max_threads(); ++t)
            counters[t].value.store(0, std::memory_order_relaxed);
        int actual = 0;
        auto start = lab::Clock::now();
#pragma omp parallel shared(counters, actual, iterations)
        {
            int tid = omp_get_thread_num();
#pragma omp single
            { actual = omp_get_num_threads(); }
            for (std::size_t k = 0; k < iterations; ++k)
                counters[tid].value.fetch_add(1, std::memory_order_relaxed);
        }
        auto stop = lab::Clock::now();
        for (int t = 0; t < actual; ++t)
            lab::require(counters[t].value.load(std::memory_order_relaxed) == iterations,
                         "counter");
        std::cout << label << ",sample," << repeat << ",seconds," << lab::seconds(start, stop)
                  << ",actual_threads," << actual << ",object_size," << sizeof(Counter)
                  << ",alignment," << alignof(Counter) << "\n";
    }
}
int main(int argc, char **argv) {
    try {
        if (argc > 3)
            throw std::invalid_argument("usage: c5_false_sharing [increments [trials]]");
        auto n = lab::arg(argc, argv, 1, 1000000, 100000000);
        int trials = int(lab::arg(argc, argv, 2, 5, 101));
        run<Packed>("packed", n, trials);
        run<Separated>("separated", n, trials);
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
