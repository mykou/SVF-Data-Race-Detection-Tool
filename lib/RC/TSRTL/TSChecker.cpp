/*
 * TSChecker.cc
 *
 *  Created on: 26 May 2016
 *      Author: pengd
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>

#include "TSRTL.h"
#include "TSChecker.h"
#include "TSInterceptors.h"

#include <set>
#include <map>


#define MAX_MALLOC_SIZE 1ull<<20
namespace __ts {

/// Stall breaker for thread scheduling
extern StallBreaker *stallbreaker;

/*
 * Get a random bool
 */
bool randomBool() {
    srand(time(NULL));
    return rand() % 2 == 1;
}

/*
 * Set the waiting flag.
 */
void Semaphore::setwaitflag(uint8_t _wf) {
    atomic_store(&waitflag, _wf, memory_order_relaxed);
}

/*
 * Release the waiting flag.
 */
void Semaphore::release() {
    atomic_store(&waitflag, 0, memory_order_relaxed);
}

/*
 * Acquire the waiting flag.
 */
void Semaphore::acquire() {
    while (atomic_load(&waitflag, memory_order_relaxed)) {

        if (stallbreaker->thrToWaitTime[get_counter()]>stallbreaker->maxWaitNumber) {
            DBPRINTF(2, std::cout<<"~~## tid:"<< get_counter()<< " wait time:" << stallbreaker->thrToWaitTime[get_counter()]<< "\n");
            break;
        }

        usleep(stallbreaker->waitTime);
        stallbreaker->thrToWaitTime[get_counter()]++;

        if (stallbreaker->IsAllBlocked()) {
            DBPRINTF(2, std::cout<<"~~## tid:"<< get_counter()
                     << " total:" << atomic_load(&stallbreaker->total_thread, memory_order_relaxed) << " alive:" << stallbreaker->getAlive()
                     << " enabled:" << stallbreaker->getEnabled()<< " postponed:" << stallbreaker->getPostponed()<<"\n");
            break;
        }
    }
}

/*
 * Constructor
 */
ActiveChecker::ActiveChecker(): sem(NULL), waddr(NULL), wsize(0), raddr(NULL), rsize(0), instID(-1) {
    atomic_store(&isBlocked, 0, memory_order_relaxed);
    sem = new Semaphore();
}

/*
 * Constructor
 */
ActiveChecker::ActiveChecker(void* _waddr, size_t _wsize, void* _raddr, size_t _rsize, size_t _instID): sem(NULL), waddr(_waddr), wsize(_wsize), raddr(_raddr), rsize(_rsize), instID(_instID) {
    atomic_store(&isBlocked, 0, memory_order_relaxed);
    sem = new Semaphore();
}

/*
 * Destructor
 */
ActiveChecker::~ActiveChecker() {
    delete sem;
}

/*
 * Set address of memory operations
 */
void ActiveChecker::setAddr(void* _waddr, size_t _wsize, void* _raddr, size_t _rsize, size_t _instID) {
    waddr = _waddr;
    wsize = _wsize;
    raddr = _raddr;
    rsize = _rsize;
    instID = _instID;
}

/*
 * Reset address
 */
void ActiveChecker::resetAddr() {
    waddr = NULL;
    wsize = 0;
    raddr = NULL;
    rsize = 0;
}

/*
 * Block current thread to wait other threads or satisfying unblock conditions.
 */
void ActiveChecker::block() {
    DBPRINTF(2, std::cout<<"## block   tid:"<< get_counter()<< " waddr:" << ((waddr)?waddr:"null") <<"+"<<wsize << " raddr:"<< ((raddr)? raddr:"null") <<"+"<<rsize<< "\n");
    if (stallbreaker->thrToWaitTime[get_counter()]>stallbreaker->maxWaitNumber) {
        resetAddr();
        stallbreaker->ac_mutex.Unlock();
        return;
    }
    stallbreaker->IncrementPostponed();
    stallbreaker->ac_mutex.Unlock();

    assert(sem);
    sem->acquire();

    stallbreaker->ac_mutex.Lock();
    resetAddr();


    stallbreaker->DecrementPostponed();
    stallbreaker->ac_mutex.Unlock();
}

/*
 * Unblock current thread.
 */
void ActiveChecker::unblock() {
    DBPRINTF(2, std::cout<<"## unblock   tid:"<< get_counter()<< " waddr:" << ((waddr)?waddr:"null") <<"+"<<wsize << " raddr:"<< ((raddr)? raddr:"null") <<"+"<<rsize<< "\n");
    assert(sem);
    sem->release();
}

/*
 * Check overlap of two addresses
 */
inline bool overlapAddr(void* addr1, size_t size1, void* addr2, size_t size2) {
    assert(addr1&&addr2&&size1&&size2);
    if (addr1==addr2)
        return true;
    return (((addr1 <= addr2) && (addr2 < (uint8_t *)addr1 + size1)) || ((addr2 <= addr1) && (addr1 < (uint8_t *)addr2 + size2)));
}

/*
 * Check if a risky pair exists.
 */
void ActiveChecker::check() {
    DBPRINTF(2, std::cout<<"## checking tid:"<< get_counter()<< " waddr:" << ((waddr)?waddr:"null") <<"+"<<wsize << " raddr:"<< ((raddr)? raddr:"null") <<"+"<<rsize<< "\n");
    stallbreaker->ac_mutex.Lock();
    for (UptrToChecker::iterator it = stallbreaker->thrToChecker.begin(),eit = stallbreaker->thrToChecker.end(); it!=eit; ++it) {
        // Not itself
        if(it->first == get_counter())
            continue;
        ActiveChecker* other = it->second;

        if (wsize>0&&wsize<MAX_MALLOC_SIZE) {
            if (other->wsize>0&&other->wsize<MAX_MALLOC_SIZE) {
                if (overlapAddr(waddr, wsize, other->waddr, other->wsize)) {
                    if (stallbreaker->selfcheck || instID != other->instID) {
                        std::cout << "      ****** TS Race at t" << get_counter() << " writes " << waddr << "+" << wsize << " t"
                                  << it->first << " writes " << other->waddr << "+" << other->wsize << " ******\n";
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (other->rsize>0&&other->rsize<MAX_MALLOC_SIZE) {
                if (overlapAddr(waddr, wsize, other->raddr, other->rsize)) {
                    if (stallbreaker->selfcheck || instID != other->instID) {
                        std::cout << "      ****** TS Race at t" << get_counter() << " writes " << waddr << "+" << wsize << " t"
                                  << it->first << " reads " << other->raddr << "+" << other->rsize << " ******\n";
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        if (rsize>0&&rsize<MAX_MALLOC_SIZE) {
            if (other->wsize>0&&other->wsize<MAX_MALLOC_SIZE) {
                if (overlapAddr(raddr, rsize, other->waddr, other->wsize)) {
                    if (stallbreaker->selfcheck || instID != other->instID) {
                        std::cout << "      ****** TS Race at t" << get_counter() << " reads " << raddr << "+" << rsize << " t"
                                  << it->first << " writes " << other->waddr << "+" << other->wsize << " ******\n";
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    block();
}

}
