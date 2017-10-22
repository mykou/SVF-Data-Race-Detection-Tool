/*
 * DCIInterceptors.h
 *
 *  Created on: 10 Jun 2016
 *      Author: pengd
 */

#ifndef DCIRTL_INTERCEPTORS_H_
#define DCIRTL_INTERCEPTORS_H_

#include "../RTLCommon/RTLInterception.h"
#include <map>

namespace __dci {
typedef unsigned long uptr;

/// Thread ID key
extern pthread_key_t counter;

#if SANITIZER_FREEBSD
#define __libc_free __free
#define __libc_malloc __malloc
#endif

/*!
 * Get current thread ID
 */
uptr get_counter();

/*!
 * Set current thread ID
 */
int set_counter(uptr value);

/*!
 * Initialize interceptor
 */
void InitializeInterceptors();

extern "C" void __libc_free(void *ptr);
extern "C" void *__libc_malloc(uptr size);

}
#endif /* DCIRTL_INTERCEPTORS_H_ */

