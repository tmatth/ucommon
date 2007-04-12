#include <config.h>
#include <ucommon/file.h>
#include <errno.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

aio::~aio()
{
	cancel();
}

#ifdef	_POSIX_ASYNC_IO
void aio::cancel(void)
{
	if(!pending)
		return;

	aio_cancel(cb.aio_fildes, &cb);
	err = aio_error(&cb);
	pending = false;
}
#else
void aio::cancel(void)
{
#ifdef	ECANCELED
	err = ECANCELED;
#else
	err = -1;
#endif
}
#endif

#ifdef	_POSIX_ASYNC_IO

bool aio::isPending(void)
{
	if(!pending)
		return false;
	if(err == EINPROGRESS)
		err = aio_error(&cb);
	if(err != EINPROGRESS)
		return false;
	return false;
}

ssize_t aio::result(void)
{
	if(err == EINPROGRESS)
		err = aio_error(&cb);
	if(err == EINPROGRESS)
		return 0;

	pending = false;
	return aio_return(&cb);
}	

void aio::write(int fd, caddr_t buf, size_t len, off_t offset)
{
    cancel();
	err = EINPROGRESS;
    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = len;
    cb.aio_offset = offset;
    if(!aio_write(&cb))
		pending = true;
}

void aio::read(int fd, caddr_t buf, size_t len, off_t offset)
{
	cancel();
	err = EINPROGRESS;
	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = fd;
	cb.aio_buf = buf;
	cb.aio_nbytes = len;
	cb.aio_offset = offset;
	if(!aio_read(&cb))
		pending = true;
}
		
#else
bool aio::isPending(void)
{
	return false;
}

ssize_t aio::result(void)
{
	if(!err)
		return (ssize_t)count;
	else
		return -1;
}

void aio::write(int fd, caddr_t buf, size_t len, off_t offset)
{
    ssize_t res = pwrite(fd, buf, len, offset);
    count = 0;
    err = 0;
    if(res < 1)
        err = errno;
    else
        count = res;
}

void aio::read(int fd, caddr_t buf, size_t len, off_t offset)
{
	ssize_t res = pread(fd, buf, len, offset);
	count = 0;
	err = 0;
	if(res < 1)
		err = errno;
	else
		count = res;
}
#endif

extern "C" bool cpr_isdir(const char *fn)
{
	struct stat ino;

	if(stat(fn, &ino))
		return false;

	if(S_ISDIR(ino.st_mode))
		return true;

	return false;
}

extern "C" bool cpr_isfile(const char *fn)
{
	struct stat ino;

	if(stat(fn, &ino))
		return false;

	if(S_ISREG(ino.st_mode))
		return true;

	return false;
}


