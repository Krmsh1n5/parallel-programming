// C1: schedule. This file is a complete program.
#include "bench.hpp"
#include <omp.h>

std::uint64_t irregular_value(std::size_t i, std::size_t n) {
    std::size_t work = i < n / 4 ? 4000 : 32;
    std::uint64_t x = i + 1;
    for (std::size_t k = 0; k < work; ++k)
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
    return x;
}

void scheduled_work(std::size_t n, std::vector<std::uint64_t> &out) {
#pragma omp parallel for default(none) shared(n, out) schedule(runtime)
    for (std::size_t i = 0; i < n; ++i)
        out[i] = irregular_value(i, n);
}

int main(int argc, char **argv) {
    try {

        const bool self_test = lab::take_self_test(argc, argv);
        lab::print_run(argc, argv);
        lab::arguments(argc, 3, "c1_schedule [N [trials]] [--self-test]");
        lab::team_info();
        if (self_test) {
            for (std::size_t n : {0, 1, 3, 17, 65}) {
                std::vector<std::uint64_t> y(n);
                scheduled_work(n, y);
                for (std::size_t i = 0; i < n; ++i)
                    lab::require(y[i] == irregular_value(i, n), "scheduled output");
            }
            std::cout << "C1: correctness checks passed\n";
        }

        auto n = lab::arg(argc, argv, 1, 16384, 1048576, true);
        int trials = int(lab::arg(argc, argv, 2, 5, 101));
        std::vector<std::uint64_t> y(n), expected(n);
        for (std::size_t i = 0; i < n; ++i)
            expected[i] = irregular_value(i, n);
        lab::measure_action(
            "schedule", trials, [&] { scheduled_work(n, y); },
            [&] { lab::require(y == expected, "scheduled output"); });

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
