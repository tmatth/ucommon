#ifndef _UCOMMON_CPR_H_
#define	_UCOMMON_CPR_H_

#ifndef	_UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#ifndef	_UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifndef	_UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

#ifndef _UCOMMON_STRING_H_
#include <ucommon/string.h>
#endif

#ifndef _UCOMMON_THREAD_H_
#include <ucommon/thread.h>
#endif

#ifdef	_MSWINDOWS_
typedef	HANDLE fd_t;
#else
typedef int fd_t;
#include <dlfcn.h>
#endif

typedef	void (*sighandler_t)(int);

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
#endif

	__EXPORT bool cpr_isasync(fd_t fd);
	__EXPORT bool cpr_isopen(fd_t fd);
	__EXPORT fd_t cpr_openfile(const char *fn, bool rewrite);
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
	__EXPORT key_t cpr_createipc(const char *path, int mode);
	__EXPORT key_t cpr_accessipc(const char *path, int mode);
	__EXPORT sighandler_t cpr_intsignal(int sig, sighandler_t handler);
	__EXPORT sighandler_t cpr_signal(int sig, sighandler_t handler);
	__EXPORT void cpr_hangup(pid_t pid);
	__EXPORT int cpr_sigwait(sigset_t *set);
#else
	#define cpr_signal(sig, handler) signal(sig, handler)
#endif
	__EXPORT pid_t cpr_wait(pid_t pid = 0, int *status = NULL);
	__EXPORT void cpr_sleep(timeout_t timeout);
	__EXPORT void cpr_yield(void);
	__EXPORT int cpr_priority(unsigned priority);
	__EXPORT void cpr_memlock(void *addr, size_t len);
	__EXPORT void cpr_memunlock(void *addr, size_t len);
	__EXPORT fd_t cpr_createctrl(const char *path);
	__EXPORT void cpr_unlinkctrl(const char *path);
	__EXPORT fd_t cpr_openctrl(const char *path);
};

#ifdef	DEBUG
#define cpr_printdbg(fmt, ...)	printf(fmt, ...)
#else
#define	cpr_printdbg(fmt, ...)
#endif

#endif

