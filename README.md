[![C/ASM CI](https://github.com/kirill-bayborodov/bignum-init-pow10/actions/workflows/ci.yml/badge.svg)](https://github.com/kirill-bayborodov/bignum-init-pow10/actions/workflows/ci.yml)
[![GitHub release](https://img.shields.io/github/v/release/kirill-bayborodov/bignum-init-pow10?label=release)](https://github.com/kirill-bayborodov/bignum-init-pow10/releases/latest)

`bignum-init-pow10` is a standalone C/ASM module that initializes a normalized 2048-bit `bignum_t` object with the value `10^k`. The production implementation is x86-64 YASM conforming to the System V AMD64 ABI; a portable C11 implementation is retained as an independent reference path.

The operation uses no dynamic allocation. It validates the destination and exponent before changing memory, computes the result with word-wise multiplication by ten, and returns a typed `bignum_init_pow10_status_t` status.

## Distribution

The module is intended to be used as a standalone component of the `bignum-lib` family. Its `bignum-core`, `bignum-init-u64`, `bignum-copy`, and `bignum-mul-bignum` dependencies are supplied as Git submodules.

## Features

- **Dual implementation:** x86-64 YASM is the primary implementation and C11 is the reference implementation.
- **Typed status API:** success, NULL-argument, invalid-exponent, and capacity-overflow statuses are explicit.
- **Complete initialization:** every successful call clears the complete destination before writing the normalized result.
- **Exact 2048-bit boundary:** `10^616` is supported; `10^617` does not fit and is rejected.
- **No dynamic allocation:** the ASM path uses registers and a fixed stack-local 264-byte working object.
- **Efficient hot path:** the ASM path multiplies each 64-bit word by ten with `mul`, `add`, and `adc`, propagating a single carry across the active words.
- **Independent reference:** the C path uses `__uint128_t` for portable per-word carry arithmetic and does not call the generic multiplication module.
- **Deterministic verification:** tests cover small powers, zero exponent, maximum exponent, repeated initialization, invalid arguments, canaries, all representable exponents, fuzz sequences, MT execution, and the integration runner.
- **Reproducible benchmarks:** single-thread and multithreaded benchmarks report iterations, successful calls, checksum, elapsed time, and nanoseconds per call.
- **Template build workflow:** the official Makefile is retained without modification.

## Dependencies

| Dependency | Purpose |
|---|---|
| `make` | Build, test, lint, benchmark, and distribution targets |
| `gcc` | C compilation and linking |
| `yasm` | x86-64 assembly compilation |
| `cppcheck` | Static analysis |
| `valgrind` | Leak and memory diagnostics |
| `perf` | Performance counters and sampling profiles |
| `taskset` | CPU affinity control |
| `pthread` | Multithreaded tests and benchmarks |

Clone the repository with all submodules:

```bash
git clone --recurse-submodules https://github.com/kirill-bayborodov/bignum-init-pow10.git
cd bignum-init-pow10
```

For an existing clone:

```bash
git submodule update --init --recursive
```

## API

The public API is declared in `include/bignum_init_pow10.h`:

```c
typedef enum {
    BIGNUM_INIT_POW10_SUCCESS                = 0,
    BIGNUM_INIT_POW10_ERROR_NULL_ARG         = -1,
    BIGNUM_INIT_POW10_ERROR_INVALID_EXPONENT = -2,
    BIGNUM_INIT_POW10_ERROR_OVERFLOW         = -3
} bignum_init_pow10_status_t;

bignum_init_pow10_status_t bignum_init_pow10(
    bignum_t *restrict dst,
    int k);
```

### Contract

| Condition | Result | Destination behavior |
|---|---|---|
| `dst != NULL` and `0 <= k <= 616` | `BIGNUM_INIT_POW10_SUCCESS` | Destination is fully initialized to `10^k` |
| `k == 0` | `BIGNUM_INIT_POW10_SUCCESS` | Normalized representation of one: `len == 1`, `words[0] == 1` |
| `dst == NULL` | `BIGNUM_INIT_POW10_ERROR_NULL_ARG` | No memory is accessed |
| `k < 0` | `BIGNUM_INIT_POW10_ERROR_INVALID_EXPONENT` | Destination remains unchanged |
| `k > 616` | `BIGNUM_INIT_POW10_ERROR_OVERFLOW` | Destination remains unchanged |

The representation is little-endian by 64-bit words. Zero is represented by `len == 0`, while every successful `pow10` result is nonzero and normalized with a nonzero highest word. The largest supported result is `10^616`, because `10^616 < 2^2048` and `10^617 > 2^2048`.

Example:

```c
#include "bignum_init_pow10.h"

bignum_t value;
bignum_init_pow10_status_t status = bignum_init_pow10(&value, 100);
if (status != BIGNUM_INIT_POW10_SUCCESS) {
    /* Handle invalid input or an unrepresentable result. */
}
```

All error checks occur before destination mutation. The function has no global mutable state and is thread-safe when separate destination objects are used by concurrent callers.

## Build and test

Build the release object and dependencies:

```bash
make build CONFIG=release
```

The production object is generated at `build/bignum_init_pow10.o`.

Run the complete deterministic, extended, multithreaded, and integration suite against ASM:

```bash
make test CONFIG=release
```

Expected summary:

```text
=== Summary: 0 / 4 failed ===
```

Run the independent C reference implementation:

```bash
make clean
make test CONFIG=release USE_ASM=no
```

Run static analysis and sanitizers:

```bash
make lint
make test_sanitize CONFIG=debug SAN=address
make test_sanitize CONFIG=debug SAN=undefined
make test_helgrind CONFIG=release
```

The tests are organized as follows:

| File | Scope |
|---|---|
| `tests/test_bignum_init_pow10.c` | Deterministic API, small powers, boundary, errors, canaries, and repeated initialization |
| `tests/test_bignum_init_pow10_extra.c` | Independent reference arithmetic for every exponent 0..616, error preservation, canaries, and 10,000-case fuzz sequence |
| `tests/test_bignum_init_pow10_mt.c` | Concurrent calls using independent destinations |
| `tests/test_bignum_init_pow10_runner.c` | Integration smoke test |

## Benchmarks

The benchmark sources are:

```text
benchmarks/bench_bignum_init_pow10.c
benchmarks/bench_bignum_init_pow10_mt.c
```

Both benchmarks generate a deterministic sequence of exponents. The `data-mode` controls that sequence: `all_zero` measures `k == 0`, `all_nonzero` generates reproducible exponents in `1..616`, and `mixed` alternates zero and nonzero cases. `seed`, `data-count`, and the selected mode determine the input fingerprint reported in the output.

### Single-thread CLI

```text
bin/bench_bignum_init_pow10 [--iterations N] [--warmup N]
                           [--data-count N] [--seed N]
                           [--data-mode all_zero|all_nonzero|mixed]
```

Example:

```bash
./bin/bench_bignum_init_pow10 \
  --iterations 1000000 \
  --warmup 10000 \
  --data-count 4096 \
  --seed 11400714819323198485 \
  --data-mode all_nonzero
```

### Multithread CLI

```text
bin/bench_bignum_init_pow10_mt [--threads N]
                               [--iterations N|--total-iterations N]
                               [--warmup N] [--data-count N] [--seed N]
                               [--data-mode all_zero|all_nonzero|mixed]
```

Examples:

```bash
./bin/bench_bignum_init_pow10_mt \
  --threads 2 --iterations 1000000 --warmup 10000 \
  --data-count 4096 --seed 11400714819323198485 \
  --data-mode all_nonzero

./bin/bench_bignum_init_pow10_mt \
  --threads 2 --total-iterations 2000000 --warmup 10000 \
  --data-count 4096 --seed 11400714819323198485 \
  --data-mode mixed
```

`--iterations` specifies measured iterations per thread in MT mode. `--total-iterations` specifies the aggregate count and must be divisible by the thread count. Warm-up calls are excluded from the measured count. The output is machine-readable and includes `successful`, `fingerprint`, `checksum`, `elapsed_seconds`, and `ns_per_call`. Benchmark comparisons must keep exponent distribution, iteration count, warm-up policy, seed, data count, CPU affinity, compiler configuration, and thread count constant.

## Perf workflow

The unchanged Makefile provides ST/MT recording and repeated counter targets. When `/usr/local/bin/perf` and the required permissions are available, run:

```bash
make bench_full CONFIG=release \
  REPORT_NAME=baseline \
  PERF_RUNS=5 \
  KEEP_PERF=1
```

For targeted measurements:

```bash
make bench_stat_st CONFIG=release REPORT_NAME=baseline_st PERF_RUNS=5
make bench_stat_mt CONFIG=release REPORT_NAME=baseline_mt MT_THREADS=2 MT_CPU_LIST=0-1 PERF_RUNS=5
```

The sandbox may not expose Linux performance counters; in that case, direct benchmark binaries remain valid for functional timing, while `perf` results should be collected on a permitted host.

## Installation and distribution

Build the object-file distribution:

```bash
make install CONFIG=release
```

Build the single-header and static-library distribution:

```bash
make dist CONFIG=release
```

Remove generated artifacts:

```bash
make clean
```

The Makefile is the official template file and must not be edited without explicit authorization.

## Linking the object file

```bash
make build CONFIG=release
gcc your_app.c build/bignum_init_pow10.o \
  -I./include -I./libs/bignum-core/include \
  -o your_app -no-pie
```

The application must use the same System V AMD64 ABI and link the typed dependency objects required by the selected distribution.

## Contributing

Contributions must preserve the typed status API, destination-preservation guarantee on errors, maximum exponent boundary, normalized representation, no-allocation property, and thread-safety contract. New behavior requires deterministic tests and independent reference checks where applicable. The official Makefile must remain unmodified.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
