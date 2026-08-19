;/**
; * @file    bignum_init_pow10.asm
; * @author  git@bayborodov.com
; * @version 1.0.0
; * @date    2026-08-19
; *
; * @brief Ассемлерный код для реализации алгоритма bignum_init_pow10
; * @details <TBD Алгоритм функции bignum_init_pow10>
; *          Применяется для статической библиотеки libbignum_init_pow10.a 
; *          и обшей библиотеки bignum-lib
; * @history
; *   - rev. 0 (2026-08-19): Первоначальное создание файла
; */
section .text

    extern bignum_init_u64
    extern bignum_copy
    extern bignum_mul_bignum

; void bignum_init_pow10(bignum_t* bn, int k)
; rdi = bn, rsi = k
bignum_init_pow10:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    
    mov rbx, rdi ; rbx = bn
    mov r12d, esi ; r12d = k
    
    ; bignum_init_u64(bn, 1)
    mov rsi, 1
    call bignum_init_u64
    
    test r12d, r12d
    jz .done
    
    ; Место на стеке для bignum_t base, temp_b, temp_base
    ; 3 * 264 = 792. Выравнивание rbp(8) + 4*regs(32) = 40.
    ; 40 + 792 = 832. 832 / 16 = 52. Выровнено.
    sub rsp, 792
    lea r13, [rsp]       ; r13 = &base
    lea r14, [rsp + 264] ; r14 = &temp_b
    ; [rsp + 528] = &temp_base
    
    ; bignum_init_u64(&base, 10)
    mov rdi, r13
    mov rsi, 10
    call bignum_init_u64
    
.loop:
    test r12d, r12d
    jz .end_loop
    
    ; if (k % 2 == 1)
    test r12d, 1
    jz .skip_mul
    
    ; bignum_copy(&temp_b, bn)
    mov rdi, r14
    mov rsi, rbx
    call bignum_copy
    
    ; bignum_mul_bignum(bn, &temp_b, &base)
    mov rdi, rbx
    mov rsi, r14
    mov rdx, r13
    call bignum_mul_bignum
    
.skip_mul:
    shr r12d, 1
    jz .end_loop
    
    ; bignum_copy(&temp_base, &base)
    lea rdi, [rsp + 528]
    mov rsi, r13
    call bignum_copy
    
    ; bignum_mul_bignum(&base, &temp_base, &temp_base)
    mov rdi, r13
    lea rsi, [rsp + 528]
    mov rdx, rsi
    call bignum_mul_bignum
    jmp .loop

.end_loop:
    add rsp, 792
.done:
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret