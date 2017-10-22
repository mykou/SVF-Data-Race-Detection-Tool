
/*
 * RTLAtomic.h
 *
 *  Created on: 05/10/2016
 *      Author: pengd
 */

/*!
 * Atomic operations for runtime libraries.
 */

#ifndef RTL_ATOMIC_H
#define RTL_ATOMIC_H

#include "RTLInternalDefs.h"

namespace __rtl_common {

enum memory_order {
    memory_order_relaxed = 1 << 0,
    memory_order_consume = 1 << 1,
    memory_order_acquire = 1 << 2,
    memory_order_release = 1 << 3,
    memory_order_acq_rel = 1 << 4,
    memory_order_seq_cst = 1 << 5
};

/// Atomic types
//@{
struct atomic_uint8_t {
    typedef u8 Type;
    volatile Type val_dont_use;
};

struct atomic_uint16_t {
    typedef u16 Type;
    volatile Type val_dont_use;
};

struct atomic_uint32_t {
    typedef u32 Type;
    volatile Type val_dont_use;
};

struct atomic_uint64_t {
    typedef u64 Type;
    // On 32-bit platforms u64 is not necessary aligned on 8 bytes.
    volatile ALIGNED(8) Type val_dont_use;
};

struct atomic_uintptr_t {
    typedef uptr Type;
    volatile Type val_dont_use;
};
//@}

}  // namespace __rtl_common

#if defined(__clang__) || defined(__GNUC__)
# include "RTLAtomicClang.h"
#elif defined(_MSC_VER)
# include "RTLAtomicMsvc.h"
#else
# error "Unsupported compiler"
#endif

namespace __rtl_common {

/*!
 * Atomic relaxed load
 */
template<typename T>
INLINE typename T::Type atomic_load_relaxed(const volatile T *a) {
    return atomic_load(a, memory_order_relaxed);
}

/*!
 * Atomic relaxed store
 */
template<typename T>
INLINE void atomic_store_relaxed(volatile T *a, typename T::Type v) {
    atomic_store(a, v, memory_order_relaxed);
}

}  // namespace __rtl_common

#endif  // RTL_ATOMIC_H
