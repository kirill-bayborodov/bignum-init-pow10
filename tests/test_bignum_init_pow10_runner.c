/* ------------------------------------------------------------------ */
/** @file test_bignum_init_pow10_runner.c @brief Integration smoke test. */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <stdio.h>

#include "bignum_init_pow10.h"

int main(void)
{
    bignum_t value;
    printf("Running test: test_bignum_init_pow10_runner... ");

    assert(bignum_init_pow10(&value, 0) == BIGNUM_INIT_POW10_SUCCESS);
    assert(value.len == 1U && value.words[0] == 1U);
    assert(bignum_init_pow10(&value, 1) == BIGNUM_INIT_POW10_SUCCESS);
    assert(value.len == 1U && value.words[0] == 10U);
    assert(bignum_init_pow10(&value, 10) == BIGNUM_INIT_POW10_SUCCESS);
    assert(value.len == 1U && value.words[0] == 10000000000U);
    assert(bignum_init_pow10(&value, -1) ==
           BIGNUM_INIT_POW10_ERROR_INVALID_EXPONENT);
    assert(bignum_init_pow10(&value, 617) ==
           BIGNUM_INIT_POW10_ERROR_OVERFLOW);
    assert(bignum_init_pow10(NULL, 1) == BIGNUM_INIT_POW10_ERROR_NULL_ARG);

    puts("PASSED");
    return 0;
}
