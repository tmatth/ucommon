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

typedef	int fd_t;
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

extern "C" __EXPORT void *cpr_memalloc(size_t size);
extern "C" __EXPORT void *cpr_memassign(size_t size, caddr_t place, size_t max);

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

#define	CPR_PRIORITY_LOWEST 0
#define	CPR_PRIORITY_LOW 1
#define	CPR_PRIORITY_NORMAL 2
#define	CPR_PRIORITY_HIGH 3

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

	__EXPORT bool cpr_isasync(fd_t fd);
	__EXPORT bool cpr_isopen(fd_t fd);
	__EXPORT bool cpr_createdir(const char *fn, bool pub = false);
	__EXPORT fd_t cpr_createfile(const char *fn, bool pub = false);
	__EXPORT fd_t cpr_rewritefile(const char *fn, bool pub = false);
	__EXPORT fd_t cpr_openfile(const char *fn, bool write);
	__EXPORT fd_t cpr_closefile(fd_t fd);
	__EXPORT ssize_t cpr_preadfile(fd_t fd, caddr_t data, size_t len, off_t offset);
	__EXPORT ssize_t cpr_pwritefile(fd_t fd, caddr_t data, size_t len, off_t offset);
	__EXPORT ssize_t cpr_readfile(fd_t fd, caddr_t data, size_t len);
	__EXPORT ssize_t cpr_writefile(fd_t fd, caddr_t data, size_t len);
	__EXPORT void cpr_seekfile(fd_t fd, off_t offset);
	__EXPORT size_t cpr_filesize(fd_t fd);
	__EXPORT caddr_t cpr_mapfile(const char *fn); 
	__EXPORT bool cpr_isfile(const char *fn);	
	__EXPORT bool cpr_isdir(const char *fn);	
	__EXPORT void cpr_printlog(const char *path, const char *fmt, ...);
	__EXPORT size_t cpr_pagesize(void);
	__EXPORT int cpr_scheduler(int policy, unsigned priority = CPR_PRIORITY_NORMAL);
	__EXPORT void cpr_pattach(const char *path);
	__EXPORT void cpr_pdetach(void);
	__EXPORT void cpr_closeall(void);
	__EXPORT void cpr_cancel(pid_t pid);
#ifndef	_MSWINDOWS_
	__EXPORT sighandler_t cpr_intsignal(int sig, sighandler_t handler);
	__EXPORT sighandler_t cpr_signal(int sig, sighandler_t handler);
	__EXPORT void cpr_hangup(pid_t pid);
	__EXPORT int cpr_sigwait(sigset_t *set);

	inline void cpr_reload(pid_t pid)
		{kill(pid, SIGHUP);};

	inline void cpr_terminate(pid_t pid)
		{kill(pid, SIGTERM);};
#else
	#define cpr_signal(sig, handler) signal(sig, handler)
#endif
	__EXPORT pid_t cpr_waitpid(pid_t pid = 0, int *status = NULL);
	__EXPORT int cpr_exitpid(pid_t pid);
	__EXPORT void cpr_sleep(timeout_t timeout);
	__EXPORT void cpr_yield(void);
	__EXPORT int cpr_priority(unsigned priority);
	__EXPORT void cpr_memlock(void *addr, size_t len);
	__EXPORT void cpr_memunlock(void *addr, size_t len);
	__EXPORT fd_t cpr_createctrl(const char *path);
	__EXPORT void cpr_unlinkctrl(const char *path);
	__EXPORT fd_t cpr_openctrl(const char *path);
	__EXPORT void cpr_infolog(const char *fmt, ...);
	__EXPORT void cpr_notice(const char *fmt, ...);
	__EXPORT void cpr_warning(const char *fmt, ...);
	__EXPORT void cpr_errlog(const char *fmt, ...);
	__EXPORT void cpr_critlog(const char *fmt, ...);

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
#define cpr_printdbg(fmt, ...)	printf(fmt, ...)
#else
#define	cpr_printdbg(fmt, ...)
#endif

#endif
