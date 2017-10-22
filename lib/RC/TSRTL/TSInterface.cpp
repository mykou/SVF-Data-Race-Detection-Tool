/*
 * TSInterface.cpp
 *
 *  Created on: 18 May 2016
 *      Author: pengd
 */


#include "TSInterface.h"
#include "TSInterceptors.h"
#include "TSRTL.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>

#include <map>
#include <vector>

#include <stdio.h>
#include <sys/types.h>

using namespace __ts;

/// Stall breaker for thread scheduling
extern StallBreaker* __ts::stallbreaker;

/*
 * TS function for memory read
 */
void __ts_memory_read(void *addr, size_t size, size_t instID) {
    assert(stallbreaker->thrToChecker[get_counter()]);
    stallbreaker->thrToChecker[get_counter()]->setAddr(NULL, 0, addr, size, instID);
    stallbreaker->thrToChecker[get_counter()]->check();
}

/*
 * TS function for memory write
 */
void __ts_memory_write(void *addr, size_t size, size_t instID) {
    assert(stallbreaker->thrToChecker[get_counter()]);
    stallbreaker->thrToChecker[get_counter()]->setAddr(addr, size, NULL, 0, instID);
    stallbreaker->thrToChecker[get_counter()]->check();
}


/*
 * TS instrumentation interfaces for initialization
 */
void __ts_init() {
    Initialize();
}

/*
 * TS instrumentation interfaces for finish function
 */
void __ts_fini() {
    Finalize();
}

/// TS instrumentation interfaces for memory intrinsic
//@{
void __ts_memmove(void *dst, void *src, int size, unsigned instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_memmove at instID "<< instID<<"\n");
    assert(stallbreaker->thrToChecker[get_counter()]);
    stallbreaker->thrToChecker[get_counter()]->setAddr(dst, size, src, size, instID);
    stallbreaker->thrToChecker[get_counter()]->check();
}
void __ts_memcpy(void *dst, void *src, int size, unsigned instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_memcpy at instID "<< instID<<"\n");
    assert(stallbreaker->thrToChecker[get_counter()]);
    stallbreaker->thrToChecker[get_counter()]->setAddr(dst, size, src, size, instID);
    stallbreaker->thrToChecker[get_counter()]->check();
}
void __ts_memset(void *dst, int size, unsigned instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_memset at instID "<< instID<<"\n");
    assert(stallbreaker->thrToChecker[get_counter()]);
    stallbreaker->thrToChecker[get_counter()]->setAddr(dst, size, NULL, 0, instID);
    stallbreaker->thrToChecker[get_counter()]->check();
}

void __ts_self_memmove(void *dst, void *src, int size, unsigned instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_memmove at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    assert(stallbreaker->thrToChecker[get_counter()]);
    stallbreaker->thrToChecker[get_counter()]->setAddr(dst, size, src, size, instID);
    stallbreaker->thrToChecker[get_counter()]->check();
}
void __ts_self_memcpy(void *dst, void *src, int size, unsigned instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_memcpy at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    assert(stallbreaker->thrToChecker[get_counter()]);
    stallbreaker->thrToChecker[get_counter()]->setAddr(dst, size, src, size, instID);
    stallbreaker->thrToChecker[get_counter()]->check();
}
void __ts_self_memset(void *dst, int size, unsigned instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_memset at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    assert(stallbreaker->thrToChecker[get_counter()]);
    stallbreaker->thrToChecker[get_counter()]->setAddr(dst, size, NULL, 0, instID);
    stallbreaker->thrToChecker[get_counter()]->check();
}
//@}

/// TS instrumentation interfaces for free
//@{
/// For free instruction, it may be reported as a use-after-free bug
/// So far I haven't dealt with such a bug.
void __ts_free(void *addr, size_t instID) {
}
void __ts_self_free(void *addr, size_t instID) {
}
//@}

/// TS instrumentation interfaces for reads
//@{
void __ts_read1(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_read1 at instID "<< instID<<"\n");
    __ts_memory_read(addr, 1, instID);
}

void __ts_read2(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_read2 at instID "<< instID<<"\n");
    __ts_memory_read(addr, 2, instID);
}

