// A4: transpose. This file is a complete program.
#include "bench.hpp"

void transpose_reference(const double *a, double *b, std::size_t n) {
    for (std::size_t r = 0; r < n; ++r)
        for (std::size_t c = 0; c < n; ++c)
            b[c * n + r] = a[r * n + c];
}

void transpose_tiled(const double *a, double *b, std::size_t n, std::size_t tile) {
    for (std::size_t r0 = 0; r0 < n; r0 += tile) {
        std::size_t r1 = std::min(r0 + tile, n);
        for (std::size_t c0 = 0; c0 < n; c0 += tile) {
            std::size_t c1 = std::min(c0 + tile, n);
            for (std::size_t r = r0; r < r1; ++r)
                for (std::size_t c = c0; c < c1; ++c)
                    b[c * n + r] = a[r * n + c];
        }
    }
}

int main(int argc, char **argv) {
    try {

        lab::arguments(argc, 3, "a4_transpose [side [trials]]");
        for (std::size_t n : {0, 1, 3, 17, 65})
            for (std::size_t tile : {1, 8, 16, 32, 64}) {
                auto x = lab::input(n * n);
                std::vector<double> y(n * n, -1);
                transpose_tiled(x.data(), y.data(), n, tile);
                for (std::size_t r = 0; r < n; ++r)
                    for (std::size_t c = 0; c < n; ++c)
                        lab::close(y[c * n + r], x[r * n + c], "transpose element");
            }
        std::cout << "A4: correctness checks passed\n";
        auto n = lab::arg(argc, argv, 1, 1024, 8192);
        int trials = int(lab::arg(argc, argv, 2, 5, 101));
        auto x = lab::input(n * n);
        std::vector<double> y(n * n), expected(n * n);
        transpose_reference(x.data(), expected.data(), n);
        auto validate = [&] { lab::require(y == expected, "transpose"); };
        lab::measure_action(
            "plain", trials, [&] { transpose_reference(x.data(), y.data(), n); }, validate);
        for (std::size_t tile : {8, 16, 32, 64})
            lab::measure_action(
                "tile_" + std::to_string(tile), trials,
                [&] { transpose_tiled(x.data(), y.data(), n, tile); }, validate);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
