/*
 * DCIInterface.h
 *
 *  Created on: 18 May 2016
 *      Author: Peng Di
 */

#ifndef INCLUDE_THREADSCHEDULING_TSINTERFACE_H_
#define INCLUDE_THREADSCHEDULING_TSINTERFACE_H_

// This header should NOT include any other headers.
// All functions in this header are extern "C" and start with __dci_.

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * This function should be called at the very beginning of the process,
 * before any instrumented code is executed and before any call to malloc.
 */
void __dci_init(unsigned instr_num);

/*!
 * This function should be called at the end of the process
 */
void __dci_fini();

/// DCI instrumentation interfaces for memory intrinsic
//@{
void __dci_memmove(void *dst, void *src, int size, unsigned instID);
void __dci_memcpy(void *dst, void *src, int size, unsigned instID);
void __dci_memset(void *dst, int size, unsigned instID);
//@}

/// DCI instrumentation interfaces for free instructions
//@{
void __dci_free(void *addr, unsigned instID);
//@}

/// DCI instrumentation interfaces for reads
//@{
void __dci_read1(void *addr, unsigned instID);
void __dci_read2(void *addr, unsigned instID);
void __dci_read4(void *addr, unsigned instID);
void __dci_read8(void *addr, unsigned instID);
void __dci_read16(void *addr, unsigned instID);
//@}

/// DCI instrumentation interfaces for writes
//@{
void __dci_write1(void *addr, unsigned instID);
void __dci_write2(void *addr, unsigned instID);
void __dci_write4(void *addr, unsigned instID);
void __dci_write8(void *addr, unsigned instID);
void __dci_write16(void *addr, unsigned instID);
//@}

/// DCI instrumentation interfaces for unaligned reads
//@{
void __dci_unaligned_read2(void *addr, unsigned instID);
void __dci_unaligned_read4(void *addr, unsigned instID);
void __dci_unaligned_read8(void *addr, unsigned instID);
void __dci_unaligned_read16(void *addr, unsigned instID);
//@}

/// DCI instrumentation interfaces for unaligned writes
//@{
void __dci_unaligned_write2(void *addr, unsigned instID);
void __dci_unaligned_write4(void *addr, unsigned instID);
void __dci_unaligned_write8(void *addr, unsigned instID);
void __dci_unaligned_write16(void *addr, unsigned instID);
//@}

#ifdef __cplusplus
}  // extern "C"
#endif


#endif /* INCLUDE_THREADSCHEDULING_TSINTERFACE_H_ */
