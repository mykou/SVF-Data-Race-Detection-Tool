/*
 * DCIInterface.cpp
 *
 *  Created on: 4 Oct 2016
 *      Author: pengd
 */

/*
 * DCI checks alias and escape address dynamically
 */

#include "DCIInterface.h"
#include "DCIRTL.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include <map>
#include <vector>

#include <stdio.h>
#include <sys/types.h>

using namespace __dci;


/*
 * DCI instrumentation interfaces for initialization
 */
void __dci_init(unsigned instr_num) {
    Initialize(instr_num);
}

/*
 * DCI instrumentation interfaces for finish function
 */
void __dci_fini() {
    Finalize();
}

/*
 * DCI function for memory read
 * ksize: n means the bytes of addr. 2^n bytes. e.g 2(4 bytes)
 */
void __dci_memory_read(void *addr, u16 ksize, unsigned instID) {
    u16 x_ = DCIMem::getAbsAddr(addr, ksize);
    dci->lockInfo((unsigned)get_counter());
    dci->dciinfo[get_counter()]->addReadInst(x_, instID);
    dci->unlockInfo((unsigned)get_counter());
    if (dci->isemptyIG(instID))
        return;
    dci->checkReadPair(x_, instID);
}

/*
 * DCI function for memory write
 */
void __dci_memory_write(void *addr, u16 ksize, unsigned instID) {
    u16 x_ = DCIMem::getAbsAddr(addr, ksize);
    dci->lockInfo((unsigned)get_counter());
    dci->dciinfo[get_counter()]->addWriteInst(x_, instID);
    dci->unlockInfo((unsigned)get_counter());
    if (dci->isemptyIG(instID))
        return;
    dci->checkWritePair(x_, instID);
}

/// DCI instrumentation interfaces for memory intrinsic
//@{
/// For guarantee DCI performance, the memory intrinsics with only 4 bytes
/// and 8 bytes as a unit size are supported. Other sizes operations are ignored.
void __dci_memmove(void *dst, void *src, int size, unsigned instID) {
    if (size < 1)
        return;
    for (int i = 0; i < size; i = i + 4) {
        __dci_memory_read((void*) ((uint8_t*) src + i), 2, instID);
        __dci_memory_write((void*) ((uint8_t*) dst + i), 2, instID);
    }
    for (int i = 0; i < size; i = i + 8) {
        __dci_memory_read((void*) ((uint8_t*) src + i), 3, instID);
        __dci_memory_write((void*) ((uint8_t*) dst + i), 3, instID);
    }
}
void __dci_memcpy(void *dst, void *src, int size, unsigned instID) {
    if (size < 1)
        return;
    for (int i = 0; i < size; i = i + 4) {
        __dci_memory_read((void*) ((uint8_t*) src + i), 2, instID);
        __dci_memory_write((void*) ((uint8_t*) dst + i), 2, instID);
    }
    for (int i = 0; i < size; i = i + 8) {
        __dci_memory_read((void*) ((uint8_t*) src + i), 3, instID);
        __dci_memory_write((void*) ((uint8_t*) dst + i), 3, instID);
    }
}
void __dci_memset(void *dst, int size, unsigned instID) {
    if (size < 1)
        return;
    for (int i = 0; i < size; i = i + 4) {
        __dci_memory_write((void*) ((uint8_t*) dst + i), 2, instID);
    }
    for (int i = 0; i < size; i = i + 8) {
        __dci_memory_write((void*) ((uint8_t*) dst + i), 2, instID);
    }
}
//@}

/// DCI instrumentation interfaces for free instruction
//@{
/// For free instruction, it may be reported as a use-after-free bug
/// So far I haven't dealt with such a bug.
void __dci_free(void *addr, unsigned instID) {
}
//@}

/// DCI instrumentation interfaces for reads
//@{
void __dci_read1(void *addr, unsigned instID) {
    __dci_memory_read(addr, 0, instID);
}

void __dci_read2(void *addr, unsigned instID) {
    __dci_memory_read(addr, 1, instID);
}

void __dci_read4(void *addr, unsigned instID) {
    __dci_memory_read(addr, 2, instID);
}

void __dci_read8(void *addr, unsigned instID) {
    __dci_memory_read(addr, 3, instID);
}

// __dci_read16 calls are emitted by compiler.
void __dci_read16(void *addr, unsigned instID) {
}
//@}

/// DCI instrumentation interfaces for writes
//@{
void __dci_write1(void *addr, unsigned instID) {
    __dci_memory_write(addr, 0, instID);
}

void __dci_write2(void *addr, unsigned instID) {
    __dci_memory_write(addr, 1, instID);
}

void __dci_write4(void *addr, unsigned instID) {
    __dci_memory_write(addr, 2, instID);
}

void __dci_write8(void *addr, unsigned instID) {
    __dci_memory_write(addr, 3, instID);
}

// __dci_write16 calls are emitted by compiler.
void __dci_write16(void *addr, unsigned instID) {
}
//@}

// __dci_unaligned_read/write calls are emitted by compiler.
/// DCI instrumentation interfaces for unaligned reads
//@{
void __dci_unaligned_read2(void *addr, unsigned instID) {
    __dci_memory_read(addr, 1, instID);
}

void __dci_unaligned_read4(void *addr, unsigned instID) {
    __dci_memory_read(addr, 2, instID);
}

void __dci_unaligned_read8(void *addr, unsigned instID) {
    __dci_memory_read(addr, 3, instID);
}

void __dci_unaligned_read16(void *addr, unsigned instID) {
}
//@}

/// DCI instrumentation interfaces for unaligned writes
//@{
void __dci_unaligned_write2(void *addr, unsigned instID) {
    __dci_memory_write(addr, 1, instID);
}

void __dci_unaligned_write4(void *addr, unsigned instID) {
    __dci_memory_write(addr, 2, instID);
}

void __dci_unaligned_write8(void *addr, unsigned instID) {
    __dci_memory_write(addr, 3, instID);
}

void __dci_unaligned_write16(void *addr, unsigned instID) {
}
//@}
