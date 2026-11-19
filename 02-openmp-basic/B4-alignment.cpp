// B4: alignment. This file is a complete program.
#include "bench.hpp"
#include <array>

std::uintptr_t power_two_alignment(std::uintptr_t p) {
    return p & (~p + 1);
}

int main(int argc, char **argv) {
    try {

        lab::arguments(argc, 1, "b4_alignment");
        (void)argv;
        for (auto address : {std::uintptr_t(0), std::uintptr_t(1), std::uintptr_t(6),
                             std::uintptr_t(8), std::uintptr_t(24), std::uintptr_t(64),
                             std::uintptr_t(1) << (8 * sizeof(std::uintptr_t) - 1)}) {
            auto p = address;
            std::uintptr_t expected = p ? 1 : 0;
            while (p && p % 2 == 0) {
                expected *= 2;
                p /= 2;
            }
            lab::require(power_two_alignment(address) == expected, "alignment result");
        }
        alignas(64) std::array<double, 8> a{};
        auto p = reinterpret_cast<std::uintptr_t>(a.data());
        lab::require(p % 64 == 0, "base alignment");
        std::cout << "B4: correctness checks passed\nbase=" << p
                  << ", observed alignment=" << power_two_alignment(p)
                  << "\nsecond element=" << reinterpret_cast<std::uintptr_t>(&a[1])
                  << ", observed alignment="
                  << power_two_alignment(reinterpret_cast<std::uintptr_t>(&a[1])) << '\n';

        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
