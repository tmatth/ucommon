#ifndef _UCOMMON_FILE_H_
#define	_UCOMMON_FILE_H_

#ifndef _UCOMMON_OBJECT_H_
#include <ucommon/object.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef _POSIX_ASYNCHRONOUS_IO
#include <aio.h>
#endif

NAMESPACE_UCOMMON

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

template <class T, unsigned C = 1>
class aio_array : public aio
{
public:
	fd_t fd;
	T value[C];

	inline aio_array(fd_t fh) : 
		aio() {fd = fh;};

	inline T* operator*()
		{return &value;};

	inline T operator[](unsigned idx)
		{return value[idx];};

	inline void get(unsigned long offset)
		{aio::read(fd, &value, sizeof(T) * C, sizeof(T) * offset);};

	inline void put(unsigned long offset)
		{aio::write(fd, &value, sizeof(T) * C, sizeof(T) * offset);};
};

#endif

END_NAMESPACE

#endif
