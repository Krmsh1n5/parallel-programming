# PW4 — C exercises

Read `main.pdf`. Every `.c` file is a complete, independently buildable C11 program, with no course-specific headers. Its `main` allocates ordinary arrays, initializes input, calls the exercise functions, checks the result and prints timings. All code is visible in that file. The first small helper functions handle arguments, allocation, comparison and wall-clock samples.

Complete functions marked TODO. B3 is supplied complete; C5 requires the indicated alignment edit. An unimplemented function prints its name and exits rather than pretending to compute a result. No implementation in another exercise is needed. Retain the stated input/output contracts. Free arrays when finished; output array ownership stays with the caller unless explicitly allocated by the function.

## Build

Use a C11 compiler and CMake 3.16+. Timing uses the POSIX monotonic clock; the teaching environment is Linux. OpenMP is required for PW2/PW3 and MPI for PW4. The alignment diagnostic itself needs no OpenMP.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Executables are in `build/bin`. Each folder has its own CMake helper and can be copied on its own. A direct command for a serial file is `cc -std=c11 -O2 FILE.c -lm -o program`; add `-fopenmp` for an OpenMP file, or use `mpicc` for an MPI file. No C++ compiler or C++ MPI bindings are used. Choose sizes within available memory.

## Programs and positional arguments

Trial count is 1..101. To set a later argument, also supply earlier ones. There is no `--self-test` option: the program checks the requested case; run different small inputs explicitly. Instructor CTest covers extra cases.

| File | Target | Arguments and ranges |
|---|---|---|
| `D1-distribute.c` | `d1_distribute` | [N]; default 10; N 0..100000000 |
| `D2-vectors.c` | `d2_vectors` | [N [trials]]; defaults 1048576,7; N 0..100000000 |
| `D3-ring-blocking.c` | `d3_ring` | no arguments |
| `D4-ring-nonblocking.c` | `d4_ring` | no arguments |

## Checks and timing

Numerical drivers check each requested result, including individual output elements/bins. Empty inputs are accepted where the table allows zero; empty allocations retain one harmless element. Checks and printed samples are outside timed kernels. The code prints every sample and a median/minimum/maximum summary. Use optimized builds, repeat trials and check correctness before interpreting performance. Do not enable fast-math or link-time optimization for these comparisons. No speedup is promised.

D1 passes ordinary pointers to a Scatterv; its main owns the local allocation. D2 returns a scalar dot product on every rank and fills the root-owned `gathered` output. Its timed function includes count/local allocation, both scatters, local arithmetic, Allreduce, Gatherv and local frees. Root input/output allocation, checks and timing aggregation are outside. Each trial reports the maximum local MPI_Wtime duration; one warm-up sample is discarded. D3/D4 fill output values through pointer parameters and test three outgoing values, including one-rank self exchange. All ranks participate even with zero local elements.

Recompile the handout with `pdflatex -interaction=nonstopmode -halt-on-error main.tex` twice.
