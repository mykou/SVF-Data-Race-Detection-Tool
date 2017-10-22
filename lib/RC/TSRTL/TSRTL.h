/*
 * TSRTL.h
 *
 *  Created on: 22 Jun 2016
 *      Author: pengd
 */

#ifndef TSRTL_H_
#define TSRTL_H_

#include <atomic>
#include <map>
#include <set>

#include "../RTLCommon/RTLMutex.h"
#include "TSChecker.h"

/// Print debug information 0: do not print debug information
#if 0
#define DPLEVEL 0 //Debug print level, 0-5, 0 is highest print level. 5 prints all debug information. Default is 0.
extern BlockingMutex print_mtx;

#define PRINTF(...) print_mtx.Lock(); \
	printf(__VA_ARGS__); \
	print_mtx.Unlock();

#define DBPRINTF(DPL, ...)  if (DPL<DPLEVEL) {\
    print_mtx.Lock(); \
	__VA_ARGS__; \
	print_mtx.Unlock(); }

#else
#define PRINTF(...)
#define DBPRINTF(...)
#endif

namespace __ts {

typedef unsigned long uptr;
typedef std::map<uptr, ActiveChecker*> UptrToChecker;
typedef std::map<uptr, pthread_t*> UptrToThrMap;
typedef std::map<uptr, pthread_mutex_t*> UptrToMtxMap;
typedef std::set<uptr> ThrSet;
typedef std::map<uptr, int> ThrToInt;
typedef std::set<ActiveChecker*> CheckerSet;

/*!
 * Get current thread id
 */
uptr get_counter();

/*!
 * StallBreaker is used to monitor the status of all running threads
 */
struct StallBreaker {

    /// Wait time and maximal wait times
    int waitTime=10000;
    int maxWaitNumber=100;

    /// Is self check
    bool selfcheck=false;

    /// The map between thread to wait time
    ThrToInt thrToWaitTime;

    /// The map between thread to checker
    UptrToChecker thrToChecker;

    /// Locks
    BlockingMutex mtx_;
    BlockingMutex ac_mutex;

    /// The number of total threads
    atomic_uint64_t total_thread;

    /// The set of postponed threads
    ThrSet postponed_thread;

    /// The set of enabled threads
    ThrSet enabled_thread;

    /// The set of alive threads
    ThrSet alive_thread;

    /*!
     * Add one total thread
     */
    uint64_t IncrementTotal() {
        return atomic_fetch_add(&total_thread, 1, memory_order_relaxed);
    }

    /*!
     * Get the number of alive threads
     */
    int getAlive() {
        return alive_thread.size();
    }

    /*!
     * Add one alive thread
     */
    void IncrementAlive(std::string s="\t") {
        BlockingMutexLock l(&mtx_);
        alive_thread.insert(get_counter());

        DBPRINTF(4, std::cout << s <<" tid: " << get_counter() << " alive_thread++ has " << alive_thread.size() << " elements:"; \
        for(ThrSet::iterator iter=alive_thread.begin(); iter!=alive_thread.end(); ++iter) {
        std::cout<<" "<< *iter;
    } \
    std::cout<<"\n";
            );
    }

    /*!
     * Remove one from alive thread
     */
    void DecrementAlive(std::string s="\t") {
        BlockingMutexLock l(&mtx_);
        ThrSet::iterator it = alive_thread.find(get_counter());
        if(it!=alive_thread.end()) {
            alive_thread.erase(it);
        }

        DBPRINTF(4, std::cout << s <<" tid: " << get_counter() << " alive_thread-- has " << alive_thread.size() << " elements:"; \
        for(ThrSet::iterator iter=alive_thread.begin(); iter!=alive_thread.end(); ++iter) {
        std::cout<<" "<< *iter;
    } \
    std::cout<<"\n";
            );
    }

    /*!
     * Get the number of enabled threads
     */
    int getEnabled() {
        return enabled_thread.size();
    }

    /*!
     * Add one enabled thread
     */
    void IncrementEnabled(std::string s="\t") {
        BlockingMutexLock l(&mtx_);
        enabled_thread.insert(get_counter());

        DBPRINTF(4, std::cout << s <<" tid: " << get_counter() << " enabled_thread++ has " << enabled_thread.size() << " elements:"; \
        for(ThrSet::iterator iter=enabled_thread.begin(); iter!=enabled_thread.end(); ++iter) {
        std::cout<<" "<< *iter;
    } \
    std::cout<<"\n";
            );

        DBPRINTF(4, std::cout << s <<" tid: " << get_counter() << " postponed_thread has " << postponed_thread.size() << " elements:"; \
        for(ThrSet::iterator iter=postponed_thread.begin(); iter!=postponed_thread.end(); ++iter) {
        std::cout<<" "<< *iter;
    } \
    std::cout<<"\n";
            );
    }

    /*!
     * Remove one from enabled thread
     */
    void DecrementEnabled(std::string s="\t") {
        BlockingMutexLock l(&mtx_);
        ThrSet::iterator it = enabled_thread.find(get_counter());
        if(it!=enabled_thread.end()) {
            enabled_thread.erase(it);
        }

        DBPRINTF(4, std::cout << s <<" tid: " << get_counter() << " enabled_thread-- has " << enabled_thread.size() << " elements:"; \
        for(ThrSet::iterator iter=enabled_thread.begin(); iter!=enabled_thread.end(); ++iter) {
        std::cout<<" "<< *iter;
    } \
    std::cout<<"\n";
            );

        DBPRINTF(4, std::cout << s <<" tid: " << get_counter() << " postponed_thread has " << postponed_thread.size() << " elements:"; \
        for(ThrSet::iterator iter=postponed_thread.begin(); iter!=postponed_thread.end(); ++iter) {
        std::cout<<" "<< *iter;
    } \
    std::cout<<"\n";
            );
    }

    /*!
     * Get the number of postponed threads
     */
    int getPostponed() {
        return postponed_thread.size();
    }

    /*!
     * Add one postponed thread
     */
    void IncrementPostponed(std::string s="\t") {
        BlockingMutexLock l(&mtx_);
        postponed_thread.insert(get_counter());

        DBPRINTF(4, std::cout << s <<" tid: " << get_counter() << " postponed_thread++ has " << postponed_thread.size() << " elements:"; \
        for(ThrSet::iterator iter=postponed_thread.begin(); iter!=postponed_thread.end(); ++iter) {
        std::cout<<" "<< *iter;
    } \
    std::cout<<"\n";
            );
    }

    /*!
     * Remove one from postponed thread
     */
    void DecrementPostponed(std::string s="\t") {
        BlockingMutexLock l(&mtx_);
        ThrSet::iterator it = postponed_thread.find(get_counter());
        if(it!=postponed_thread.end()) {
            postponed_thread.erase(it);
        }

        DBPRINTF(4, std::cout << s <<" tid: " << get_counter() << " postponed_thread-- has " << postponed_thread.size() << " elements:"; \
        for(ThrSet::iterator iter=postponed_thread.begin(); iter!=postponed_thread.end(); ++iter) {
        std::cout<<" "<< *iter;
    } \
    std::cout<<"\n";
            );
    }

    /*!
     * Check if all threads are blocked
     */
    bool IsAllBlocked() {
        BlockingMutexLock l(&mtx_);
        return postponed_thread==enabled_thread;
    }

};

/// StallBreaker stores status of running threads
extern StallBreaker *stallbreaker;

/*!
 * Initialize function
 */
void Initialize();

/*!
 * Finalize function
 */
void Finalize();
}

#endif /* TSRTL_H_ */
