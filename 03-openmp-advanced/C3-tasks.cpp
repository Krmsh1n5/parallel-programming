// C3: tasks. This file is a complete program.
#include "bench.hpp"
#include <omp.h>

double split_sum(const double *x, std::size_t begin, std::size_t end, std::size_t cutoff) {
    if (end - begin <= cutoff) {
        double s = 0;
        for (std::size_t i = begin; i < end; ++i)
            s += x[i];
        return s;
    }
    std::size_t mid = begin + (end - begin) / 2;
    double left = 0;
#pragma omp task default(none) firstprivate(x, begin, mid, cutoff) shared(left)
    left = split_sum(x, begin, mid, cutoff);
    double right = split_sum(x, mid, end, cutoff);
#pragma omp taskwait
    return left + right;
}

double task_sum(const double *x, std::size_t n, std::size_t cutoff) {
    double s = 0;
#pragma omp parallel default(none) shared(x, n, cutoff, s)
#pragma omp single
    s = split_sum(x, 0, n, cutoff);
    return s;
}

int main(int argc, char **argv) {
    try {

        const bool self_test = lab::take_self_test(argc, argv);
        lab::print_run(argc, argv);
        lab::arguments(argc, 4, "c3_tasks [N [cutoff [trials]]] [--self-test]");
        lab::team_info();
        if (self_test) {
            for (std::size_t n : {0, 1, 2, 11, 17, 65})
                for (std::size_t cutoff : {1, 3, 128}) {
                    auto x = lab::input(n);
                    lab::close(task_sum(x.data(), n, cutoff), lab::serial_sum(x), "task sum");
                }
            std::cout << "C3: correctness checks passed\n";
        }

        auto n = lab::arg(argc, argv, 1, 1048576, 134217728, true),
             cutoff = lab::arg(argc, argv, 2, 2048, 134217728);
        int trials = int(lab::arg(argc, argv, 3, 5, 101));
        auto x = lab::input(n);
        lab::measure(
            "tasks", trials, [&] { return task_sum(x.data(), n, cutoff); }, lab::serial_sum(x));

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
