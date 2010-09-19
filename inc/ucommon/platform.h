// Copyright (C) 2006-2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

/**
 * Various miscellaneous platform specific headers and defines.
 * This is used to support ucommon on different platforms.  The ucommon
 * library assumes at least a real posix threading library is present or
 * will build thread support native on Microsoft Windows legacy platform.
 * This header also deals with issues related to common base types.
 * @file ucommon/platform.h
 */

#ifndef	_UCOMMON_PLATFORM_H_
#define	_UCOMMON_PLATFORM_H_
#define	UCOMMON_ABI	3

/**
 * Common namespace for all ucommon objects.
 * We are using a common namespace to easily separate ucommon from other
 * libraries.  This namespace may be changed from ucc to gnu when we
 * merge code with GNU Common C++.  In any case, it is controlled by
 * macros and so any changes will be hidden from user applications so long 
 * as the namespace macros (UCOMMON_NAMESPACE, NAMESPACE_UCOMMON, 
 * END_NAMESPACE) are used in place of direct namespace declarations.
 * @namespace ucc
 */ 

#define	UCOMMON_NAMESPACE	ucc
#define	NAMESPACE_UCOMMON	namespace ucc {
#define	END_NAMESPACE		}

#ifndef	_REENTRANT
#define _REENTRANT 1
#endif

#ifndef	__PTH__
#ifndef	_THREADSAFE
#define	_THREADSAFE 1
#endif

#ifndef	_POSIX_PTHREAD_SEMANTICS
#define	_POSIX_PTHREAD_SEMANTICS
#endif
#endif

#if defined(__GNUC__) && (__GNUC < 3) && !defined(_GNU_SOURCE)
#define	_GNU_SOURCE
#endif

#if __GNUC__ > 3 || (__GNUC__ == 3 && (__GNU_MINOR__ > 3))
#define	__PRINTF(x,y)	__attribute__ ((format (printf, x, y)))
#define	__SCANF(x, y) __attribute__ ((format (scanf, x, y)))
#define	__MALLOC	  __attribute__ ((malloc))
#endif

#ifndef	__MALLOC
#define	__PRINTF(x, y)
#define	__SCANF(x, y)
#define __MALLOC
#endif

#ifndef	DEBUG
#ifndef	NDEBUG
#define	NDEBUG
#endif
#endif

#ifdef	DEBUG
#ifdef	NDEBUG
#undef	NDEBUG
#endif
#endif

// see if we are building for or using extended stdc++ runtime library support

#if defined(NEW_STDCPP) || defined(OLD_STDCPP)
#define	_UCOMMON_EXTENDED_
#endif

// see if targeting legacy Microsoft windows platform 

#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32)
#define	_MSWINDOWS_

//#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0501
//#undef	_WIN32_WINNT
//#define	_WIN32_WINNT 0x0501
//#endif

//#ifndef _WIN32_WINNT
//#define	_WIN32_WINNT 0x0501
//#endif

#pragma warning(disable: 4996)
#pragma warning(disable: 4355)
#pragma warning(disable: 4290)
#pragma	warning(disable: 4291)

#if defined(__BORLANDC__) && !defined(__MT__)
#error Please enable multithreading
#endif

#if defined(_MSC_VER) && !defined(_MT)
#error Please enable multithreading (Project -> Settings -> C/C++ -> Code Generation -> Use Runtime Library)
#endif

// Require for compiling with critical sections.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

// Make sure we're consistent with _WIN32_WINNT
#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#if defined(_MSC_VER)
typedef	signed long ssize_t;
typedef int pid_t;
#endif

#include <process.h>
#ifndef	__EXPORT
#ifdef	UCOMMON_STATIC
#define	__EXPORT
#else
#define	__EXPORT	__declspec(dllimport)
#endif
#endif
#define	__LOCAL
#elif UCOMMON_VISIBILITY > 0
#define	__EXPORT	__attribute__ ((visibility("default")))
#define	__LOCAL		__attribute__ ((visibility("hidden")))
#else
#define	__EXPORT
#define	__LOCAL
#endif

