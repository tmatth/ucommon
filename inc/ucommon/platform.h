// Copyright (C) 2006-2007 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

/**
 * Various miscelanous platform specific headers and defines.
 * This is used to support ucommon on different platforms.  The ucommon
 * library assumes at least a real posix threading library is present or
 * will build thread support native on Microsoft Windows legacy platform.
 * This header also deals with issues related to common base types.
 * @file ucommon/platform.h
 */

#ifndef	_UCOMMON_PLATFORM_H_
#define	_UCOMMON_PLATFORM_H_

/**
 * Common namespace for all ucommon objects.
 * We are using a common namespace to easily seperate ucommon from other
 * libraries.  This namespace may be changed from ucc to gnu when we
 * merge code with GNU Common C++.  In any case, it is controlled by
 * macros and so any changes will be hidden from user applications so long 
 * as the namespace macros (UCOMMON_NAMESPACE, NAMESPACE_UCOMMON, 
 * END_NAMESPACE) are used in place of direct namespace declarations.
 * @namespace ucc
 */ 

#define	UCOMMON_NAMESPACE	ucc
#define	NAMESPACE_UCOMMON	namespace ucc {
#define	END_NAMESPACE		};

#ifndef	_REENTRANT
#define _REENTRANT 1
#endif

#ifndef	_THREADSAFE
#define	_THREADSAFE 1
#endif

#ifndef	_POSIX_PTHREAD_SEMANTICS
#define	_POSIX_PTHREAD_SEMANTICS
#endif

#ifdef	__GNUC__
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

#if defined(_MSC_VER) || defined(WIN32) || defined(_WIN32)
#define	_MSWINDOWS_
#if defined(_WIN32_WINNT) && _WIN32_WINNT < 0x0501
#undef	_WIN32_WINNT
#define	_WIN32_WINNT 0x0501
#endif

#ifndef _WIN32_WINNT
#define	_WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <process.h>
#ifndef	__EXPORT
#define	__EXPORT	__declspec(dllimport)
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

#include <sys/stat.h>
#include <malloc.h>

typedef	DWORD pthread_t;
typedef	CRITICAL_SECTION pthread_mutex_t;
typedef char *caddr_t;
typedef	HANDLE fd_t;

typedef	struct timespec {
	time_t tv_sec;
	long  tv_nsec;
} timespec_t;

extern "C" {

inline void pthread_exit(void *p)
	{_endthreadex((DWORD)p);};

inline bool pthread_equal(pthread_t x, pthread_t y)
	{return (x == y);};

inline void pthread_mutex_init(pthread_mutex_t *mutex, void *x)
	{InitializeCriticalSection(mutex);};

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

inline int stat(const char *path, struct stat *buf)
	{return _stat(path, (struct _stat *)(buf));};

};

#else

#include <pthread.h>

typedef	int SOCKET;
typedef	int fd_t;
#define	INVALID_SOCKET -1
#define	INVALID_HANDLE_VALUE -1
#include <dlfcn.h>
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

#define	snprintf _snprintf
#define vsnprintf _vsnprintf
	
#else

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>

#endif

typedef	void (*sighandler_t)(int);
typedef	unsigned long timeout_t;
typedef	int32_t rpcint_t;
typedef	rpcint_t rpcbool_t;
typedef	char *rpcstring_t;
typedef	double rpcdouble_t;

#include <stdlib.h>

extern "C" __EXPORT void *cpr_memalloc(size_t size) __MALLOC;
extern "C" __EXPORT void *cpr_memassign(size_t size, caddr_t place, size_t max) __MALLOC;

inline void *operator new(size_t size)
	{return cpr_memalloc(size);};

inline void *operator new[](size_t size)
	{return cpr_memalloc(size);};

inline void *operator new[](size_t size, caddr_t place)
	{return cpr_memassign(size, place, size);};

inline void *operator new[](size_t size, caddr_t place, size_t max)
	{return cpr_memassign(size, place, max);};

inline void *operator new(size_t size, size_t extra)
	{return cpr_memalloc(size + extra);};

inline void *operator new(size_t size, caddr_t place)
	{return cpr_memassign(size, place, size);};

inline void *operator new(size_t size, caddr_t place, size_t max)
	{return cpr_memassign(size, place, max);};

inline void operator delete(void *mem)
	{free(mem);};

inline void operator delete[](void *mem)
	{free(mem);};

#ifdef	__GNUC__
extern "C" __EXPORT void __cxa_pure_virtual(void);
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
#define	crit(x)	assert(x)
#else
#define	crit(x) if(!(x)) abort()
#endif

extern "C" {

	__EXPORT uint16_t lsb_getshort(uint8_t *b);
	__EXPORT uint32_t lsb_getlong(uint8_t *b);
	__EXPORT uint16_t msb_getshort(uint8_t *b);
	__EXPORT uint32_t msb_getlong(uint8_t *b);

	__EXPORT void lsb_setshort(uint8_t *b, uint16_t v);
	__EXPORT void lsb_setlong(uint8_t *b, uint32_t v);
	__EXPORT void msb_setshort(uint8_t *b, uint16_t v);
	__EXPORT void msb_setlong(uint8_t *b, uint32_t v);

};

#ifdef	DEBUG
#define cpr_debug(fmt, ...)	fprintf(stderr, fmt, ...)
#else
#define	cpr_debug(fmt, ...)
#endif

#endif
