/* ------------------------------------------------------------------ */
/**
 * @file    bench_bignum_init_pow10_mt.c
 * @brief   Multithread benchmark for bignum_init_pow10.
 * @version 0.1.0
 */
/* ------------------------------------------------------------------ */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
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
    uint64_t total_iterations;
    uint64_t warmup;
    size_t threads;
    size_t data_count;
    uint64_t seed;
    data_mode_t mode;
} options_t;

typedef struct {
    const uint32_t *data;
    const options_t *options;
    size_t thread_id;
    uint64_t checksum;
    uint64_t successful;
    int failed;
} worker_arg_t;

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
        .total_iterations = 0U,
        .warmup = UINT64_C(10000),
        .threads = 2U,
        .data_count = 4096U,
        .seed = UINT64_C(0x9E3779B97F4A7C15),
        .mode = DATA_ALL_NONZERO
    };

    for (int i = 1; i < argc; ++i) {
        uint64_t value;
        if (strcmp(argv[i], "--threads") == 0 ||
            strcmp(argv[i], "--iterations") == 0 ||
            strcmp(argv[i], "--total-iterations") == 0 ||
            strcmp(argv[i], "--warmup") == 0 ||
            strcmp(argv[i], "--data-count") == 0 ||
            strcmp(argv[i], "--seed") == 0) {
            if (i + 1 >= argc || parse_u64(argv[++i], &value) != 0) return -1;
            if (strcmp(argv[i - 1], "--threads") == 0) options->threads = (size_t)value;
            else if (strcmp(argv[i - 1], "--iterations") == 0) options->iterations = value;
            else if (strcmp(argv[i - 1], "--total-iterations") == 0) options->total_iterations = value;
            else if (strcmp(argv[i - 1], "--warmup") == 0) options->warmup = value;
            else if (strcmp(argv[i - 1], "--data-count") == 0) options->data_count = (size_t)value;
            else options->seed = value;
        } else if (strcmp(argv[i], "--data-mode") == 0) {
            if (i + 1 >= argc || parse_mode(argv[++i], &options->mode) != 0) return -1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: %s [--threads N] [--iterations N|--total-iterations N] [--warmup N] [--data-count N] [--seed N] [--data-mode all_zero|all_nonzero|mixed]\n", argv[0]);
            exit(EXIT_SUCCESS);
        } else {
            return -1;
        }
    }
    if (options->threads == 0U || options->threads > 128U ||
        options->data_count == 0U) return -1;
    if (options->total_iterations != 0U) {
        if (options->total_iterations % options->threads != 0U) return -1;
        options->iterations = options->total_iterations / options->threads;
    }
    return options->iterations != 0U ? 0 : -1;
}

static void fill_data(uint32_t *data, const options_t *options)
{
    uint64_t state = options->seed;
    for (size_t i = 0; i < options->data_count; ++i) {
        int zero = options->mode == DATA_ALL_ZERO ||
                   (options->mode == DATA_MIXED && (i % 2U) == 0U);
        data[i] = zero ? 0U : (uint32_t)(next_value(&state) % 617U);
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

static void *worker(void *opaque)
{
    worker_arg_t *arg = opaque;
    bignum_t result;
    uint64_t total = arg->options->warmup + arg->options->iterations;
    for (uint64_t i = 0; i < total; ++i) {
        uint32_t exponent = arg->data[(i + arg->thread_id) % arg->options->data_count];
        if (bignum_init_pow10(&result, (int)exponent) != BIGNUM_INIT_POW10_SUCCESS) {
            arg->failed = 1;
            return NULL;
        }
        if (i >= arg->options->warmup) {
            arg->checksum ^= result.words[0] + (uint64_t)result.len + i;
            ++arg->successful;
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    options_t options;
    uint32_t *data;
    pthread_t *threads;
    worker_arg_t *args;
    struct timespec start;
    struct timespec end;
    uint64_t checksum = 0U;
    uint64_t successful = 0U;
    uint64_t total_iterations;
    double elapsed;

    if (parse_options(argc, argv, &options) != 0) {
        fprintf(stderr, "invalid benchmark arguments; use --help\n");
        return EXIT_FAILURE;
    }
    data = calloc(options.data_count, sizeof(*data));
    threads = calloc(options.threads, sizeof(*threads));
    args = calloc(options.threads, sizeof(*args));
    if (data == NULL || threads == NULL || args == NULL) {
        perror("calloc");
        free(data);
        free(threads);
        free(args);
        return EXIT_FAILURE;
    }
    fill_data(data, &options);
    total_iterations = options.iterations * (uint64_t)options.threads;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < options.threads; ++i) {
        args[i] = (worker_arg_t){
            .data = data,
            .options = &options,
            .thread_id = i,
            .checksum = 0U,
            .successful = 0U,
            .failed = 0
        };
        if (pthread_create(&threads[i], NULL, worker, &args[i]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            free(data);
            free(threads);
            free(args);
            return EXIT_FAILURE;
        }
    }
    for (size_t i = 0; i < options.threads; ++i) {
        if (pthread_join(threads[i], NULL) != 0 || args[i].failed) {
            fprintf(stderr, "worker failed\n");
            free(data);
            free(threads);
            free(args);
            return EXIT_FAILURE;
        }
        checksum ^= args[i].checksum;
        successful += args[i].successful;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = seconds_between(start, end);

    printf("benchmark=bignum_init_pow10_mt data_mode=%s seed=%" PRIu64
           " threads=%zu iterations_per_thread=%" PRIu64
           " total_iterations=%" PRIu64 " warmup=%" PRIu64
           " data_count=%zu successful=%" PRIu64
           " fingerprint=%" PRIu64 " checksum=%" PRIu64
           " elapsed_seconds=%.9f ns_per_call=%.3f\n",
           mode_name(options.mode), options.seed, options.threads,
           options.iterations, total_iterations, options.warmup,
           options.data_count, successful, fingerprint(data, &options),
           checksum, elapsed, elapsed * 1000000000.0 / (double)total_iterations);
    free(data);
    free(threads);
    free(args);
    return EXIT_SUCCESS;
}
