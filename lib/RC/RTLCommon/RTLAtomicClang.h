/*
 * RTLAtomicClang.h
 *
 *  Created on: 05/10/2016
 *      Author: pengd
 */

/*!
 * \Atomic operations for runtime libraries.
 */

#ifndef RTL_ATOMIC_CLANG_H

#define RTL_ATOMIC_CLANG_H

#if defined(__i386__) || defined(__x86_64__)
# include "RTLAtomicClang_x86.h"
#else
# include "RTLAtomicClang_other.h"
#endif

namespace __rtl_common {

/*!
 * Atomic signal fence
 */
INLINE void atomic_signal_fence(memory_order) {
    __asm__ __volatile__("" ::: "memory");
}

/*!
 * Atomic thread fence
 */
INLINE void atomic_thread_fence(memory_order) {
    __sync_synchronize();
}

/*!
 * Atomic fetch and add
 */
template<typename T>
INLINE typename T::Type atomic_fetch_add(volatile T *a,
        typename T::Type v, memory_order mo) {
    (void)mo;
    DCHECK(!((uptr)a % sizeof(*a)));
    return __sync_fetch_and_add(&a->val_dont_use, v);
}

/*!
 * Atomic fetch and subtract
 */
template<typename T>
INLINE typename T::Type atomic_fetch_sub(volatile T *a,
        typename T::Type v, memory_order mo) {
    (void)mo;
    DCHECK(!((uptr)a % sizeof(*a)));
    return __sync_fetch_and_add(&a->val_dont_use, -v);
}

/// Atomic exchange
//{@
template<typename T>
INLINE typename T::Type atomic_exchange(volatile T *a,
                                        typename T::Type v, memory_order mo) {
    DCHECK(!((uptr)a % sizeof(*a)));
    if (mo & (memory_order_release | memory_order_acq_rel | memory_order_seq_cst))
        __sync_synchronize();
    v = __sync_lock_test_and_set(&a->val_dont_use, v);
    if (mo == memory_order_seq_cst)
        __sync_synchronize();
    return v;
}

template<typename T>
INLINE bool atomic_compare_exchange_strong(volatile T *a,
        typename T::Type *cmp,
        typename T::Type xchg,
        memory_order mo) {
    typedef typename T::Type Type;
    Type cmpv = *cmp;
    Type prev = __sync_val_compare_and_swap(&a->val_dont_use, cmpv, xchg);
    if (prev == cmpv)
        return true;
    *cmp = prev;
    return false;
}

template<typename T>
INLINE bool atomic_compare_exchange_weak(volatile T *a,
        typename T::Type *cmp,
        typename T::Type xchg,
        memory_order mo) {
    return atomic_compare_exchange_strong(a, cmp, xchg, mo);
}
//@}

}  // namespace __rtl_common

#undef ATOMIC_ORDER

#endif  // RTL_ATOMIC_CLANG_H
