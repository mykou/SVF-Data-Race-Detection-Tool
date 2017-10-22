/*
 * TSChecker.h
 *
 *  Created on: 26 May 2016
 *      Author: pengd
 */

#ifndef TSCHECKER_H_
#define TSCHECKER_H_


#include "../RTLCommon/RTLMutex.h"

#include <iostream>
#include <map>
#include <set>
#include <assert.h>

namespace __ts {
typedef unsigned long uptr;

/*!
 * This semaphore is black flag to schedule threads.
 */
class Semaphore {
public:
    /// Atomic waiting flag.
    atomic_uint16_t waitflag;

public:

    /*!
     * Constructor: initialize the waiting flag.
     */
    Semaphore() {
        atomic_store(&waitflag, 1, memory_order_relaxed);
    }

    ~Semaphore() {
    }

    /*!
     * Release the waiting flag.
     */
    void release();

    /*!
     * Acquire the waiting flag.
     */
    void acquire();

    /*!
     * Set the waiting flag.
     */
    void setwaitflag(uint8_t _wf);
};

/*!
 * This active checker aims to schedule threads.
 */
class ActiveChecker {

public:
    /// Semaphore
    Semaphore *sem;

    /// Write memory access address
    void* waddr;

    /// Write emory size
    size_t wsize;

    /// Read memory access address
    void* raddr;

    /// Read memory size
    size_t rsize;

    /// Instruction ID
    size_t instID;

    /// Block flag
    atomic_uint16_t isBlocked;

public:

    /*!
     * Constructor
     */
    //@{
    ActiveChecker(void* _waddr, size_t _wsize, void* _raddr, size_t _rsize, size_t _instID);
    ActiveChecker();
    ~ActiveChecker();
    //@}

    /*!
     * Block current thread to wait other threads or satisfying unblock conditions.
     */
    void block();

    /*!
     * Unblock current thread.
     */
    void unblock();

    /*!
     * Set address of memory operations. _waddr _raddr are the addresses of write and read;
     * _wsize _rsize are the sizes of write and read; e.g. memcpy has 4 values for these parameters
     * _instID is instruction ID
     */
    void setAddr(void* _waddr, size_t _wsize, void* _raddr, size_t _rsize, size_t _instID);

    /*!
     * Reset the addresses and their size
     */
    void resetAddr();

    /*!
     * Check if a risky pair exists.
     */
    void check();
};

}

#endif /* TSCHECKER_H_ */
