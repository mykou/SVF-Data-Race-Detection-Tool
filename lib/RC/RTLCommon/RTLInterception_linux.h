/*
 * RTLInterception_linux.h
 *
 *  Created on: 05/10/2016
 *      Author: pengd
 */

/*!
 * \Linux-specific interception methods for runtime library,
 */


#ifdef __linux__

#if !defined(INCLUDED_FROM_INTERCEPTION_LIB)
# error "interception_linux.h should be included from interception library only"
#endif

#ifndef INTERCEPTION_LINUX_H
#define INTERCEPTION_LINUX_H

namespace __interception {

/*!
 * Returns true if a function with the given name was found.
 */
bool GetRealFunctionAddress(const char *func_name, uptr *func_addr,
                            uptr real, uptr wrapper);
}  // namespace __interception

#define INTERCEPT_FUNCTION_LINUX(func) \
    ::__interception::GetRealFunctionAddress( \
          #func, (::__interception::uptr*)&REAL(func), \
          (::__interception::uptr)&(func), \
          (::__interception::uptr)&WRAP(func))

#endif  // INTERCEPTION_LINUX_H
#endif  // __linux__