void __ts_read4(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_read4 at instID "<< instID<<"\n");
    __ts_memory_read(addr, 4, instID);
}

void __ts_read8(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_read8 at instID "<< instID<<"\n");
    __ts_memory_read(addr, 8, instID);
}

// __ts_read16 calls are emitted by compiler.
void __ts_read16(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_read16 at instID "<< instID<<"\n");
}

void __ts_self_read1(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_read1 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_read(addr, 1, instID);
}

void __ts_self_read2(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_read2 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_read(addr, 2, instID);
}

void __ts_self_read4(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_read4 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_read(addr, 4, instID);
}

void __ts_self_read8(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_read8 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_read(addr, 8, instID);
}
// __ts_self_read16 calls are emitted by compiler.
void __ts_self_read16(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_read16 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
}
//@}

/// TS instrumentation interfaces for reads
//@{
void __ts_write1(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_write1 at instID "<< instID<<"\n");
    __ts_memory_write(addr, 1, instID);
}

void __ts_write2(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_write2 at instID "<< instID<<"\n");
    __ts_memory_write(addr, 2, instID);
}

void __ts_write4(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_write4 at instID "<< instID<<"\n");
    __ts_memory_write(addr, 4, instID);
}

void __ts_write8(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_write8 at instID "<< instID<<"\n");
    __ts_memory_write(addr, 8, instID);
}

// __ts_write16 calls are emitted by compiler.
void __ts_write16(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_write16 at instID "<< instID<<"\n");
}

void __ts_self_write1(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_write1 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_write(addr, 1, instID);
}

void __ts_self_write2(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_write2 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_write(addr, 2, instID);
}

void __ts_self_write4(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_write4 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_write(addr, 4, instID);
}

void __ts_self_write8(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_write8 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_write(addr, 8, instID);
}

// __ts_self_write16 calls are emitted by compiler.
void __ts_self_write16(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_write16 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
}
//@}

// __ts_unaligned_read calls are emitted by compiler.
/// TS instrumentation interfaces for reads
//@{
void __ts_unaligned_read2(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_unaligned_read2 at instID "<< instID<<"\n");
    __ts_memory_read(addr, 2, instID);
}

void __ts_unaligned_read4(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_unaligned_read4 at instID "<< instID<<"\n");
    __ts_memory_read(addr, 4, instID);
}

void __ts_unaligned_read8(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_unaligned_read8 at instID "<< instID<<"\n");
    __ts_memory_read(addr, 8, instID);
}

void __ts_unaligned_read16(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_unaligned_read16 at instID "<< instID<<"\n");
}

void __ts_self_unaligned_read2(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_unaligned_read2 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_read(addr, 2, instID);
}

void __ts_self_unaligned_read4(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_unaligned_read4 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_read(addr, 4, instID);
}

void __ts_self_unaligned_read8(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_unaligned_read8 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_read(addr, 8, instID);
}

void __ts_self_unaligned_read16(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_unaligned_read16 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
}
//@}

// __ts_unaligned_write calls are emitted by compiler.
/// TS instrumentation interfaces for reads
//@{
void __ts_unaligned_write2(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_unaligned_write2 at instID "<< instID<<"\n");
    __ts_memory_write(addr, 2, instID);
}

void __ts_unaligned_write4(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_unaligned_write4 at instID "<< instID<<"\n");
    __ts_memory_write(addr, 4, instID);
}

void __ts_unaligned_write8(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_unaligned_write8 at instID "<< instID<<"\n");
    __ts_memory_write(addr, 8, instID);
}

void __ts_unaligned_write16(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_unaligned_write16 at instID "<< instID<<"\n");
}

void __ts_self_unaligned_write2(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_unaligned_write2 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_write(addr, 2, instID);
}

void __ts_self_unaligned_write4(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_unaligned_write4 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_write(addr, 4, instID);
}

void __ts_self_unaligned_write8(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_unaligned_write8 at instID "<< instID<<"\n");
    stallbreaker->selfcheck=true;
    __ts_memory_write(addr, 8, instID);
}

void __ts_self_unaligned_write16(void *addr, size_t instID) {
    DBPRINTF(4, std::cout<<"@@ tid: " << get_counter()<<" call __ts_self_unaligned_write16 at instID "<< instID<<"\n");
}
//@}