#ifdef	_MSWINDOWS_

#define	_UWIN

#include <sys/stat.h>
#include <io.h>

typedef	DWORD pthread_t;
typedef	CRITICAL_SECTION pthread_mutex_t;
typedef char *caddr_t;
typedef	HANDLE fd_t;
typedef	SOCKET socket_t;

typedef	struct timespec {
	time_t tv_sec;
	long  tv_nsec;
} timespec_t;

extern "C" {

int cpr_setenv(const char *s, const char *v, int p);

inline int setenv(const char *s, const char *v, int overwrite)
	{return cpr_setenv(s, v, overwrite);};

inline void sleep(int seconds)
	{::Sleep((seconds * 1000l));};

inline void pthread_exit(void *p)
	{_endthreadex((DWORD)p);};

inline pthread_t pthread_self(void)
	{return (pthread_t)GetCurrentThreadId();};

inline int pthread_mutex_init(pthread_mutex_t *mutex, void *x)
	{InitializeCriticalSection(mutex); return 0;};

inline void pthread_mutex_destroy(pthread_mutex_t *mutex)
	{DeleteCriticalSection(mutex);};

inline void pthread_mutex_lock(pthread_mutex_t *mutex)
	{EnterCriticalSection(mutex);};

inline void pthread_mutex_unlock(pthread_mutex_t *mutex)
	{LeaveCriticalSection(mutex);};

inline char *strdup(const char *s)
	{return _strdup(s);};

inline int stricmp(const char *s1, const char *s2)
	{return _stricmp(s1, s2);};

inline int strnicmp(const char *s1, const char *s2, size_t l)
	{return _strnicmp(s1, s2, l);};
};

#elif defined(__PTH__)

#include <pth.h>
#include <sys/wait.h>

typedef int socket_t;
typedef int fd_t;
#define	INVALID_SOCKET -1
#define	INVALID_HANDLE_VALUE -1
#include <signal.h>

#define	pthread_mutex_t pth_mutex_t
#define	pthread_cond_t pth_cond_t
#define	pthread_t pth_t

inline int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
	{return pth_sigmask(how, set, oset);};

inline void pthread_exit(void *p)
	{pth_exit(p);};

inline void pthread_kill(pthread_t tid, int sig)
	{pth_raise(tid, sig);};

inline int pthread_mutex_init(pthread_mutex_t *mutex, void *x)
	{return pth_mutex_init(mutex) != 0;};

inline void pthread_mutex_destroy(pthread_mutex_t *mutex)
	{};

inline void pthread_mutex_lock(pthread_mutex_t *mutex)
	{pth_mutex_acquire(mutex, 0, NULL);};

inline void pthread_mutex_unlock(pthread_mutex_t *mutex)
	{pth_mutex_release(mutex);};

inline void pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
	{pth_cond_await(cond, mutex, NULL);};

inline void pthread_cond_signal(pthread_cond_t *cond)
	{pth_cond_notify(cond, FALSE);};

inline void pthread_cond_broadcast(pthread_cond_t *cond)
	{pth_cond_notify(cond, TRUE);};

#else

#include <pthread.h>

typedef	int socket_t;
typedef	int fd_t;
#define	INVALID_SOCKET -1
#define	INVALID_HANDLE_VALUE -1
#include <signal.h>

#endif

#ifdef _MSC_VER
typedef	signed __int8 int8_t;
typedef	unsigned __int8 uint8_t;
typedef	signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef char *caddr_t;

#if defined(_MSC_VER)
#include <stdio.h>
#define	snprintf _snprintf
#define vsnprintf _vsnprintf
#endif
	
#else

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>

#endif

typedef	void (*sighandler_t)(int);	/**< Convenient typedef for signal handlers. */
typedef	unsigned long timeout_t;	/**< Typedef for millisecond timer values. */

