/*
 * RFinterceptors.cpp
 *
 *  Created on: 10 Jun 2016
 *      Author: pengd
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "TSInterceptors.h"
#include "TSRTL.h"

#include <iostream>

#include <set>
#include <map>

namespace __ts {

extern StallBreaker *stallbreaker;
pthread_key_t counter;

/*
 * Get current thread ID
 */
uptr get_counter() {
    return (uptr) pthread_getspecific(counter);
}

/*
 * Set current thread ID
 */
int set_counter(uptr value) {
    return pthread_setspecific(counter, (void *) value);
}

/*
 * Thread creation information
 */
struct ThreadCreateInfo {

    /// Called function of thread creation
    void* (*callback)(void *arg);

    /// Parameters of thread creation
    void *param;

    /// Parent thread ID. It is used to synchronize thread creation
    atomic_uintptr_t tid;
};

/*
 * Rebuild call function for thread creation
 */
void *__ts_thread_start_func(void *arg) {

    ThreadCreateInfo *p = (ThreadCreateInfo*)arg;
    void* (*callback)(void *arg) = p->callback;
    void *param = p->param;
    u32 tid = 0;

    if (set_counter(stallbreaker->IncrementTotal())) {
        std::cout<<"TS: failed to set thread counter\n";
    } else {
        stallbreaker->IncrementAlive();
        stallbreaker->IncrementEnabled("create\t");
        assert(!stallbreaker->thrToChecker[get_counter()]);
        stallbreaker->thrToChecker[get_counter()] = new ActiveChecker();
    }

    /// Sync with parent thread
    while ((tid = atomic_load(&p->tid, memory_order_acquire)) == UINT32_MAX)
        internal_sched_yield();
    atomic_store(&p->tid, UINT32_MAX, memory_order_release);

    DBPRINTF(1, std::cout<<"# ThreadCreate "<< tid << " -> "<< get_counter()<<"\n");

    assert(callback);
    void *res = callback(param);

    assert(stallbreaker->thrToChecker[get_counter()]);
    delete stallbreaker->thrToChecker[get_counter()];

    stallbreaker->DecrementAlive();
    stallbreaker->DecrementEnabled("create\t");
    return res;
}

/*
 * Interceptor interface for pthread_create
 */
DECLARE_REAL(int, pthread_create, pthread_t *th, const pthread_attr_t *attr,
             void *(*callback)(void*), void * param)
