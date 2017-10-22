/*
 * RTLInterception_linux.cpp
 *
 *  Created on: 05/10/2016
 *      Author: pengd
 */

/*
 * \Linux-specific interception methods for runtime library,
 */

#ifdef __linux__
#include "RTLInterception.h"

#include <stddef.h>  // for NULL
#include <dlfcn.h>   // for dlsym

namespace __interception {

/*
 * Returns true if a function with the given name was found.
 */
bool GetRealFunctionAddress(const char *func_name, uptr *func_addr,
                            uptr real, uptr wrapper) {
    *func_addr = (uptr)dlsym(RTLD_NEXT, func_name);
    return real == wrapper;
}
}  // namespace __interception


#endif  // __linux__
