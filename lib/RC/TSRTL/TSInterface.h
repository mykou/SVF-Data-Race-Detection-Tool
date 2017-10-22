/*
 * TSInterface.h
 *
 *  Created on: 18 May 2016
 *      Author: Peng Di
 */

#ifndef INCLUDE_THREADSCHEDULING_TSINTERFACE_H_
#define INCLUDE_THREADSCHEDULING_TSINTERFACE_H_

// This header should NOT include any other headers.
// All functions in this header are extern "C" and start with __ts_.

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * This function should be called at the very beginning of the process,
 * before any instrumented code is executed and before any call to malloc.
 */
void __ts_init();

/*!
 * This function should be called at the end of the process
 */
void __ts_fini();

/// TS instrumentation interfaces for memory intrinsic
//@{
void __ts_memmove(void *dst, void *src, int size, unsigned instID);
void __ts_memcpy(void *dst, void *src, int size, unsigned instID);
void __ts_memset(void *dst, int size, unsigned instID);
void __ts_self_memmove(void *dst, void *src, int size, unsigned instID);
void __ts_self_memcpy(void *dst, void *src, int size, unsigned instID);
void __ts_self_memset(void *dst, int size, unsigned instID);
//@}

/// TS instrumentation interfaces for free
//@{
void __ts_free(void *addr, size_t instID);
void __ts_self_free(void *addr, size_t instID);
//@}

/// TS instrumentation interfaces for reads
//@{
void __ts_read1(void *addr, size_t instID);
void __ts_read2(void *addr, size_t instID);
void __ts_read4(void *addr, size_t instID);
void __ts_read8(void *addr, size_t instID);
void __ts_read16(void *addr, size_t instID);
void __ts_self_read1(void *addr, size_t instID);
void __ts_self_read2(void *addr, size_t instID);
void __ts_self_read4(void *addr, size_t instID);
void __ts_self_read8(void *addr, size_t instID);
void __ts_self_read16(void *addr, size_t instID);
//@}

/// TS instrumentation interfaces for writes
//@{
void __ts_write1(void *addr, size_t instID);
void __ts_write2(void *addr, size_t instID);
void __ts_write4(void *addr, size_t instID);
void __ts_write8(void *addr, size_t instID);
void __ts_write16(void *addr, size_t instID);
void __ts_self_write1(void *addr, size_t instID);
void __ts_self_write2(void *addr, size_t instID);
void __ts_self_write4(void *addr, size_t instID);
void __ts_self_write8(void *addr, size_t instID);
void __ts_self_write16(void *addr, size_t instID);
//@}

/// TS instrumentation interfaces for unaligned reads
//@{
void __ts_unaligned_read2(void *addr, size_t instID);
void __ts_unaligned_read4(void *addr, size_t instID);
void __ts_unaligned_read8(void *addr, size_t instID);
void __ts_unaligned_read16(void *addr, size_t instID);
void __ts_self_unaligned_read2(void *addr, size_t instID);
void __ts_self_unaligned_read4(void *addr, size_t instID);
void __ts_self_unaligned_read8(void *addr, size_t instID);
void __ts_self_unaligned_read16(void *addr, size_t instID);
//@}

/// TS instrumentation interfaces for unaligned writes
//@{
void __ts_unaligned_write2(void *addr, size_t instID);
void __ts_unaligned_write4(void *addr, size_t instID);
void __ts_unaligned_write8(void *addr, size_t instID);
void __ts_unaligned_write16(void *addr, size_t instID);
void __ts_self_unaligned_write2(void *addr, size_t instID);
void __ts_self_unaligned_write4(void *addr, size_t instID);
void __ts_self_unaligned_write8(void *addr, size_t instID);
void __ts_self_unaligned_write16(void *addr, size_t instID);
//@}

#ifdef __cplusplus
}  // extern "C"
#endif


#endif /* INCLUDE_THREADSCHEDULING_TSINTERFACE_H_ */
