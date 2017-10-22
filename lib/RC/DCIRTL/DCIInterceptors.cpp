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
#include <iostream>

#include "../RTLCommon/RTLMutex.h"
#include "DCIInterceptors.h"
#include "DCIRTL.h"

#include <set>
#include <map>

namespace __dci {

pthread_key_t counter;
atomic_uint64_t total_thread;
extern DCIInfo *dci;

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
void *__dci_thread_start_func(void *arg) {

    ThreadCreateInfo *p = (ThreadCreateInfo*)arg;
    void* (*callback)(void *arg) = p->callback;
    void *param = p->param;
    u32 tid = 0;

    if (set_counter(atomic_fetch_add(&total_thread, 1, memory_order_relaxed))) {
        std::cout <<"TS: failed to set thread counter\n";
    }

    /// Sync with parent thread
    while ((tid = atomic_load(&p->tid, memory_order_acquire)) == UINT32_MAX)
        internal_sched_yield();
    atomic_store(&p->tid, UINT32_MAX, memory_order_release);

    dci->dciinfo[get_counter()] = new DCIMem();

    assert(callback);
    void *res = callback(param);

    return res;
}

/*
 * Interceptor interface for pthread_create
 */
DECLARE_REAL(int, pthread_create, pthread_t *th, const pthread_attr_t *attr,
             void *(*callback)(void*), void * param)
INTERCEPTOR(int, pthread_create,
            pthread_t *th, const pthread_attr_t *attr, void *(*callback)(void*), void * param) {

    ThreadCreateInfo p;
    p.callback = callback;
    p.param = param;

    /// Set p.tid UINT32_MAX for sync
    atomic_store(&p.tid, UINT32_MAX, memory_order_relaxed);

    int res = REAL(pthread_create)(th, attr, __dci_thread_start_func, &p);

    /// Let child thread call real callback function
    atomic_store(&p.tid, get_counter(), memory_order_release);
    while (atomic_load(&p.tid, memory_order_acquire) != UINT32_MAX)
        internal_sched_yield();

    return res;
}

/*
 * Initialize interceptor
 */
void InitializeInterceptors() {

    atomic_store(&total_thread, 0, memory_order_relaxed);

    if (pthread_key_create(&counter, NULL)) {
        std::cout<<"TS: failed to create thread key.\n";
    }

    // set main thread
    if (set_counter(atomic_fetch_add(&total_thread, 1, memory_order_relaxed))) {
        std::cout<<"TS: failed to set thread counter\n";
    }

    dci->dciinfo[get_counter()] = new DCIMem();

    INTERCEPT_FUNCTION(pthread_create);
}

}
