/* ------------------------------------------------------------------ */
/**
 * @file    bench_bignum_init_pow10.c
 * @brief   Single-thread benchmark for bignum_init_pow10.
 * @version 0.1.0
 */
/* ------------------------------------------------------------------ */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bignum_init_pow10.h"

typedef enum {
    DATA_ALL_ZERO,
    DATA_ALL_NONZERO,
    DATA_MIXED
} data_mode_t;

typedef struct {
    uint64_t iterations;
    uint64_t warmup;
    size_t data_count;
    uint64_t seed;
    data_mode_t mode;
} options_t;

static uint64_t next_value(uint64_t *state)
{
    *state ^= *state << 7;
    *state ^= *state >> 9;
    *state ^= *state << 8;
    return *state;
}

static const char *mode_name(data_mode_t mode)
{
    switch (mode) {
    case DATA_ALL_ZERO: return "all_zero";
    case DATA_ALL_NONZERO: return "all_nonzero";
    case DATA_MIXED: return "mixed";
    }
    return "unknown";
}

static int parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;
    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') return -1;
    *value = (uint64_t)parsed;
    return 0;
}

static int parse_mode(const char *text, data_mode_t *mode)
{
    if (strcmp(text, "all_zero") == 0) *mode = DATA_ALL_ZERO;
    else if (strcmp(text, "all_nonzero") == 0) *mode = DATA_ALL_NONZERO;
    else if (strcmp(text, "mixed") == 0) *mode = DATA_MIXED;
    else return -1;
    return 0;
}

static int parse_options(int argc, char **argv, options_t *options)
{
    *options = (options_t){
        .iterations = UINT64_C(1000000),
        .warmup = UINT64_C(10000),
        .data_count = 4096U,
        .seed = UINT64_C(0x9E3779B97F4A7C15),
        .mode = DATA_ALL_NONZERO
    };

    for (int i = 1; i < argc; ++i) {
        uint64_t value;
        if (strcmp(argv[i], "--iterations") == 0 ||
            strcmp(argv[i], "--warmup") == 0 ||
            strcmp(argv[i], "--data-count") == 0 ||
            strcmp(argv[i], "--seed") == 0) {
            if (i + 1 >= argc || parse_u64(argv[++i], &value) != 0) return -1;
            if (strcmp(argv[i - 1], "--iterations") == 0) options->iterations = value;
            else if (strcmp(argv[i - 1], "--warmup") == 0) options->warmup = value;
            else if (strcmp(argv[i - 1], "--data-count") == 0) options->data_count = (size_t)value;
            else options->seed = value;
        } else if (strcmp(argv[i], "--data-mode") == 0) {
            if (i + 1 >= argc || parse_mode(argv[++i], &options->mode) != 0) return -1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [--iterations N] [--warmup N] [--data-count N] [--seed N] [--data-mode all_zero|all_nonzero|mixed]\n", argv[0]);
            exit(EXIT_SUCCESS);
        } else {
            return -1;
        }
    }
    return options->iterations != 0U && options->data_count != 0U ? 0 : -1;
}

static void fill_data(uint32_t *data, const options_t *options)
{
    uint64_t state = options->seed;
    for (size_t i = 0; i < options->data_count; ++i) {
        int zero = options->mode == DATA_ALL_ZERO ||
                   (options->mode == DATA_MIXED && (i % 2U) == 0U);
        if (zero) data[i] = 0U;
        else data[i] = (uint32_t)(next_value(&state) % 617U);
    }
}

static uint64_t fingerprint(const uint32_t *data, const options_t *options)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < options->data_count; ++i) {
        hash ^= data[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static double seconds_between(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

int main(int argc, char **argv)
{
    options_t options;
    uint32_t *data;
    bignum_t result;
    uint64_t checksum = 0U;
    uint64_t successful = 0U;
    struct timespec start;
    struct timespec end;
    double elapsed;

    if (parse_options(argc, argv, &options) != 0) {
        fprintf(stderr, "invalid benchmark arguments; use --help\n");
        return EXIT_FAILURE;
    }
    data = calloc(options.data_count, sizeof(*data));
    if (data == NULL) {
        perror("calloc");
        return EXIT_FAILURE;
    }
    fill_data(data, &options);

    for (uint64_t i = 0; i < options.warmup; ++i) {
        if (bignum_init_pow10(&result, (int)data[i % options.data_count]) !=
            BIGNUM_INIT_POW10_SUCCESS) {
            free(data);
            return EXIT_FAILURE;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (uint64_t i = 0; i < options.iterations; ++i) {
        if (bignum_init_pow10(&result, (int)data[i % options.data_count]) !=
            BIGNUM_INIT_POW10_SUCCESS) {
            free(data);
            return EXIT_FAILURE;
        }
        checksum ^= result.words[0] + (uint64_t)result.len + i;
        ++successful;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = seconds_between(start, end);

    printf("benchmark=bignum_init_pow10_st data_mode=%s seed=%" PRIu64
           " data_count=%zu iterations=%" PRIu64 " warmup=%" PRIu64
           " successful=%" PRIu64 " fingerprint=%" PRIu64
           " checksum=%" PRIu64 " elapsed_seconds=%.9f ns_per_call=%.3f\n",
           mode_name(options.mode), options.seed, options.data_count,
           options.iterations, options.warmup, successful,
           fingerprint(data, &options), checksum, elapsed,
           elapsed * 1000000000.0 / (double)options.iterations);
    free(data);
    return EXIT_SUCCESS;
}
