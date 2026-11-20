// C4: jacobi. This file is a complete program.
#include "bench.hpp"
#include <omp.h>

std::vector<double> serial_jacobi(std::vector<double> a, std::size_t steps) {
    auto b = a;
    for (std::size_t t = 0; t < steps; ++t) {
        for (std::size_t i = 1; i + 1 < a.size(); ++i)
            b[i] = 0.5 * (a[i - 1] + a[i + 1]);
        a.swap(b);
    }
    return a;
}

void jacobi(std::vector<double> &a, std::size_t steps) {
    std::vector<double> b = a;
    double *src = a.data(), *dst = b.data();
    const std::size_t last = a.size() > 1 ? a.size() - 1 : 0;
#pragma omp parallel default(none) shared(src, dst, last, steps)
    for (std::size_t t = 0; t < steps; ++t) {
#pragma omp for
        for (std::size_t i = 1; i < last; ++i)
            dst[i] = 0.5 * (src[i - 1] + src[i + 1]);
#pragma omp single
        std::swap(src, dst);
    }
    if (src != a.data())
        a.swap(b);
}

int main(int argc, char **argv) {
    try {

        const bool self_test = lab::take_self_test(argc, argv);
        lab::print_run(argc, argv);
        lab::arguments(argc, 4, "c4_jacobi [N [steps [trials]]] [--self-test]");
        lab::team_info();
        if (self_test) {
            for (std::size_t n : {0, 1, 2, 17, 65})
                for (std::size_t steps : {0, 1, 2, 5}) {
                    std::vector<double> a(n + 2, 0);
                    for (std::size_t i = 1; i <= n; ++i)
                        a[i] = lab::value(i);
                    auto expected = serial_jacobi(a, steps);
                    jacobi(a, steps);
                    lab::require(a.size() == expected.size(), "Jacobi output size");
                    for (std::size_t i = 0; i < a.size(); ++i)
                        lab::close(a[i], expected[i], "Jacobi element");
                }
            std::cout << "C4: correctness checks passed\n";
        }

        auto n = lab::arg(argc, argv, 1, 65536, 134217728, true),
             steps = lab::arg(argc, argv, 2, 20, 10000, true);
        int trials = int(lab::arg(argc, argv, 3, 5, 101));
        std::vector<double> initial(n + 2, 0), a;
        for (std::size_t i = 1; i <= n; ++i)
            initial[i] = lab::value(i);
        auto expected = serial_jacobi(initial, steps);
        lab::measure_action(
            "jacobi_with_input_copy", trials,
            [&] {
                a = initial;
                jacobi(a, steps);
            },
            [&] {
                lab::require(a.size() == expected.size(), "Jacobi output size");
                for (std::size_t i = 0; i < a.size(); ++i)
                    lab::close(a[i], expected[i], "Jacobi element");
            });

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
