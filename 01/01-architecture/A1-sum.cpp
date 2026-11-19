// A1: sum. This file is a complete program.
#include "bench.hpp"

double sum_one(const double *a, std::size_t n) {
    double s = 0;
    for (std::size_t i = 0; i < n; ++i)
        s += a[i];
    return s;
}

double sum_four(const double *a, std::size_t n) {
    double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    std::size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        s0 += a[i];
        s1 += a[i + 1];
        s2 += a[i + 2];
        s3 += a[i + 3];
    }
    for (; i < n; ++i)
        s0 += a[i];
    return (s0 + s1) + (s2 + s3);
}

int main(int argc, char **argv) {
    try {

        lab::arguments(argc, 3, "a1_sum [N [trials]]");
        for (std::size_t n : {0, 1, 2, 3, 4, 5, 17, 65}) {
            auto x = lab::input(n);
            lab::close(sum_four(x.data(), n), lab::serial_sum(x), "sum_four tail");
        }
        std::cout << "A1: correctness checks passed\n";
        auto n = lab::arg(argc, argv, 1, 1048576, 134217728);
        int trials = int(lab::arg(argc, argv, 2, 7, 101));
        auto x = lab::input(n);
        double expected = lab::serial_sum(x);
        lab::measure(
            "one", trials, [&] { return sum_one(x.data(), n); }, expected);
        lab::measure(
            "four", trials, [&] { return sum_four(x.data(), n); }, expected);
        const double rounding[] = {1e16, 1.0, -1e16, 1.0};
        std::cout << "Rounding example: exact=2, one=" << sum_one(rounding, 4)
                  << ", four=" << sum_four(rounding, 4) << '\n';

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