#include <stdlib.h>
#include <errno.h>

#ifdef	_MSWINDOWS_
#ifndef	ENETDOWN
#define	ENETDOWN		((int)(WSAENETDOWN))
#endif
#ifndef	EINPROGRESS
#define	EINPROGRESS		((int)(WSAEINPROGRESS))
#endif
#ifndef	ENOPROTOOPT
#define	ENOPROTOOPT		((int)(WSAENOPROTOOPT))
#endif
#ifndef	EADDRINUSE
#define	EADDRINUSE		((int)(WSAEADDRINUSE))
#endif
#ifndef	EADDRNOTAVAIL
#define	EADDRNOTAVAIL	((int)(WSAEADDRNOTAVAIL))
#endif
#ifndef	ENETUNREACH
#define	ENETUNREACH		((int)(WSAENETUNREACH))
#endif
#ifndef	EHOSTUNREACH
#define	EHOSTUNREACH	((int)(WSAEHOSTUNREACH))
#endif
#ifndef	EHOSTDOWN
#define	EHOSTDOWN		((int)(WSAEHOSTDOWN))
#endif
#ifndef	ENETRESET
#define	ENETRESET		((int)(WSAENETRESET))
#endif
#ifndef	ECONNABORTED
#define	ECONNABORTED	((int)(WSAECONNABORTED))
#endif
#ifndef	ECONNRESET
#define	ECONNRESET		((int)(WSAECONNRESET))
#endif
#ifndef	EISCONN
#define	EISCONN			((int)(WSAEISCONN))
#endif
#ifndef	ENOTCONN		
#define	ENOTCONN		((int)(WSAENOTCONN))
#endif
#ifndef	ESHUTDOWN
#define	ESHUTDOWN		((int)(WSAESHUTDOWN))
#endif
#ifndef	ETIMEDOUT
#define	ETIMEDOUT		((int)(WSAETIMEDOUT))
#endif
#ifndef	ECONNREFUSED
#define	ECONNREFUSED	((int)(WSAECONNREFUSED))
#endif
#endif

/**
 * Function to handle runtime errors.  When using the standard C library,
 * runtime errors are handled by a simple abort.  When using the stdc++
 * library with stdexcept, then std::runtime_error will be thrown.
 * @param text of runtime error.
 */
__EXPORT void cpr_runtime_error(const char *text);

/**
 * Portable memory allocation helper function.  Handles out of heap error
 * as a runtime error.
 * @param size of memory block to allocate from heap.
 * @return memory address of allocated heap space.
 */
extern "C" __EXPORT void *cpr_memalloc(size_t size) __MALLOC;

/**
 * Portable memory placement helper function.  This is used to process
 * "placement" new operators where a new object is constructed over a
 * pre-allocated area of memory.  This handles invalid values through
 * runtime error.
 * @param size of object being constructed.
 * @param address where the object is being placed.
 * @param known size of the location we are constructing the object in.
 */
extern "C" __EXPORT void *cpr_memassign(size_t size, caddr_t address, size_t known) __MALLOC;

/**
 * Portable swap code.
 * @param mem1 to swap.
 * @param mem2 to swap.
 * @param size of swap area.
 */
extern "C" __EXPORT void cpr_memswap(void *mem1, void *mem2, size_t size);

#ifndef	_UCOMMON_EXTENDED_
/**
 * Our generic new operator.  Uses our heap memory allocator.
 * @param size of object being constructed.
 * @return memory allocated from heap.
 */
inline void *operator new(size_t size)
	{return cpr_memalloc(size);};

/**
 * Our generic new array operator.  Uses our heap memory allocator.
 * @param size of memory needed for object array.
 * @return memory allocated from heap.
 */
inline void *operator new[](size_t size)
	{return cpr_memalloc(size);};
#endif

