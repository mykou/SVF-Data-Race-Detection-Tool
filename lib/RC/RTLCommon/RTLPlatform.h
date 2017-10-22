/*
 * RTLPlatform.h
 *
 *  Created on: 05/10/2016
 *      Author: pengd
 */

/*!
 * \Common platform macros.
 */

#ifndef RTL_PLATFORM_H
#define RTL_PLATFORM_H

#if !defined(__linux__) && !defined(__FreeBSD__) && \
  !defined(__APPLE__) && !defined(_WIN32)
# error "This operating system is not supported"
#endif

#if defined(__linux__)
# define RTL_LINUX   1
#else
# define RTL_LINUX   0
#endif

#if defined(__FreeBSD__)
# define RTL_FREEBSD 1
#else
# define RTL_FREEBSD 0
#endif

#if defined(__APPLE__)
# define RTL_MAC     1
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE
#  define RTL_IOS    1
# else
#  define RTL_IOS    0
# endif
# if TARGET_IPHONE_SIMULATOR
#  define RTL_IOSSIM 1
# else
#  define RTL_IOSSIM 0
# endif
#else
# define RTL_MAC     0
# define RTL_IOS     0
# define RTL_IOSSIM  0
#endif

#if defined(_WIN32)
# define RTL_WINDOWS 1
#else
# define RTL_WINDOWS 0
#endif

#if defined(__ANDROID__)
# define RTL_ANDROID 1
#else
# define RTL_ANDROID 0
#endif

#define RTL_POSIX (RTL_FREEBSD || RTL_LINUX || RTL_MAC)

#if __LP64__ || defined(_WIN64)
#  define RTL_WORDSIZE 64
#else
#  define RTL_WORDSIZE 32
#endif

#if RTL_WORDSIZE == 64
# define FIRST_32_SECOND_64(a, b) (b)
#else
# define FIRST_32_SECOND_64(a, b) (a)
#endif

#if defined(__x86_64__) && !defined(_LP64)
# define RTL_X32 1
#else
# define RTL_X32 0
#endif

// By default we allow to use SizeClassAllocator64 on 64-bit platform.
// But in some cases (e.g. AArch64's 39-bit address space) SizeClassAllocator64
// does not work well and we need to fallback to SizeClassAllocator32.
// For such platforms build this code with -DRTL_CAN_USE_ALLOCATOR64=0 or
// change the definition of RTL_CAN_USE_ALLOCATOR64 here.
#ifndef RTL_CAN_USE_ALLOCATOR64
# if defined(__mips64) || defined(__aarch64__)
#  define RTL_CAN_USE_ALLOCATOR64 0
# else
#  define RTL_CAN_USE_ALLOCATOR64 (RTL_WORDSIZE == 64)
# endif
#endif

// The range of addresses which can be returned my mmap.
// FIXME: this value should be different on different platforms.  Larger values
// will still work but will consume more memory for TwoLevelByteMap.
#if defined(__mips__)
# define RTL_MMAP_RANGE_SIZE FIRST_32_SECOND_64(1ULL << 32, 1ULL << 40)
#else
# define RTL_MMAP_RANGE_SIZE FIRST_32_SECOND_64(1ULL << 32, 1ULL << 47)
#endif

// The AArch64 linux port uses the canonical syscall set as mandated by
// the upstream linux community for all new ports. Other ports may still
// use legacy syscalls.
#ifndef RTL_USES_CANONICAL_LINUX_SYSCALLS
# if defined(__aarch64__) && RTL_LINUX
# define RTL_USES_CANONICAL_LINUX_SYSCALLS 1
# else
# define RTL_USES_CANONICAL_LINUX_SYSCALLS 0
# endif
#endif

// udi16 syscalls can only be used when the following conditions are
// met:
// * target is one of arm32, x86-32, sparc32, sh or m68k
// * libc version is libc5, glibc-2.0, glibc-2.1 or glibc-2.2 to 2.15
//   built against > linux-2.2 kernel headers
// Since we don't want to include libc headers here, we check the
// target only.
#if defined(__arm__) || RTL_X32 || defined(__sparc__)
#define RTL_USES_UID16_SYSCALLS 1
#else
#define RTL_USES_UID16_SYSCALLS 0
#endif

#if defined(__mips__)
# define RTL_POINTER_FORMAT_LENGTH FIRST_32_SECOND_64(8, 10)
#else
# define RTL_POINTER_FORMAT_LENGTH FIRST_32_SECOND_64(8, 12)
#endif

// Assume obsolete RPC headers are available by default
#if !defined(HAVE_RPC_XDR_H) && !defined(HAVE_TIRPC_RPC_XDR_H)
# define HAVE_RPC_XDR_H (RTL_LINUX && !RTL_ANDROID)
# define HAVE_TIRPC_RPC_XDR_H 0
#endif

/// \macro MSC_PREREQ
/// \brief Is the compiler MSVC of at least the specified version?
/// The common \param version values to check for are:
///  * 1800: Microsoft Visual Studio 2013 / 12.0
///  * 1900: Microsoft Visual Studio 2015 / 14.0
#ifdef _MSC_VER
# define MSC_PREREQ(version) (_MSC_VER >= (version))
#else
# define MSC_PREREQ(version) 0
#endif

#endif // RTL_PLATFORM_H
