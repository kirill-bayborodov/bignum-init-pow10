/* ------------------------------------------------------------------ */
/** @file test_bignum_init_pow10.c @brief Deterministic pow10 tests. */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bignum_init_pow10.h"

static void test_small_powers(void)
{
    bignum_t b;
    uint64_t expected = 1U;
    for (int k = 0; k <= 18; ++k) {
        memset(&b, 0xA5, sizeof(b));
        assert(bignum_init_pow10(&b, k) == BIGNUM_INIT_POW10_SUCCESS);
        assert(b.len == 1U);
        assert(b.words[0] == expected);
        for (size_t i = 1; i < BIGNUM_CAPACITY; ++i) {
            assert(b.words[i] == 0U);
        }
        expected *= 10U;
    }
    puts("test_small_powers: PASSED");
}

static void test_zero_and_boundary(void)
{
    bignum_t b;
    assert(bignum_init_pow10(&b, 0) == BIGNUM_INIT_POW10_SUCCESS);
    assert(b.len == 1U && b.words[0] == 1U);
    assert(bignum_init_pow10(&b, 616) == BIGNUM_INIT_POW10_SUCCESS);
    assert(b.len > 0U && b.len <= BIGNUM_CAPACITY);
    assert(b.words[b.len - 1U] != 0U);
    puts("test_zero_and_boundary: PASSED");
}

static void test_invalid_arguments_preserve_destination(void)
{
    bignum_t before;
    bignum_t after;
    memset(&before, 0x5A, sizeof(before));
    after = before;

    assert(bignum_init_pow10(NULL, 1) == BIGNUM_INIT_POW10_ERROR_NULL_ARG);
    assert(bignum_init_pow10(&after, -1) ==
           BIGNUM_INIT_POW10_ERROR_INVALID_EXPONENT);
    assert(memcmp(&after, &before, sizeof(after)) == 0);
    assert(bignum_init_pow10(&after, 617) == BIGNUM_INIT_POW10_ERROR_OVERFLOW);
    assert(memcmp(&after, &before, sizeof(after)) == 0);
    puts("test_invalid_arguments_preserve_destination: PASSED");
}

static void test_repeated_initialization(void)
{
    bignum_t b;
    assert(bignum_init_pow10(&b, 3) == BIGNUM_INIT_POW10_SUCCESS);
    assert(b.words[0] == 1000U);
    assert(bignum_init_pow10(&b, 1) == BIGNUM_INIT_POW10_SUCCESS);
    assert(b.len == 1U && b.words[0] == 10U);
    puts("test_repeated_initialization: PASSED");
}

int main(void)
{
    puts("--- Starting deterministic bignum_init_pow10 tests ---");
    test_small_powers();
    test_zero_and_boundary();
    test_invalid_arguments_preserve_destination();
    test_repeated_initialization();
    puts("--- All deterministic bignum_init_pow10 tests passed ---");
    return 0;
}
