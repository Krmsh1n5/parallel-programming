// B1: affine. This file is a complete program.
#include "bench.hpp"
#include <omp.h>

void affine(const double *x, double *y, std::size_t n) {
#pragma omp parallel for default(none) shared(x, y, n)
    for (std::size_t i = 0; i < n; ++i)
        y[i] = 2 * x[i] + 1;
}

int main(int argc, char **argv) {
    try {

        const bool self_test = lab::take_self_test(argc, argv);
        lab::print_run(argc, argv);
        lab::arguments(argc, 3, "b1_affine [N [trials]] [--self-test]");
#pragma omp parallel
        {
#pragma omp critical(hello)
            {
                std::cout << "thread=" << omp_get_thread_num() << " team=" << omp_get_num_threads()
                          << '\n';
            }
        }
        if (self_test) {
            for (std::size_t n : {0, 1, 3, 7, 17, 1003}) {
                auto x = lab::input(n);
                std::vector<double> y(n, -1);
                affine(x.data(), y.data(), n);
                for (std::size_t i = 0; i < n; ++i)
                    lab::close(y[i], 2 * x[i] + 1, "affine element");
            }
            std::cout << "B1: correctness checks passed\n";
        }

        auto n = lab::arg(argc, argv, 1, 1048576, 134217728, true);
        int trials = int(lab::arg(argc, argv, 2, 7, 101));
        auto x = lab::input(n);
        std::vector<double> y(n);
        lab::measure_action(
            "affine", trials, [&] { affine(x.data(), y.data(), n); },
            [&] {
                for (std::size_t i = 0; i < n; ++i)
                    lab::close(y[i], 2 * x[i] + 1, "affine element");
            });

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
