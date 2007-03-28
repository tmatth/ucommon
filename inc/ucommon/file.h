#ifndef _UCOMMON_FILE_H_
#define	_UCOMMON_FILE_H_

#ifndef _UCOMMON_OBJECT_H_
#include <ucommon/object.h>
#endif

#ifndef	_MSWINDOWS_
#include <dlfcn.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef	_MSWINDOWS_
typedef	HANDLE fd_t;
#else
typedef int fd_t;
#endif

#ifdef _POSIX_ASYNC_IO
#include <aio.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT aio
{
private:
	off_t offset;
	size_t count;
	bool pending;
	int err;
	int fd;
#ifdef _POSIX_ASYNC_IO
	struct aiocb cb;
#endif

public:
	aio(int fd);
	~aio();

	void pos(off_t offset);
	void set(int fd);
	void read(caddr_t buf, size_t len);
	void write(caddr_t buf, size_t len);
	void cancel(void);
	ssize_t result(void);	

	inline int error(void)
		{return err;};
};

class __EXPORT auto_close : private AutoObject
{
private:
	union {
#ifdef	_MSWINDOWS_
		HANDLE h;
#endif
		FILE *fp;
		DIR *dp;
		int fd;
	}	obj;

	enum {
#ifdef	_MSWINDOWS_
		T_HANDLE,
#endif
		T_FILE,
		T_DIR,
		T_FD,
		T_CLOSED
	} type;

	void release(void);

public:

#ifdef	_MSWINDOWS_
	auto_close(HANDLE hv);
#endif

	auto_close(FILE *fp);
	auto_close(DIR *dp);
	auto_close(int fd);
	~auto_close();
};

#define	autoclose(x)	auto_close __access_name(__ac__)(x)

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
 
	__EXPORT bool cpr_isfile(const char *fn);	
	__EXPORT bool cpr_isdir(const char *fn);
}

END_NAMESPACE

#endif
