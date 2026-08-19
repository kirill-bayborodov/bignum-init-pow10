/* ------------------------------------------------------------------ */
/** @file test_bignum_init_pow10_extra.c @brief Extended pow10 tests. */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bignum_init_pow10.h"

static void reference_mul10(bignum_t *b)
{
    uint64_t carry = 0U;
    for (size_t i = 0; i < b->len; ++i) {
        __uint128_t p = (__uint128_t)b->words[i] * 10U + carry;
        b->words[i] = (uint64_t)p;
        carry = (uint64_t)(p >> 64U);
    }
    if (carry != 0U) {
        assert(b->len < BIGNUM_CAPACITY);
        b->words[b->len++] = carry;
    }
}

static void test_all_representable_exponents(void)
{
    bignum_t expected;
    bignum_t actual;
    memset(&expected, 0, sizeof(expected));
    expected.len = 1U;
    expected.words[0] = 1U;

    for (int k = 0; k <= 616; ++k) {
        memset(&actual, 0xCC, sizeof(actual));
        assert(bignum_init_pow10(&actual, k) == BIGNUM_INIT_POW10_SUCCESS);
        assert(memcmp(&actual, &expected, sizeof(actual)) == 0);
        if (k != 616) {
            reference_mul10(&expected);
        }
    }
    puts("test_all_representable_exponents: PASSED");
}

static void test_error_matrix_and_canary(void)
{
    struct {
        unsigned char left;
        bignum_t value;
        unsigned char right;
    } object;
    memset(&object, 0xA6, sizeof(object));
    bignum_t before = object.value;

    assert(bignum_init_pow10(&object.value, -100) ==
           BIGNUM_INIT_POW10_ERROR_INVALID_EXPONENT);
    assert(memcmp(&object.value, &before, sizeof(before)) == 0);
    assert(object.left == 0xA6 && object.right == 0xA6);

    assert(bignum_init_pow10(&object.value, 10000) ==
           BIGNUM_INIT_POW10_ERROR_OVERFLOW);
    assert(memcmp(&object.value, &before, sizeof(before)) == 0);
    assert(object.left == 0xA6 && object.right == 0xA6);
    puts("test_error_matrix_and_canary: PASSED");
}

static void test_repeated_fuzz_sequence(void)
{
    bignum_t b;
    for (unsigned i = 0; i < 10000U; ++i) {
        int k = (int)((i * 37U) % 617U);
        assert(bignum_init_pow10(&b, k) == BIGNUM_INIT_POW10_SUCCESS);
        assert(b.len > 0U && b.len <= BIGNUM_CAPACITY);
        assert(b.words[b.len - 1U] != 0U);
    }
    puts("test_repeated_fuzz_sequence: PASSED");
}

int main(void)
{
    puts("--- Starting extended bignum_init_pow10 tests ---");
    test_all_representable_exponents();
    test_error_matrix_and_canary();
    test_repeated_fuzz_sequence();
    puts("--- All extended bignum_init_pow10 tests passed ---");
    return 0;
}
