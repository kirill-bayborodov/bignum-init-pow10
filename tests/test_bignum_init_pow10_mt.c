/* ------------------------------------------------------------------ */
/** @file test_bignum_init_pow10_mt.c @brief Multithreaded pow10 tests. */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

#include "bignum_init_pow10.h"

typedef struct {
    size_t id;
    int failed;
} worker_data_t;

static void *worker(void *arg)
{
    worker_data_t *data = arg;
    bignum_t value;
    for (size_t iteration = 0; iteration < 10000U; ++iteration) {
        int k = (int)((iteration + data->id * 13U) % 617U);
        if (bignum_init_pow10(&value, k) != BIGNUM_INIT_POW10_SUCCESS ||
            value.len == 0U || value.len > BIGNUM_CAPACITY ||
            value.words[value.len - 1U] == 0U) {
            data->failed = 1;
            return NULL;
        }
    }
    return NULL;
}

int main(void)
{
    enum { THREAD_COUNT = 8 };
    pthread_t threads[THREAD_COUNT];
    worker_data_t data[THREAD_COUNT];
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        data[i].id = i;
        data[i].failed = 0;
        assert(pthread_create(&threads[i], NULL, worker, &data[i]) == 0);
    }
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        assert(pthread_join(threads[i], NULL) == 0);
        assert(data[i].failed == 0);
    }
    puts("--- Multithreaded bignum_init_pow10 test passed ---");
    return 0;
}
