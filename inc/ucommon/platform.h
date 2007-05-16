#ifndef	_UCOMMON_PLATFORM_H_
#define	_UCOMMON_PLATFORM_H_

#define	UCOMMON_NAMESPACE	ucc
#define	NAMESPACE_UCOMMON	namespace ucc {
#define	END_NAMESPACE		};

#ifndef	_REENTRANT
#define _REENTRANT 1
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

#include <pthread.h>

#ifdef	_MSWINDOWS_

#include <sys/stat.h>
#include <malloc.h>

typedef char *caddr_t;
typedef	HANDLE fd_t;

extern "C" {

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

#if _POSIX_MAPPED_FILES > 0 && _POSIX_SHARED_MEMORY_OBJECTS > 0
#define	UCOMMON_MAPPED_MEMORY 1
#endif

#if _POSIX_MAPPED_FILES > 0 || defined(_MSWINDOWS_)
#define	UCOMMON_MAPPED_FILES 1
#endif

#if _POSIX_SHARED_MEMORY_OBJECTS > 0 && _POSIX_THREAD_PROCESS_SHARED > 0
#define	UCOMMON_MAPPED_THREADS 1
#endif

#if _POSIX_ASYCHRONOUS_IO > 0
#define UCOMMON_ASYNC_IO 1
#endif

extern "C" {
#ifdef	_MSWINDOWS_
#define	RTLD_LAZY 0
#define	RTLD_NOW 0
#define	RTLD_LOCAL 0
#define RTLD_GLOBAL 0
	
	typedef	HINSTANCE loader_handle_t;

	inline bool cpr_isloaded(loader_handle_t mem)
		{return mem != NULL;};

	inline loader_handle_t cpr_load(const char *fn, unsigned flags)
		{return LoadLibrary(fn);};

	inline void cpr_unload(loader_handle_t mem)
		{FreeLibrary(mem);};

	inline void *cpr_getloadaddr(loader_handle_t mem, const char *sym)
		{return (void *)GetProcAddress(mem, sym);};

	inline bool cpr_removedir(const char *path)
		{return RemoveDirectory(path);};

#else
	typedef	void *loader_handle_t;

	inline bool cpr_isloaded(loader_handle_t mem)
		{return mem != NULL;};

	inline loader_handle_t cpr_load(const char *fn, unsigned flags)
		{return dlopen(fn, flags);};

	inline const char *cpr_loaderror(void)
		{return dlerror();};

	inline void *cpr_getloadaddr(loader_handle_t mem, const char *sym)
		{return dlsym(mem, sym);};

	inline void cpr_unload(loader_handle_t mem)
		{dlclose(mem);};

	inline void cpr_removedir(const char *fn)
		{rmdir(fn);};
#endif

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
