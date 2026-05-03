/* Portable coroutine stack-swap for XEmacs async scheduler.
   Implements coro_swap.
   Based on the approach used by LuaJIT/libco. */

#if defined(__x86_64__)

/* struct coro_context { void *sp; }; — first field is saved stack pointer */

/* void coro_swap(coro_context *from, coro_context *to)
   rdi = from, rsi = to */
    .text
    .globl coro_swap
#if defined(__ELF__)
    .type  coro_swap, @function
#endif
coro_swap:
    /* Save callee-saved registers + return address onto current stack */
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    /* Save current stack pointer into from->sp */
    movq  %rsp, (%rdi)
    /* Restore stack pointer from to->sp */
    movq  (%rsi), %rsp
    /* Restore callee-saved registers */
    popq  %r15
    popq  %r14
    popq  %r13
    popq  %r12
    popq  %rbx
    popq  %rbp
    /* Return into the resumed coroutine */
    ret
#if defined(__ELF__)
    .size coro_swap, .-coro_swap
#endif

#elif defined(__aarch64__)

    .text
    .globl coro_swap
#if defined(__ELF__)
    .type  coro_swap, %function
#endif
coro_swap:
    /* Save callee-saved registers to current stack */
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x28, [sp, #-16]!
    stp x29, x30, [sp, #-16]!
    /* Save FP callee-saved registers d8-d15 */
    stp d8,  d9,  [sp, #-16]!
    stp d10, d11, [sp, #-16]!
    stp d12, d13, [sp, #-16]!
    stp d14, d15, [sp, #-16]!
    /* Save sp into from->sp */
    mov x2, sp
    str x2, [x0]
    /* Load sp from to->sp */
    ldr x2, [x1]
    mov sp, x2
    /* Restore FP callee-saved registers */
    ldp d14, d15, [sp], #16
    ldp d12, d13, [sp], #16
    ldp d10, d11, [sp], #16
    ldp d8,  d9,  [sp], #16
    /* Restore callee-saved registers */
    ldp x29, x30, [sp], #16
    ldp x27, x28, [sp], #16
    ldp x25, x26, [sp], #16
    ldp x23, x24, [sp], #16
    ldp x21, x22, [sp], #16
    ldp x19, x20, [sp], #16
    ret
#if defined(__ELF__)
    .size coro_swap, .-coro_swap
#endif

#else
/* Fallback: use ucontext_t on other architectures */
#endif
