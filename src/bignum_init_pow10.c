/* ------------------------------------------------------------------ */
/**
 * @file    bignum_init_pow10.c
 * @brief   Переносимая эталонная реализация bignum_init_pow10.
 * @version 0.1.0
 * @history
 *   - rev. 0.1.0 (2026-08-19): Initial typed implementation.
 */
/* ------------------------------------------------------------------ */
#include <stddef.h>
#include <stdint.h>

#include "bignum_init_pow10.h"
#include "bignum_init_u64.h"
#include "bignum_copy.h"
#include "bignum_mul_bignum.h"

#define BIGNUM_INIT_POW10_MAX_EXPONENT 616

bignum_init_pow10_status_t bignum_init_pow10(
    bignum_t *restrict dst,
    int k)
{
    if (dst == NULL) {
        return BIGNUM_INIT_POW10_ERROR_NULL_ARG;
    }
    if (k < 0) {
        return BIGNUM_INIT_POW10_ERROR_INVALID_EXPONENT;
    }
    if (k > BIGNUM_INIT_POW10_MAX_EXPONENT) {
        return BIGNUM_INIT_POW10_ERROR_OVERFLOW;
    }

    bignum_t result;
    if (bignum_init_u64(&result, 1U) != BIGNUM_INIT_U64_SUCCESS) {
        return BIGNUM_INIT_POW10_ERROR_OVERFLOW;
    }

    for (int exponent = 0; exponent < k; ++exponent) {
        uint64_t carry = 0U;
        for (size_t i = 0; i < result.len; ++i) {
            __uint128_t product = (__uint128_t)result.words[i] * 10U + carry;
            result.words[i] = (uint64_t)product;
            carry = (uint64_t)(product >> 64U);
        }
        if (carry != 0U) {
            if (result.len == BIGNUM_CAPACITY) {
                return BIGNUM_INIT_POW10_ERROR_OVERFLOW;
            }
            result.words[result.len++] = carry;
        }
    }

    if (bignum_init_u64(dst, 0U) != BIGNUM_INIT_U64_SUCCESS) {
        return BIGNUM_INIT_POW10_ERROR_OVERFLOW;
    }
    for (size_t i = 0; i < result.len; ++i) {
        dst->words[i] = result.words[i];
    }
    dst->len = result.len;
    return BIGNUM_INIT_POW10_SUCCESS;
}

/* SPDX-License-Identifier: MIT */
