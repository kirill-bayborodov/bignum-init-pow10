; -----------------------------------------------------------------------------
; @file    bignum_init_pow10.asm
; @brief   x86-64 YASM implementation of bignum_init_pow10.
; @version 0.1.0
; @details System V AMD64 ABI: rdi = dst, esi = int k.
;          Computes 10^k by binary exponentiation using typed dependencies.
; @history
;   - rev. 0.1.0 (2026-08-19): Initial typed implementation.
; -----------------------------------------------------------------------------
; SPDX-License-Identifier: MIT
; -----------------------------------------------------------------------------
default rel
section .text
    align 16
    global bignum_init_pow10
    extern bignum_init_u64
    extern bignum_copy
    extern bignum_mul_bignum

BIGNUM_CAPACITY                    equ 32
BIGNUM_SIZE                        equ 264
BIGNUM_OFFSET_LEN                  equ 256
BIGNUM_INIT_POW10_MAX_EXPONENT     equ 616
RET_SUCCESS                        equ 0
RET_ERROR_NULL_ARG                 equ -1
RET_ERROR_INVALID_EXPONENT         equ -2
RET_ERROR_OVERFLOW                 equ -3

; bignum_init_pow10_status_t bignum_init_pow10(bignum_t *dst, int k)
bignum_init_pow10:
    test    rdi, rdi
    jz      .error_null
    test    esi, esi
    js      .error_invalid
    cmp     esi, BIGNUM_INIT_POW10_MAX_EXPONENT
    jg      .error_overflow

    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, BIGNUM_SIZE * 3

    mov     rbx, rdi
    mov     r12d, esi
    mov     r13, rsp

    mov     rdi, r13
    mov     esi, 1
    call    bignum_init_u64
    test    eax, eax
    jnz     .error_stack

.loop:
    test    r12d, r12d
    jz      .finish
    xor     r8d, r8d                   ; carry
    xor     ecx, ecx                   ; word index
.word_loop:
    cmp     rcx, [r13 + BIGNUM_OFFSET_LEN]
    jae     .words_done
    mov     rax, [r13 + rcx * 8]
    mov     r10d, 10
    mul     r10                         ; rdx:rax = word * 10
    add     rax, r8
    adc     rdx, 0
    mov     [r13 + rcx * 8], rax
    mov     r8, rdx
    inc     rcx
    jmp     .word_loop
.words_done:
    test    r8, r8
    jz      .decrement
    cmp     rcx, BIGNUM_CAPACITY
    jae     .error_stack
    mov     [r13 + rcx * 8], r8
    inc     rcx
    mov     [r13 + BIGNUM_OFFSET_LEN], rcx
.decrement:
    dec     r12d
    jmp     .loop

.finish:
    mov     rdi, rbx
    xor     esi, esi
    call    bignum_init_u64
    test    eax, eax
    jnz     .error_stack
    mov     rdi, rbx
    mov     rsi, r13
    call    bignum_copy
    test    eax, eax
    jnz     .error_stack
    xor     eax, eax
    jmp     .epilogue

.error_stack:
    mov     eax, RET_ERROR_OVERFLOW
.epilogue:
    add     rsp, BIGNUM_SIZE * 3
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret

.error_null:
    mov     eax, RET_ERROR_NULL_ARG
    ret
.error_invalid:
    mov     eax, RET_ERROR_INVALID_EXPONENT
    ret
.error_overflow:
    mov     eax, RET_ERROR_OVERFLOW
    ret