#ifndef	_UCOMMON_EXTENDED_
/**
 * A placement new array operator where we assume the size of memory is good.
 * We construct the array at a specified place in memory which we assume is
 * valid for our needs.
 * @param size of memory needed for object array.
 * @param address where to place object array.
 * @return memory we placed object array.
 */
inline void *operator new[](size_t size, caddr_t address)
	{return cpr_memassign(size, address, size);};

/**
 * A placement new array operator where we know the allocated size.  We
 * find out how much memory is needed by the new and can prevent arrayed
 * objects from exceeding the available space we are placing the object.
 * @param size of memory needed for object array.
 * @param address where to place object array.
 * @param known size of location we are placing array.
 * @return memory we placed object array.
 */
inline void *operator new[](size_t size, caddr_t address, size_t known)
	{return cpr_memassign(size, address, known);};
#endif

/**
 * Overdraft new to allocate extra memory for object from heap.  This is
 * used for objects that must have a known class size but store extra data
 * behind the class.  The last member might be an unsized or 0 element
 * array, and the actual size needed from the heap is hence not the size of 
 * the class itself but is known by the routine allocating the object.
 * @param size of object.
 * @param extra heap space needed for data.
 */
inline void *operator new(size_t size, size_t extra)
	{return cpr_memalloc(size + extra);}

/**
 * A placement new operator where we assume the size of memory is good.
 * We construct the object at a specified place in memory which we assume is
 * valid for our needs.
 * @param size of memory needed for object.
 * @param address where to place object.
 * @return memory we placed object.
 */
inline void *operator new(size_t size, caddr_t address)
	{return cpr_memassign(size, address, size);}

/**
 * A placement new operator where we know the allocated size.  We
 * find out how much memory is needed by the new and can prevent the object
 * from exceeding the available space we are placing the object.
 * @param size of memory needed for object.
 * @param address where to place object.
 * @param known size of location we are placing object.
 * @return memory we placed object.
 */

inline void *operator new(size_t size, caddr_t address, size_t known)
	{return cpr_memassign(size, address, known);}

#ifndef	_UCOMMON_EXTENDED_
/**
 * Delete an object from the heap.
 * @param object to delete.
 */
inline void operator delete(void *object)
	{free(object);}

/**
 * Delete an array from the heap.  All array element destructors are called.
 * @param array to delete.
 */
inline void operator delete[](void *array)
	{free(array);}

#ifdef	__GNUC__
extern "C" __EXPORT void __cxa_pure_virtual(void);
#endif
#endif

#ifndef	DEBUG
#ifndef	NDEBUG
#define	NDEBUG
#endif
#endif

#ifdef	DEBUG
#ifdef	NDEBUG
#undef	NDEBUG
#endif
#endif

#include <assert.h>
#ifdef	DEBUG
#define	crit(x, text)	assert(x)
#else
#define	crit(x, text) if(!(x)) cpr_runtime_error(text)
#endif

/**
 * Template function to initialize memory by invoking default constructor.
 * If NULL is passed, then NULL is returned without any constructor called.
 * @param memory to initialize.
 * @return memory initialized.
 */
template<class T>
inline T *init(T *memory)
	{return ((memory) ? new(((caddr_t)memory)) T : NULL);}

extern "C" {

	__EXPORT uint16_t lsb_getshort(uint8_t *b);
	__EXPORT uint32_t lsb_getlong(uint8_t *b);
	__EXPORT uint16_t msb_getshort(uint8_t *b);
	__EXPORT uint32_t msb_getlong(uint8_t *b);

	__EXPORT void lsb_setshort(uint8_t *b, uint16_t v);
	__EXPORT void lsb_setlong(uint8_t *b, uint32_t v);
	__EXPORT void msb_setshort(uint8_t *b, uint16_t v);
	__EXPORT void msb_setlong(uint8_t *b, uint32_t v);

}

typedef	long Integer;
typedef unsigned long Unsigned;
typedef double Real;

#endif
