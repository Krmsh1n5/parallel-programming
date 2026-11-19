// B2: dot. This file is a complete program.
#include "bench.hpp"
#include <omp.h>

double dot_reduction(const double *x, const double *y, std::size_t n) {
    double s = 0;
#pragma omp parallel for default(none) shared(x, y, n) reduction(+ : s)
    for (std::size_t i = 0; i < n; ++i)
        s += x[i] * y[i];
    return s;
}

double dot_atomic(const double *x, const double *y, std::size_t n) {
    double s = 0;
#pragma omp parallel for default(none) shared(x, y, n, s)
    for (std::size_t i = 0; i < n; ++i) {
#pragma omp atomic update
        s += x[i] * y[i];
    }
    return s;
}

double dot_critical(const double *x, const double *y, std::size_t n) {
    double s = 0;
#pragma omp parallel for default(none) shared(x, y, n, s)
    for (std::size_t i = 0; i < n; ++i) {
#pragma omp critical(dot_sum)
        s += x[i] * y[i];
    }
    return s;
}

int main(int argc, char **argv) {
    try {

        const bool self_test = lab::take_self_test(argc, argv);
        lab::print_run(argc, argv);
        lab::arguments(argc, 3, "b2_dot [N [trials]] [--self-test]");
        lab::team_info();
        if (self_test) {
            for (std::size_t n : {0, 1, 3, 17, 1003}) {
                auto x = lab::input(n), y = lab::input(n, 7);
                double expected = 0;
                for (std::size_t i = 0; i < n; ++i)
                    expected += x[i] * y[i];
                lab::close(dot_reduction(x.data(), y.data(), n), expected, "reduction");
                lab::close(dot_atomic(x.data(), y.data(), n), expected, "atomic");
                lab::close(dot_critical(x.data(), y.data(), n), expected, "critical");
            }
            std::cout << "B2: correctness checks passed\n";
        }

        auto n = lab::arg(argc, argv, 1, 1048576, 134217728, true);
        int trials = int(lab::arg(argc, argv, 2, 7, 101));
        auto x = lab::input(n), y = lab::input(n, 7);
        double expected = 0;
        for (std::size_t i = 0; i < n; ++i)
            expected += x[i] * y[i];
        lab::measure(
            "reduction", trials, [&] { return dot_reduction(x.data(), y.data(), n); }, expected);
        lab::measure(
            "atomic", trials, [&] { return dot_atomic(x.data(), y.data(), n); }, expected);
        lab::measure(
            "critical", trials, [&] { return dot_critical(x.data(), y.data(), n); }, expected);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
