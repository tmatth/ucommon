#include <config.h>
#include <ucommon/file.h>
#include <ucommon/cpr.h>
#include <stdarg.h>

#if _POSIX_MAPPED_FILES > 0
#include <sys/mman.h>
#endif

#include <errno.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

#if UCOMMON_ASYNC_IO > 0

aio::aio()
{
	pending = false;
	count = 0;
	err = 0;
}

aio::~aio()
{
	cancel();
}

void aio::cancel(void)
{
	if(!pending)
		return;

	count = 0;
	aio_cancel(cb.aio_fildes, &cb);
	err = aio_error(&cb);
	pending = false;
}

bool aio::isPending(void)
{
	if(!pending)
		return false;
	if(err == EINPROGRESS)
		err = aio_error(&cb);
	if(err != EINPROGRESS)
		return false;
	return true;
}

ssize_t aio::result(void)
{
	ssize_t res;
	if(err == EINPROGRESS)
		err = aio_error(&cb);
	if(err == EINPROGRESS)
		return 0;

	pending = false;
	res = aio_return(&cb);
	if(res > -1)
		count = res;
	return res;
}	

void aio::write(fd_t fd, caddr_t buf, size_t len, off_t offset)
{
    cancel();
	err = EINPROGRESS;
    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = len;
    cb.aio_offset = offset;
    if(!aio_write(&cb)) {
		count = 0;
		pending = true;
	}
}

void aio::read(fd_t fd, caddr_t buf, size_t len, off_t offset)
{
	cancel();
	err = EINPROGRESS;
	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = fd;
	cb.aio_buf = buf;
	cb.aio_nbytes = len;
	cb.aio_offset = offset;
	if(!aio_read(&cb)) {
		count = 0;
		pending = true;
	}
}
		
aiopager::aiopager(fd_t fdes, unsigned pages, unsigned scan, off_t start, size_t ps)
{
	offset = start;
	count = pages;
	ahead = scan;
	pagesize = ps;
	written = count;
	fetched = 0;
	error = 0;
	fd = fdes;
	control = new aio[count];
	buffer = new char[count * pagesize];

	if(ahead > count)
		ahead = count;
}

aiopager::~aiopager()
{
	cancel();
	fd = cpr_closefile(fd);

	if(buffer)
		delete[] buffer;
	if(control)
		delete[] control;
	buffer = NULL;
	control = NULL;
}

void aiopager::sync(void)
{
	if(fd < 0)
		return;

	for(unsigned pos = 0; pos < count; ++pos) {
		while(control[pos].isPending())
			cpr_sleep(10);
		control[pos].result();
	}
	written = count;
	error = 0;
}

void aiopager::cancel(void)
{
	if(fd < 0)
		return;

	for(unsigned pos = 0; pos < count; ++pos) {
		if(control[pos].isPending())
			control[pos].cancel();
	}
	written = count;
	fetched = 0;
	error = 0;
}

caddr_t aiopager::buf(void)
{
	unsigned apos = index + fetched;
	off_t aoffset = offset + (pagesize * fetched);

	while(!error && ahead && fetched < ahead) {
		++fetched;
		if(apos >= count)
			apos -= count;
		control[apos].read(fd, &buffer[apos * pagesize], pagesize, aoffset);
		++apos;
		aoffset += pagesize;
	}
	if(!error || index != written) {
		if(control[index].isPending())
			return NULL;

		if(ahead || index == written)
			error = control[index].result();

		if(!ahead && ++written >= count)
			written = 0;
	}	
	return &buffer[index * pagesize];
}

void aiopager::next(size_t len)
{
	if(!len)
		len = pagesize;

	if(!ahead) {
		if(written == count)
			written = index;
		control[index].write(fd, &buffer[index * pagesize], len, offset);
	}
	if(++index >= count)
		index = 0;
	
	offset += len;

	if(fetched)
		--fetched;
}

size_t aiopager::len(void)
{
	if(error || control[index].isPending())
		return 0;
	
	return control[index].transfer();
}

#endif


