/*
 * RTLMutex.h
 *
 *  Created on: 05/10/2016
 *      Author: pengd
 */

/*!
 * \Machinery of mutex and lock for runtime library.
 * Because all system mutex opterations are hacked by interceptor,
 * new mutex operations are needed.
 */

#ifndef RTL_MUTEX_H
#define RTL_MUTEX_H

#include "RTLAtomic.h"
#include "RTLInternalDefs.h"
#include <asm/unistd.h>

#define SYSCALL(name) __NR_ ## name

namespace __rtl_common {

/*!
 * Internal sched yield function
 */
INLINE uptr internal_sched_yield() {
    u64 retval;
    asm volatile("syscall" : "=a"(retval) : "a"(SYSCALL(sched_yield)) : "rcx", "r11",
                 "memory", "cc");
    return retval;
}

/*!
 * Static Spin Mutex
 */
class StaticSpinMutex {
public:

    /*!
     * Initialization
     */
    void Init() {
        atomic_store(&state_, 0, memory_order_relaxed);
    }

    /*!
     * Lock
     */
    void Lock() {
        if (TryLock())
            return;
        LockSlow();
    }

    /*!
     * TryLock
     */
    bool TryLock() {
        return atomic_exchange(&state_, 1, memory_order_acquire) == 0;
    }

    /*!
     * Unlock
     */
    void Unlock() {
        atomic_store(&state_, 0, memory_order_release);
    }

    /*!
     * Check lock
     */
    void CheckLocked() {
        CHECK_EQ(atomic_load(&state_, memory_order_relaxed), 1);
    }

private:
    atomic_uint8_t state_;

    /*!
     * Lock slow
     */
    void NOINLINE LockSlow() {
        for (int i = 0;; i++) {
            if (i < 10)
                proc_yield(10);
            else
                internal_sched_yield();

            if (atomic_load(&state_, memory_order_relaxed) == 0
                    && atomic_exchange(&state_, 1, memory_order_acquire) == 0)
                return;
        }
    }
};

/*!
 * Spin mutex
 */
class SpinMutex : public StaticSpinMutex {
public:
    /*!
     * Constructor
     */
    SpinMutex() {
        Init();
    }

private:
    /*!
     * Constructor
     */
    SpinMutex(const SpinMutex&);

    /*!
     * Overload operator =
     */
    void operator=(const SpinMutex&);
};

/*!
 * Blocking mutex
 */
class BlockingMutex {
public:
#if RTL_WINDOWS
    // Windows does not currently support LinkerInitialized
    explicit BlockingMutex(LinkerInitialized);
#else
    explicit constexpr BlockingMutex(LinkerInitialized)
        : opaque_storage_ {0, }, owner_(0) {}
#endif
    /*!
     * Constructor
     */
    BlockingMutex();

    /*!
     * Lock
     */
    void Lock();

    /*!
     * Unlock
     */
    void Unlock();

    /*!
     * Check lock
     */
    void CheckLocked();
private:
    uptr opaque_storage_[10];
    uptr owner_;  // for debugging
};

/*!
 * Reader-writer spin mutex.
 */
class RWMutex {
public:
    /*!
     * Constructor
     */
    RWMutex() {
        atomic_store(&state_, kUnlocked, memory_order_relaxed);
    }

    /*!
     * Destructor
     */
    ~RWMutex() {
        CHECK_EQ(atomic_load(&state_, memory_order_relaxed), kUnlocked);
    }

    /*!
     * Lock
     */
    void Lock() {
        u32 cmp = kUnlocked;
        if (atomic_compare_exchange_strong(&state_, &cmp, kWriteLock,
                                           memory_order_acquire))
            return;
        LockSlow();
    }

    /*!
     * Unlock
     */
    void Unlock() {
        u32 prev = atomic_fetch_sub(&state_, kWriteLock, memory_order_release);
        DCHECK_NE(prev & kWriteLock, 0);
        (void)prev;
    }

    /*!
     * Read lock
     */
    void ReadLock() {
        u32 prev = atomic_fetch_add(&state_, kReadLock, memory_order_acquire);
        if ((prev & kWriteLock) == 0)
            return;
        ReadLockSlow();
    }

    /*!
     * Read unlock
     */
    void ReadUnlock() {
        u32 prev = atomic_fetch_sub(&state_, kReadLock, memory_order_release);
        DCHECK_EQ(prev & kWriteLock, 0);
        DCHECK_GT(prev & ~kWriteLock, 0);
        (void)prev;
    }

    /*!
     * Check lock
     */
    void CheckLocked() {
        CHECK_NE(atomic_load(&state_, memory_order_relaxed), kUnlocked);
    }

private:
    atomic_uint32_t state_;

    enum {
        kUnlocked = 0,
        kWriteLock = 1,
        kReadLock = 2
    };

    /*!
     * Lock slow
     */
    void NOINLINE LockSlow() {
        for (int i = 0;; i++) {
            if (i < 10)
                proc_yield(10);
            else
                internal_sched_yield();

            u32 cmp = atomic_load(&state_, memory_order_relaxed);
            if (cmp == kUnlocked &&
                    atomic_compare_exchange_weak(&state_, &cmp, kWriteLock,
                                                 memory_order_acquire))
                return;
        }
    }

    /*!
     * Read lock slow
     */
    void NOINLINE ReadLockSlow() {
        for (int i = 0;; i++) {
            if (i < 10)
                proc_yield(10);
            else
                internal_sched_yield();

            u32 prev = atomic_load(&state_, memory_order_acquire);
            if ((prev & kWriteLock) == 0)
                return;
        }
    }

    /*!
     * Constructor
     */
    RWMutex(const RWMutex&);

    /*!
     * Overloading operator =
     */
    void operator = (const RWMutex&);
};

/*!
 * Generic scoped lock
 */
template<typename MutexType>
class GenericScopedLock {
public:
    /*!
     * Constructor
     */
    explicit GenericScopedLock(MutexType *mu)
        : mu_(mu) {
        mu_->Lock();
    }

    /*!
     * Destructor
     */
    ~GenericScopedLock() {
        mu_->Unlock();
    }

private:
    MutexType *mu_;

    /*!
     * Constructor
     */
    GenericScopedLock(const GenericScopedLock&);

    /*!
     * Overloading operator =
     */
    void operator=(const GenericScopedLock&);
};

/*!
 * Generic scoped read lock
 */
template<typename MutexType>
class GenericScopedReadLock {
public:
    /*!
     * Constructor
     */
    explicit GenericScopedReadLock(MutexType *mu)
        : mu_(mu) {
        mu_->ReadLock();
    }

    /*!
     * Destructor
     */
    ~GenericScopedReadLock() {
        mu_->ReadUnlock();
    }

private:
    MutexType *mu_;

    /*!
     * Constructor
     */
    GenericScopedReadLock(const GenericScopedReadLock&);

    /*!
     * Overload operator =
     */
    void operator=(const GenericScopedReadLock&);
};

/// So far only blocking mutex is used in thread scheduling
/// Lock type definitions
//{@
typedef GenericScopedLock<StaticSpinMutex> SpinMutexLock;
typedef GenericScopedLock<BlockingMutex> BlockingMutexLock;
typedef GenericScopedLock<RWMutex> RWMutexLock;
typedef GenericScopedReadLock<RWMutex> RWMutexReadLock;
//@}

}  // namespace __rtl_common

#endif  // RTL_MUTEX_H
