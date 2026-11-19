// A3: stride. This file is a complete program.
#include "bench.hpp"

double strided_sum(const double *a, std::size_t n, std::size_t stride) {
    double s = 0;
    for (std::size_t i = 0; i < n; i += stride)
        s += a[i];
    return s;
}

int main(int argc, char **argv) {
    try {

        lab::arguments(argc, 3, "a3_stride [N [trials]]");
        for (std::size_t n : {0, 1, 3, 10, 17, 65})
            for (std::size_t stride : {1, 3, 8, 64}) {
                auto x = lab::input(n);
                double expected = 0;
                for (std::size_t i = 0; i < n; i += stride)
                    expected += x[i];
                lab::close(strided_sum(x.data(), n, stride), expected, "strided sum");
            }
        std::cout << "A3: correctness checks passed\n";
        auto n = lab::arg(argc, argv, 1, 1048576, 134217728);
        int trials = int(lab::arg(argc, argv, 2, 7, 101));
        auto x = lab::input(n);
        for (std::size_t stride : {1, 2, 4, 8, 16, 32, 64}) {
            double expected = 0;
            for (std::size_t i = 0; i < n; i += stride)
                expected += x[i];
            double t = lab::measure(
                "stride_" + std::to_string(stride), trials,
                [&] { return strided_sum(x.data(), n, stride); }, expected);
            auto visits = (n - 1) / stride + 1;
            std::cout << "stride=" << stride << ", visits=" << visits
                      << ", ns_per_visit=" << t * 1e9 / visits
                      << ", useful_GB_s=" << visits * sizeof(double) / t / 1e9 << '\n';
        }

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