INTERCEPTOR(int, pthread_create,
            pthread_t *th, const pthread_attr_t *attr, void *(*callback)(void*), void * param) {

    DBPRINTF(5, std::cout<<"# enter create: "<< get_counter()<<"\n");

    ThreadCreateInfo p;
    p.callback = callback;
    p.param = param;

    /// Set p.tid UINT32_MAX for sync
    atomic_store(&p.tid, UINT32_MAX, memory_order_relaxed);

    int res = REAL(pthread_create)(th, attr, __ts_thread_start_func, &p);

    /// Let child thread call real callback function
    atomic_store(&p.tid, get_counter(), memory_order_release);
    while (atomic_load(&p.tid, memory_order_acquire) != UINT32_MAX)
        internal_sched_yield();

    DBPRINTF(5, std::cout<<"# exit  create: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_join
 */
DECLARE_REAL(int, pthread_join, pthread_t th, void **ret)
INTERCEPTOR(int, pthread_join, pthread_t th, void **ret) {

    DBPRINTF(5, std::cout<<"# enter join: "<< get_counter()<<"\n");
    stallbreaker->DecrementEnabled("join\t");

    int res = REAL(pthread_join)(th, ret);

    stallbreaker->IncrementEnabled("join\t");
    DBPRINTF(5, std::cout<<"# exit  join: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_mutex_lock
 */
DECLARE_REAL(int, pthread_mutex_lock, pthread_mutex_t *m)
INTERCEPTOR(int, pthread_mutex_lock, pthread_mutex_t *m) {

    DBPRINTF(5, std::cout<<"# enter lock: "<< get_counter()<<"\n");
    stallbreaker->DecrementEnabled("lock\t");

    int res = REAL(pthread_mutex_lock)(m);

    stallbreaker->IncrementEnabled("lock\t");
    DBPRINTF(5, std::cout<<"# exit  lock: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_mutex_trylock
 */
DECLARE_REAL(int, pthread_mutex_trylock, pthread_mutex_t *m)
INTERCEPTOR(int, pthread_mutex_trylock, pthread_mutex_t *m) {

    DBPRINTF(5, std::cout<<"# enter trylock: "<< get_counter()<<"\n");

    int res = REAL(pthread_mutex_trylock)(m);

    DBPRINTF(5, std::cout<<"# exit  trylock: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_mutex_unlock
 */
DECLARE_REAL(int, pthread_mutex_unlock, pthread_mutex_t *m)
INTERCEPTOR(int, pthread_mutex_unlock, pthread_mutex_t *m) {
    DBPRINTF(5, std::cout<<"# enter unlock: "<< get_counter()<<"\n");

    int res = REAL(pthread_mutex_unlock)(m);

    DBPRINTF(5, std::cout<<"# exit  unlock: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_cond_wait
 */
DECLARE_REAL(int, pthread_cond_wait, pthread_cond_t *c, pthread_mutex_t *m)
INTERCEPTOR(int, pthread_cond_wait, pthread_cond_t *c, pthread_mutex_t *m) {

    DBPRINTF(5, std::cout<<"# enter cond_wait: "<< get_counter()<<"\n");
    stallbreaker->DecrementEnabled("wait\t");

    int res = REAL(pthread_cond_wait)(c, m);

    stallbreaker->IncrementEnabled("wait\t");
    DBPRINTF(5, std::cout<<"# exit  cond_wait: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_cond_signal
 */
DECLARE_REAL(int, pthread_cond_signal, pthread_cond_t *c)
INTERCEPTOR(int, pthread_cond_signal, pthread_cond_t *c) {

    DBPRINTF(5, std::cout<<"# enter cond_signal: "<< get_counter()<<"\n");

    int res = REAL(pthread_cond_signal)(c);

    DBPRINTF(5, std::cout<<"# exit  cond_signal: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_cond_broadcast
 */
DECLARE_REAL(int, pthread_cond_broadcast, pthread_cond_t *c)
INTERCEPTOR(int, pthread_cond_broadcast, pthread_cond_t *c) {

    DBPRINTF(5, std::cout<<"# enter cond_broadcast: "<< get_counter()<<"\n");

    int res = REAL(pthread_cond_broadcast)(c);

    DBPRINTF(5, std::cout<<"# exit  cond_broadcast: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_cond_destroy
 */
DECLARE_REAL(int, pthread_cond_destroy, pthread_cond_t *c)
INTERCEPTOR(int, pthread_cond_destroy, pthread_cond_t *c) {

    DBPRINTF(5, std::cout<<"# enter cond_destroy: "<< get_counter()<<"\n");

    int res = REAL(pthread_cond_destroy)(c);

    DBPRINTF(5, std::cout<<"# exit  cond_destroy: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_barrier_init
 */
DECLARE_REAL(int, pthread_barrier_init, pthread_barrier_t *b, const pthread_barrierattr_t *a, unsigned count)
INTERCEPTOR(int, pthread_barrier_init, pthread_barrier_t *b, const pthread_barrierattr_t *a, unsigned count) {
    DBPRINTF(5, std::cout<<"# enter barrier_init: "<< get_counter()<<"\n");

    int res = REAL(pthread_barrier_init)(b, a, count);

    DBPRINTF(5, std::cout<<"# exit  barrier_init: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_barrier_destroy
 */
DECLARE_REAL(int, pthread_barrier_destroy, pthread_barrier_t *b)
INTERCEPTOR(int, pthread_barrier_destroy, pthread_barrier_t *b) {
    DBPRINTF(5, std::cout<<"# enter barrier_destroy: "<< get_counter()<<"\n");

    int res = REAL(pthread_barrier_destroy)(b);

    DBPRINTF(5, std::cout<<"# exit  barrier_destroy: "<< get_counter()<<"\n");
    return res;
}

/*
 * Interceptor interface for pthread_barrier_wait
 */
DECLARE_REAL(int, pthread_barrier_wait, pthread_barrier_t *b)
INTERCEPTOR(int, pthread_barrier_wait, pthread_barrier_t *b) {

    DBPRINTF(5, std::cout<<"# enter barrier_wait: "<< get_counter()<<"\n");
    stallbreaker->DecrementEnabled("wait\t");

    int res = REAL(pthread_barrier_wait)(b);

    stallbreaker->IncrementEnabled("wait\t");
    DBPRINTF(5, std::cout<<"# exit  barrier_wait: "<< get_counter()<<"\n");
    return res;
}

/*
 * Initialize interceptor
 */
void InitializeInterceptors() {

    if (pthread_key_create(&counter, NULL)) {
        std::cout<<"TS: failed to create thread key.\n";
    }

    // set main thread
    if (set_counter(stallbreaker->IncrementTotal())) {
        std::cout<<"TS: failed to set thread counter\n";
    } else {
        stallbreaker->IncrementAlive("\tmain");
        stallbreaker->IncrementEnabled();
        assert(!stallbreaker->thrToChecker[get_counter()]);
        stallbreaker->thrToChecker[get_counter()] = new ActiveChecker();
        DBPRINTF(1, std::cout<<"# Main thread: "<< get_counter()<<"\n");
    }

    INTERCEPT_FUNCTION(pthread_create);
    INTERCEPT_FUNCTION(pthread_join);

    INTERCEPT_FUNCTION(pthread_mutex_lock);
    INTERCEPT_FUNCTION(pthread_mutex_trylock);
    INTERCEPT_FUNCTION(pthread_mutex_unlock);

    INTERCEPT_FUNCTION(pthread_cond_wait);
    INTERCEPT_FUNCTION(pthread_cond_signal);
    INTERCEPT_FUNCTION(pthread_cond_broadcast);
    INTERCEPT_FUNCTION(pthread_cond_destroy);

    INTERCEPT_FUNCTION(pthread_barrier_init);
    INTERCEPT_FUNCTION(pthread_barrier_destroy);
    INTERCEPT_FUNCTION(pthread_barrier_wait);

}

/*
 * Interceptor interface for __tsan_testonly_barrier_init
 * This interface is only used for TSAN test case set
 */
extern "C" RTL_INTERFACE_ATTRIBUTE
void __tsan_testonly_barrier_init(u64 *barrier, u32 count) {
    if (count >= (1 << 8)) {
        PRINTF("barrier_init: count is too large (%d)\n", count);
        exit(0);
    }
    // 8 lsb is thread count, the remaining are count of entered threads.
    *barrier = count;
}

/*
 * Interceptor interface for __tsan_testonly_barrier_wait
 * This interface is only used for TSAN test case set
 */
extern "C" RTL_INTERFACE_ATTRIBUTE
void __tsan_testonly_barrier_wait(u64 *barrier) {
    stallbreaker->DecrementEnabled("tsan_barrier\t");
    unsigned old = __atomic_fetch_add(barrier, 1 << 8, __ATOMIC_RELAXED);
    unsigned old_epoch = (old >> 8) / (old & 0xff);
    for (;;) {
        unsigned cur = __atomic_load_n(barrier, __ATOMIC_RELAXED);
        unsigned cur_epoch = (cur >> 8) / (cur & 0xff);
        if (cur_epoch != old_epoch)
            return;

        internal_sched_yield();
    }
    stallbreaker->IncrementEnabled("tsan_barrier\t");
}

}
