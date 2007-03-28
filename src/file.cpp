#include <private.h>
#include <ucommon/file.h>
#include <errno.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

aio::aio(int f)
{
	fd = f;
	offset = (off_t)0;
	pending = false;	
	err = 0;
}

aio::~aio()
{
	cancel();
}

void aio::set(int f)
{
	cancel();
	fd = f;
	offset = (off_t)0;
	pending = false;
	err = 0;
}

#ifdef	_POSIX_ASYNC_IO
void aio::cancel(void)
{
	if(!pending)
		return;

	aio_cancel(fd, &cb);
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
ssize_t aio::result(void)
{
	ssize_t rc;
	err = aio_error(&cb);
	if(err == EINPROGRESS)
		return 0;

	rc = aio_return(&cb);
	if(rc > 0)
		offset += rc;

	return rc;
}	

void aio::write(caddr_t buf, size_t len)
{
    cancel();
    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = len;
    cb.aio_offset = offset;
    aio_write(&cb);
}

void aio::read(caddr_t buf, size_t len)
{
	cancel();
	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = fd;
	cb.aio_buf = buf;
	cb.aio_nbytes = len;
	cb.aio_offset = offset;
	aio_read(&cb);
}
		
#else
ssize_t aio::result(void)
{
	if(!err)
		return (ssize_t)count;
	else
		return -1;
}

void aio::write(caddr_t buf, size_t len)
{
    ssize_t res = pwrite(fd, buf, len, offset);
    count = 0;
    err = 0;
    if(res < 1)
        err = errno;
    else
        count = res;
    offset += count;
}

void aio::read(caddr_t buf, size_t len)
{
	ssize_t res = pread(fd, buf, len, offset);
	count = 0;
	err = 0;
	if(res < 1)
		err = errno;
	else
		count = res;
	offset += count;
}
#endif

auto_close::auto_close(FILE *fp) :
AutoObject()
{
	obj.fp = fp;
	type = T_FILE;
}

auto_close::auto_close(DIR *dp) :
AutoObject()
{
	obj.dp = dp;
	type = T_DIR;
}

#ifdef	_MSWINDOWS_
auto_close::auto_close(HANDLE hv) :
AutoObject()
{
    obj.h = hv;
    type = T_HANDLE;
}

#endif

auto_close::auto_close(int fd) :
AutoObject()
{
	obj.fd = fd;
	type = T_FD;
}

auto_close::~auto_close()
{
	delist();
	auto_close::release();
}

void auto_close::release(void)
{
	switch(type) {
#ifdef	_MSWINDOWS_
	case T_HANDLE:
		::CloseHandle(obj.h);
		break;
#endif
	case T_DIR:
		::closedir(obj.dp);
		break;
	case T_FILE:
		::fclose(obj.fp);
		break;
	case T_FD:
		::close(obj.fd);
	case T_CLOSED:
		break;
	}
	type = T_CLOSED;
}

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


