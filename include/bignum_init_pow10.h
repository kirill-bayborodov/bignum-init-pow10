/* ------------------------------------------------------------------ */
/**
 * @file    bignum_init_pow10.h
 * @brief   Инициализация bignum_t значением 10^k.
 * @version 0.1.0
 * @details
 *   Реализация рассчитана на нормализованный 2048-битный bignum_t.
 *   Revision 0.1.0: typed API and initial contract.
 */
/* ------------------------------------------------------------------ */
#pragma once
#ifndef BIGNUM_INIT_POW10_H
#define BIGNUM_INIT_POW10_H

#include <stdint.h>
#include "bignum.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Результаты выполнения bignum_init_pow10. */
typedef enum {
    BIGNUM_INIT_POW10_SUCCESS              = 0,
    BIGNUM_INIT_POW10_ERROR_NULL_ARG       = -1,
    BIGNUM_INIT_POW10_ERROR_INVALID_EXPONENT = -2,
    BIGNUM_INIT_POW10_ERROR_OVERFLOW       = -3
} bignum_init_pow10_status_t;

/**
 * @brief Инициализирует bignum_t значением 10 в степени k.
 *
 * При успешном вызове `dst` полностью перезаписывается нормализованным
 * представлением 10^k. Поддерживаются k >= 0; для k == 0 результат равен 1.
 * Для 2048-битной ёмкости максимальная представимая десятичная степень равна
 * 616. Отрицательная степень является ошибкой входа, а степень, результат
 * которой не помещается в BIGNUM_CAPACITY слов, возвращает overflow.
 *
 * При любой ошибке `dst` не изменяется. Аргумент dst не должен быть NULL.
 * Функция не выделяет динамическую память и потокобезопасна при независимых
 * объектах назначения.
 *
 * @param[out] dst Объект, который следует инициализировать.
 * @param[in]  k   Неотрицательная десятичная степень.
 * @return Статус операции.
 */
bignum_init_pow10_status_t bignum_init_pow10(
    bignum_t *restrict dst,
    int k);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_INIT_POW10_H */

/* SPDX-License-Identifier: MIT */
