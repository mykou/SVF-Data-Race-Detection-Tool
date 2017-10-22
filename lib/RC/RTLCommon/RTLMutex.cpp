/*
 * RTLMutex.cpp
 *
 *  Created on: 25 Jul 2016
 *      Author: pengd
 */

#include "RTLMutex.h"

//using namespace __rtl_common;

enum MutexState {
    MtxUnlocked = 0,
    MtxLocked = 1,
    MtxSleeping = 2
};


/*!
 * internal memset
 */
void *internal_memset(void* s, int c, uptr n) {
    // The next line prevents Clang from making a call to memset() instead of the
    // loop below.
    char volatile *t = (char*)s;
    for (uptr i = 0; i < n; ++i, ++t) {
        *t = c;
    }
    return s;
}

/*
 * Blocking mutex constructor
 */
BlockingMutex::BlockingMutex() {
    internal_memset(this, 0, sizeof(*this));
}

/*
 * Blocking mutex lock
 */
void BlockingMutex::Lock() {
    CHECK_EQ(owner_, 0);
    atomic_uint32_t *m = reinterpret_cast<atomic_uint32_t *>(&opaque_storage_);
    if (atomic_exchange(m, MtxLocked, memory_order_acquire) == MtxUnlocked)
        return;
    while (atomic_exchange(m, MtxSleeping, memory_order_acquire) != MtxUnlocked) {
        u64 retval;
        asm volatile("mov %5, %%r10;"
                     "mov %6, %%r8;"
                     "mov %7, %%r9;"
                     "syscall" : "=a"(retval) : "a"(__NR_futex), "D"((u64)m),
                     "S"((u64)0), "d"((u64)MtxSleeping), "r"((u64)0), "r"((u64)0),
                     "r"((u64)0) : "rcx", "r11", "r10", "r8", "r9",
                     "memory", "cc");
    }
}

/*
 * Blocking mutex unlock
 */
void BlockingMutex::Unlock() {
    atomic_uint32_t *m = reinterpret_cast<atomic_uint32_t *>(&opaque_storage_);
    u32 v = atomic_exchange(m, MtxUnlocked, memory_order_relaxed);
    CHECK_NE(v, MtxUnlocked);
    if (v == MtxSleeping) {
        u64 retval;
        asm volatile("mov %5, %%r10;"
                     "mov %6, %%r8;"
                     "mov %7, %%r9;"
                     "syscall" : "=a"(retval) : "a"(__NR_futex), "D"((u64)m),
                     "S"((u64)1), "d"((u64)1), "r"((u64)0), "r"((u64)0),
                     "r"((u64)0) : "rcx", "r11", "r10", "r8", "r9",
                     "memory", "cc");
    }
}

/*
 * Blocking mutex check lock
 */
void BlockingMutex::CheckLocked() {
    atomic_uint32_t *m = reinterpret_cast<atomic_uint32_t *>(&opaque_storage_);
    CHECK_NE(MtxUnlocked, atomic_load(m, memory_order_relaxed));
}

