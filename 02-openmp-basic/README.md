# PW2: openmp basic

This folder is self-contained. Copy or share it on its own: its CMake helpers and headers are included in `cmake/` and `common/`. No parent or sibling folder is required. Install the system tools below before compiling.

Requirements: CMake 3.16+, a C++17 compiler with OpenMP support.

## Build

Run inside this folder:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Executables are in `build/bin/`. To rebuild only one program, use `cmake --build build --target TARGET -j`.

Complete the TODO functions in the section’s `.cpp`, rebuild and run. Unfinished functions report an error; compilation alone does not validate an implementation. Other sections do not need to be completed first.

## Inputs and checks

Positional arguments accept any integer within the indicated range, not just the examples in the handout. Bounds limit memory use and integer representation. Omitted arguments use defaults. To change a later argument, supply the preceding ones too. Choose sizes that fit available RAM.

The requested input is checked against the reference before its timing is reported. Programs that list `--self-test` also offer extra small/edge-case checks: add this flag to run those before the requested case. Without the flag, no predefined size sweep runs. The separate A1 rounding demonstration and fixed diagnostic/ring examples are not size benchmarks.

| Source | Target | Arguments | Positional defaults and ranges |
|---|---|---|---|
| `B1-affine.cpp` | `b1_affine` | `[N [trials]] [--self-test]` | 1: default 1048576, range 0–134217728; 2: default 7, range 1–101 |
| `B2-dot.cpp` | `b2_dot` | `[N [trials]] [--self-test]` | 1: default 1048576, range 0–134217728; 2: default 7, range 1–101 |
| `B3-affinity.cpp` | `b3_affinity` | `none` | 1: default 100000, range 1–10000000 |
| `B4-alignment.cpp` | `b4_alignment` | `none` | — |

Set `OMP_NUM_THREADS` and `OMP_DYNAMIC=false` for reproducible team requests, within your CPU allocation.

Read `main.pdf` for the questions. Record measurements in `results-template.csv`. To rebuild the handout, run `pdflatex -interaction=nonstopmode -halt-on-error main.tex` twice in this folder (requires the TeX packages used by `main.tex`).
