// A2: matrix. This file is a complete program.
#include "bench.hpp"

double matrix_rows(const double *a, std::size_t side) {
    double s = 0;
    for (std::size_t r = 0; r < side; ++r)
        for (std::size_t c = 0; c < side; ++c)
            s += a[r * side + c];
    return s;
}

double matrix_columns(const double *a, std::size_t side) {
    double s = 0;
    for (std::size_t c = 0; c < side; ++c)
        for (std::size_t r = 0; r < side; ++r)
            s += a[r * side + c];
    return s;
}

int main(int argc, char **argv) {
    try {

        lab::arguments(argc, 3, "a2_matrix [side [trials]]");
        for (std::size_t side : {0, 1, 3, 17, 65}) {
            auto x = lab::input(side * side);
            lab::close(matrix_columns(x.data(), side), lab::serial_sum(x), "matrix columns");
        }
        std::cout << "A2: correctness checks passed\n";
        auto side = lab::arg(argc, argv, 1, 1024, 8192);
        int trials = int(lab::arg(argc, argv, 2, 7, 101));
        auto x = lab::input(side * side);
        double expected = lab::serial_sum(x);
        lab::measure(
            "rows", trials, [&] { return matrix_rows(x.data(), side); }, expected);
        lab::measure(
            "columns", trials, [&] { return matrix_columns(x.data(), side); }, expected);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
