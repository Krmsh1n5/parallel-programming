#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _OPENMP
#include <omp.h>
#endif
namespace lab {
// Keep repeated reads visible to the optimizer without making the arrays volatile.
inline void compiler_memory_barrier() {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" ::: "memory");
#endif
}
inline void arguments(int argc, int maximum, const char *usage) {
    if (argc > maximum)
        throw std::invalid_argument(std::string("usage: ") + usage);
}
inline void team_info() {
#ifdef _OPENMP
#pragma omp parallel
    {
#pragma omp single
        { std::cout << "actual_threads," << omp_get_num_threads() << "\n"; }
    }
#endif
}

inline std::size_t arg(int argc, char **argv, int i, std::size_t fallback, std::size_t cap,
                       bool zero = false) {
    if (i >= argc)
        return fallback;
    std::string s = argv[i];
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("expected an unsigned decimal argument");
    std::size_t used = 0;
    auto n = std::stoull(s, &used);
    if (used != s.size() || (!zero && n == 0) || n > cap)
        throw std::invalid_argument("argument outside documented range");
    return static_cast<std::size_t>(n);
}
inline double value(std::size_t i) { return double(i % 97 + 1) / 128.0; }
inline void require(bool ok, const std::string &message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline void close(double a, double b, const std::string &tag) {
    require(std::isfinite(a) && std::isfinite(b) && std::abs(a - b) <= 1e-11 * (1 + std::abs(b)),
            tag + ": incorrect result");
}
using Clock = std::chrono::steady_clock;
inline double seconds(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double>(b - a).count();
}
inline double median(std::vector<double> x) {
    std::sort(x.begin(), x.end());
    return x.size() % 2 ? x[x.size() / 2] : (x[x.size() / 2 - 1] + x[x.size() / 2]) * 0.5;
}
inline volatile double result_sink = 0; // Only the result is volatile, not the input arrays.
template <class F> double measure(const std::string &name, int trials, F f, double expected) {
    close(f(), expected, name + " warm-up");
    std::vector<double> times;
    for (int t = 0; t < trials; ++t) {
        compiler_memory_barrier();
        auto a = Clock::now();
        double result = f();
        auto b = Clock::now();
        close(result, expected, name);
        result_sink = result;
        times.push_back(seconds(a, b));
        std::cout << "sample," << name << "," << t << "," << std::setprecision(9) << times.back()
                  << "\n";
    }
    auto m = median(times);
    std::cout << "summary," << name << ",median_s," << m << ",min_s,"
              << *std::min_element(times.begin(), times.end()) << ",max_s,"
              << *std::max_element(times.begin(), times.end()) << "\n";
    return m;
}

template <class F, class V>
double measure_action(const std::string &name, int trials, F run, V validate) {
    run();
    validate();
    std::vector<double> times;
    for (int t = 0; t < trials; ++t) {
        compiler_memory_barrier();
        auto a = Clock::now();
        run();
        auto b = Clock::now();
        validate();
        times.push_back(seconds(a, b));
        std::cout << "sample," << name << "," << t << "," << std::setprecision(9) << times.back()
                  << "\n";
    }
    auto m = median(times);
    std::cout << "summary," << name << ",median_s," << m << ",min_s,"
              << *std::min_element(times.begin(), times.end()) << ",max_s,"
              << *std::max_element(times.begin(), times.end()) << "\n";
    return m;
}
inline std::vector<double> input(std::size_t n, std::size_t offset = 0) {
    std::vector<double> a(n);
    for (std::size_t i = 0; i < n; ++i)
        a[i] = value(i + offset);
    return a;
}
inline void summary(const std::string &name, const std::vector<double> &times) {
    std::cout << "summary," << name << ",median_s," << median(times) << ",min_s,"
              << *std::min_element(times.begin(), times.end()) << ",max_s,"
              << *std::max_element(times.begin(), times.end()) << "\n";
}
inline double serial_sum(const std::vector<double> &x) {
    double s = 0;
    for (double v : x)
        s += v;
    return s;
}
} // namespace lab
