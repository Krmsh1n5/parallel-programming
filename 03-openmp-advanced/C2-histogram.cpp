// C2: histogram. This file is a complete program.
#include "bench.hpp"
#include <omp.h>

std::vector<std::uint64_t> histogram(const std::vector<unsigned> &x, std::size_t bins) {
    const std::size_t n = x.size();
    std::vector<std::uint64_t> result(bins, 0), rows;
    int team = 0;
#pragma omp parallel default(none) shared(x, n, bins, rows, team, result)
    {
#pragma omp single
        {
            team = omp_get_num_threads();
            rows.assign(std::size_t(team) * bins, 0);
        }
        std::uint64_t *row = rows.data() + std::size_t(omp_get_thread_num()) * bins;
#pragma omp for
        for (std::size_t i = 0; i < n; ++i)
            ++row[x[i] % bins];
#pragma omp for
        for (std::size_t b = 0; b < bins; ++b)
            for (int t = 0; t < team; ++t)
                result[b] += rows[std::size_t(t) * bins + b];
    }
    return result;
}

int main(int argc, char **argv) {
    try {

        const bool self_test = lab::take_self_test(argc, argv);
        lab::print_run(argc, argv);
        lab::arguments(argc, 4, "c2_histogram [N [bins [trials]]] [--self-test]");
        lab::team_info();
        if (self_test) {
            for (std::size_t n : {0, 1, 3, 17, 65})
                for (std::size_t bins : {1, 3, 7, 31}) {
                    std::vector<unsigned> x(n);
                    std::vector<std::uint64_t> expected(bins, 0);
                    for (std::size_t i = 0; i < n; ++i) {
                        x[i] = unsigned(i % 23);
                        ++expected[x[i] % bins];
                    }
                    lab::require(histogram(x, bins) == expected, "histogram bins");
                }
            lab::require(histogram({0, 1, 7, 2, 8}, 3) == std::vector<std::uint64_t>({1, 2, 2}),
                         "hand histogram");
            std::cout << "C2: correctness checks passed\n";
        }

        auto n = lab::arg(argc, argv, 1, 1048576, 134217728, true),
             bins = lab::arg(argc, argv, 2, 31, 65536);
        int trials = int(lab::arg(argc, argv, 3, 5, 101));
        std::vector<unsigned> x(n);
        std::vector<std::uint64_t> expected(bins, 0), got;
        for (std::size_t i = 0; i < n; ++i) {
            x[i] = unsigned(i % 23);
            ++expected[x[i] % bins];
        }
        lab::measure_action(
            "histogram_including_storage_and_merge", trials, [&] { got = histogram(x, bins); },
            [&] { lab::require(got == expected, "histogram bins"); });

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
