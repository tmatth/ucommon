#ifndef _UCOMMON_FILE_H_
#define	_UCOMMON_FILE_H_

#ifndef _UCOMMON_OBJECT_H_
#include <ucommon/object.h>
#endif

#ifndef _UCOMMON_MEMORY_H_
#include <ucommon/memory.h>
#endif

#ifndef _UCOMMON_THREAD_H_
#include <ucommon/thread.h>
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

#ifdef _POSIX_ASYNCHRONOUS_IO
#include <aio.h>
#endif

NAMESPACE_UCOMMON

class __EXPORT MappedMemory
{
private:
	caddr_t map;
#if defined(_MSWINDOWS_) 
	HANDLE hmap;
#endif
	fd_t fd;

protected:
	size_t size, used;

	virtual void fault(void);

public:
	MappedMemory(const char *fname, size_t len = 0);
	virtual ~MappedMemory();

	inline operator bool() const
		{return (size != 0);};

	inline bool operator!() const
		{return (size == 0);};

	void *sbrk(size_t size);
	void *get(size_t offset);

	inline size_t len(void)
		{return size;};
};

class MappedAssoc : protected MappedMemory, protected keyassoc
{
private:
	pthread_mutex_t mutex;

public:
	MappedAssoc(mempager *pager, const char *fname, size_t len, size_t isize = 177);

	void *find(const char *id, size_t osize, size_t tsize = 32);
};

#if _POSIX_ASYNCHRONOUS_IO > 0

class __EXPORT aio
{
private:
	size_t count;
	bool pending;
	int err;
#ifdef _POSIX_ASYNCHRONOUS_IO
	struct aiocb cb;
#endif

public:
	aio();
	~aio();

	bool isPending(void);
	void read(fd_t fd, caddr_t buf, size_t len, off_t offset);
	void write(fd_t fd, caddr_t buf, size_t len, off_t offset);
	void cancel(void);
	ssize_t result(void);	

	inline size_t transfer(void)
		{return count;};

	inline int error(void)
		{return err;};
};

class __EXPORT aiopager
{
private:
	fd_t fd;
	size_t pagesize;
	off_t offset;
	unsigned index, count, ahead, error, written, fetched;
	caddr_t buffer;
	aio *control;	

public:
	aiopager(fd_t fdes, unsigned pages, unsigned scanahead, off_t start = 0, size_t ps = 1024);
	~aiopager();

	operator bool();
	bool operator!();

	void sync(void);
	void cancel(void);
	caddr_t buf(void);
	unsigned len(void);
	void next(size_t len = 0);

	inline void operator++(void)
		{next();};
};

#endif

template <class T, unsigned I = 32, unsigned H = 177>
class mapped_assoc : public MappedAssoc
{
public:
	inline mapped_assoc(mempager *pager, const char *fn, unsigned members) :
		MappedAssoc(pager, fn, members * (sizeof(T) + I), H) {};

	inline T *operator()(const char *id)
		{return static_cast<T*>(find(id, sizeof(T), I));};

	inline unsigned getUsed(void)
		{return (unsigned)(used / (sizeof(T) + I));};

	inline unsigned getSize(void)
		{return (unsigned)(size / (sizeof(T) + I));};

	inline unsigned getFree(void)
		{return (unsigned)((size - used) / (sizeof(T) + I));};
};

template <class T>
class mapped_array : public MappedMemory
{
public:
	inline mapped_array(const char *fn, unsigned members) : 
		MappedMemory(fn, members * sizeof(T)) {};

	inline void addLock(void)
		{sbrk(sizeof(T));};
	
	inline T *operator()(unsigned idx)
		{return static_cast<T*>(get(idx * sizeof(T)));}

	inline T *operator()(void)
		{return static_cast<T*>(sbrk(sizeof(T)));};
	
	inline T &operator[](unsigned idx)
		{return *(operator()(idx));};

	inline unsigned getSize(void)
		{return (unsigned)(size / sizeof(T));};
};
	
template <class T, unsigned I = 0>
class mapped_share : protected MappedMemory
{
public:
	inline mapped_share(const char *fn) : 
		MappedMemory(fn) {};
	
	inline const char *id(unsigned idx)
		{return static_cast<const char *>(get(idx * (sizeof(T) + I)));};

	inline T *operator()(unsigned idx)
		{return static_cast<const T*>(get(idx * (sizeof(T) + I)) + I);}
	
	inline T &operator[](unsigned idx)
		{return *(operator()(idx));};

	inline unsigned getSize(void)
		{return (unsigned)(size / (sizeof(T) + I));};
};

END_NAMESPACE

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
	__EXPORT ssize_t cpr_readfile(fd_t fd, caddr_t data, size_t len);
	__EXPORT void cpr_seekfile(fd_t fd, off_t offset);
	__EXPORT size_t cpr_filesize(fd_t fd);
	__EXPORT caddr_t cpr_mapfile(const char *fn); 
	__EXPORT bool cpr_isfile(const char *fn);	
	__EXPORT bool cpr_isdir(const char *fn);
}

#endif
